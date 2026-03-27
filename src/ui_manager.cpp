#include "ui_manager.h"
#include "display_hal.h"
#include "button_hal.h"
#include "max30100_hal.h"
#include "bmi160_hal.h"
#include "power_manager.h"
#include "app_config.h"
#include "assets_wallpaper.h"
#include "assets_icons.h"

// States extension (Internal)
#define STATE_TIMER_ALARM 99 

// Persist - RTC_DATA_ATTR
static RTC_DATA_ATTR bool aod_allowed = false; 
static RTC_DATA_ATTR int brightness_ui_val = 127; 
static AppState current_state = STATE_WATCHFACE;

// [UI AGENT] Claude Premium HR Colors & Vars
#define C_BG        0x0000 
#define C_HR_RED    0xE926 
#define C_SPO2      0xF48E 
#define C_MUTED     0x8C71 
#define C_DIVIDER   0x18C3
#define C_ACCENT    0xF96A  // [UI AGENT] Spec: #FF2D55 (Vibrant Red)
#define C_DIM       0x4208  // [UI AGENT] Dim text color
#define C_SURFACE   0x0861  // [UI AGENT] Card surface color
static uint32_t hr_start_time = 0;
static uint8_t  last_bpm_v = 0;
static uint8_t  last_spo2_v = 0;

// Timer Vars
static int timer_h = 0, timer_m = 0, timer_s = 0;
static unsigned long timer_target_ms = 0;
static bool timer_running = false;
static uint8_t alarm_color_idx = 0;
static unsigned long last_alarm_flip = 0;

// Stopwatch Vars
static unsigned long sw_start_ms = 0;
static unsigned long sw_elapsed_ms = 0;
static bool sw_running = false;

// Runtime
static bool is_dimmed_aod = false; 
static unsigned long last_activity_time = 0;
static AppState last_rendered_state = (AppState)-1;
static float current_fps = 0;
static uint32_t draw_count = 0;
static uint32_t last_fps_time = 0;

// Time
static int clock_h = 10, clock_m = 10;
static int last_min_val = -1;
static uint8_t last_batt_val = 255;
static bool mid_reset_done = false; // [POWER AGENT] Track midnight reset
static unsigned long last_tick = 0;

// Sprites
static TFT_eSprite clock_spr  = TFT_eSprite(&display_hal_get_tft());
static TFT_eSprite status_spr = TFT_eSprite(&display_hal_get_tft());
static TFT_eSprite fps_spr    = TFT_eSprite(&display_hal_get_tft());
static TFT_eSprite top_clock_spr = TFT_eSprite(&display_hal_get_tft());
static TFT_eSprite menu_spr   = TFT_eSprite(&display_hal_get_tft()); 
static TFT_eSprite graph_spr  = TFT_eSprite(&display_hal_get_tft()); // [UI AGENT] New Heart Graph Sprite
static TFT_eSprite heart_spr  = TFT_eSprite(&display_hal_get_tft()); // [UI AGENT] New Heart Icon Sprite
static TFT_eSprite bottom_spr = TFT_eSprite(&display_hal_get_tft()); // [UI AGENT] Bottom Row Sprite

// Helpers
static void render_current_state();
static uint8_t get_battery_percentage();
static void draw_watchface(TFT_eSPI& tft, bool full_redraw);
static void push_top_clock(bool force = false);
static void animate_slide_transition(AppState from, AppState to, int dir);
static void build_menu_sprite(TFT_eSprite& spr, AppState st, int bx, int by);
static void push_wallpaper_rect(int x, int y, int w, int h);
static void draw_brightness_icon(TFT_eSprite& spr, int x, int y, uint16_t color);
static void draw_heart_icon(TFT_eSPI &tft, int cx, int cy, uint16_t color, float scale);

// ─── CONSTANTS ──────────────────────────────────────────────────────────
static const int MENU_BX = 20, MENU_BY = 90, MENU_BW = 200, MENU_BH = 100;
static const int MENU_RADIUS = 15;
static const uint16_t BOX_COL = 0x18C3; 

// ─── INIT ────────────────────────────────────────────────────────────────
void ui_manager_init() {
    last_activity_time = millis(); last_tick = millis();
    clock_spr.createSprite(180, 150); clock_spr.setSwapBytes(true); 
    status_spr.createSprite(65, 25);  status_spr.setSwapBytes(true);
    fps_spr.createSprite(35, 15);     fps_spr.setSwapBytes(true);
    top_clock_spr.createSprite(60, 15); top_clock_spr.setSwapBytes(true);
    menu_spr.createSprite(MENU_BW, MENU_BH); menu_spr.setSwapBytes(true);
    graph_spr.createSprite(208, 80);  graph_spr.setSwapBytes(true); // Claude Spec: 208x80
    heart_spr.createSprite(32, 32);  heart_spr.setSwapBytes(true); // Heart Icon Sprite
    bottom_spr.createSprite(240, 65); bottom_spr.setSwapBytes(true); // Bottom UI Sprite
    
    // UI ZERO-FLICKER BOOT STRATEGY:
    display_hal_display_off(); // Mask hardware noise
    display_hal_backlight_set(0); 
    
    TFT_eSPI& tft = display_hal_get_tft();
    
    // 1. COLD SCRUB: Wipe the hardware VRAM with pure black while backlight is DEAD
    tft.fillScreen(TFT_BLACK);
    delay(50);
    
    // 2. LAYER RECONSTRUCTION: Build the stable Watchface hidden in VRAM
    render_current_state(); 
    delay(50);
    render_current_state(); // Double assurance for ST7789 internal buffers
    delay(100); 

    // 3. UNMASK: Reveal the stable image before the lights go on
    display_hal_display_on(); 
    delay(20);

    // 4. BREATHING IN: Smoothly transition from dark to target brightness
    display_hal_backlight_fade_in(brightness_ui_val, 150); // FAST BREATH (150ms)
    
    // 5. SYNC: Map current state as "already rendered" to prevent redundant first-loop redraw
    last_rendered_state = current_state;
}

static void push_wallpaper_rect(int x, int y, int w, int h) {
    TFT_eSPI& tft = display_hal_get_tft();
    const uint16_t* wall = assets_get_wallpaper();
    tft.setWindow(x, y, x + w - 1, y + h - 1);
    for (int i = 0; i < h; i++)
        tft.pushPixels((uint16_t*)wall + (y + i) * 240 + x, w);
}

static void push_top_clock(bool force) {
    if (is_dimmed_aod || current_state == STATE_WATCHFACE) return;
    if (current_state == (int)STATE_TIMER_ALARM || current_state == STATE_EXEC_TIMER || current_state >= STATE_SET_TIMER_H || current_state == STATE_EXEC_STOPWATCH || current_state == STATE_EXEC_HR) return; 
    if (!force && clock_m == last_min_val) return; 
    int tx = 90, ty = 8;
    top_clock_spr.pushImage(-tx, -ty, 240, 280, assets_get_wallpaper());
    top_clock_spr.setTextDatum(MC_DATUM); top_clock_spr.setTextColor(TFT_WHITE); top_clock_spr.setTextSize(1);
    char buf[8]; sprintf(buf, "%02d:%02d", clock_h, clock_m);
    top_clock_spr.drawString(buf, 30, 8); top_clock_spr.pushSprite(tx, ty);
    last_min_val = clock_m;
}

static void draw_brightness_icon(TFT_eSprite& spr, int x, int y, uint16_t color) {
    spr.drawCircle(x, y, 6, color);
    for (int i = 0; i < 360; i += 45) {
        float rad = i * 0.0174533f;
        spr.drawLine(x + cos(rad)*8,  y + sin(rad)*8, x + cos(rad)*11, y + sin(rad)*11, color);
    }
}

static void build_menu_sprite(TFT_eSprite& spr, AppState st, int bx, int by) {
    spr.pushImage(-bx, -by, 240, 280, assets_get_wallpaper());
    
    // Transparent Card for Steps?
    if (st != STATE_EXEC_STEPS) {
        spr.fillRoundRect(0, 0, MENU_BW, MENU_BH, MENU_RADIUS, BOX_COL);
        spr.drawRoundRect(0, 0, MENU_BW, MENU_BH, MENU_RADIUS, spr.color565(80, 80, 80));
    }

    const char* title = ""; const char* sub = ""; bool draws_sw = false;
    switch(st) {
        case STATE_MENU_TIMER:      title = "TIMER";      sub = "[HOLD TO SETUP]"; break;
        case STATE_MENU_HR:         title = "HEART RATE"; sub = "[HOLD TO START]"; break;
        case STATE_MENU_TIMEOUT:    title = "TIMEOUT";    sub = "[HOLD TO ADJUST]"; break;
        case STATE_MENU_AOD:        title = "AOD MODE";   sub = aod_allowed ? "[ON]" : "[OFF]"; break;
        case STATE_MENU_STOPWATCH:  title = "STOPWATCH";  sub = sw_running ? "[RUNNING]" : "[HOLD TO START]"; draws_sw = true; break;
        case STATE_MENU_BRIGHTNESS: title = "BRIGHTNESS";  sub = "[HOLD TO ADJUST]"; break;
        default: break;
    }
    spr.setTextDatum(MC_DATUM); spr.setTextColor(TFT_WHITE); spr.setTextSize(2);
    bool is_set = (st == STATE_MENU_TIMEOUT && current_state == STATE_SET_TIMEOUT);
    spr.drawString(title, MENU_BW/2, is_set ? 20 : MENU_BH/2 - 5);
    if (strlen(sub) > 0) { spr.setTextSize(1); spr.setTextColor(TFT_GOLD); spr.drawString(sub, MENU_BW/2, MENU_BH - 20); }
    if (st == STATE_MENU_BRIGHTNESS) draw_brightness_icon(spr, MENU_BW/2, 25, TFT_GOLD);
    if (draws_sw) {
        unsigned long t = sw_elapsed_ms; if (sw_running) t += (millis() - sw_start_ms);
        int s = (t / 1000) % 60; int m = (t / 60000) % 60;
        char buf[16]; sprintf(buf, "%02d:%02d", m, s);
        spr.setTextSize(1); spr.setTextColor(TFT_YELLOW); spr.drawString(buf, MENU_BW/2, 25);
    }
}

static void animate_slide_transition(AppState from, AppState to, int dir) {
    TFT_eSPI& tft = display_hal_get_tft();
    
    // [UI AGENT] SURGICAL GHOSTING FIX:
    // If moving from a full-screen dashboard (like Steps), wipe the top/bottom static areas first.
    if (from == STATE_EXEC_STEPS) {
        const uint16_t* wall = assets_get_wallpaper();
        tft.pushImage(0, 0, 240, 90, wall, 240); // Top slice with 240 stride
        tft.pushImage(0, 190, 240, 90, (uint16_t*)(wall + (190 * 240)), 240); // Bottom slice with 240 stride
    }

    // Testing 80MHz smoothness with optimized SPI (80MHz)...
    TFT_eSPI& tft_ref = display_hal_get_tft();
    TFT_eSprite combo = TFT_eSprite(&tft_ref);
    TFT_eSprite stemp = TFT_eSprite(&tft_ref);
    
    if (!combo.createSprite(240, MENU_BH)) return;
    if (!stemp.createSprite(MENU_BW, MENU_BH)) { combo.deleteSprite(); return; }
    
    combo.setSwapBytes(true); stemp.setSwapBytes(true);
    
    build_menu_sprite(menu_spr, from, MENU_BX, MENU_BY); // FROM (Global)
    build_menu_sprite(stemp, to, MENU_BX, MENU_BY);      // TO (Local)
    
    const uint16_t* wall = assets_get_wallpaper(); 
    static const int steps = 10; // Faster transition for 80MHz responsiveness
    
    for (int i = 0; i <= steps; i++) {
        int offset = (i * 240) / steps; 
        combo.pushImage(0, -90, 240, 280, wall); // Clean background slice
        
        if (dir > 0) { 
            menu_spr.pushToSprite(&combo, MENU_BX - offset, 0); 
            stemp.pushToSprite(&combo, MENU_BX + 240 - offset, 0); 
        } else { 
            menu_spr.pushToSprite(&combo, MENU_BX + offset, 0); 
            stemp.pushToSprite(&combo, MENU_BX - 240 + offset, 0); 
        }
        combo.pushSprite(0, 90);
    }
    
    stemp.deleteSprite(); combo.deleteSprite();
}

void ui_manager_update() {
    ButtonEvent bt = button_hal_read(); unsigned long now = millis();
    bool charging = power_manager_is_charging(); // [POWER AGENT] Track charging status
    
    // Auto-AOD on Charging
    if (charging) aod_allowed = true;

    // [POWER AGENT] Auto-Reset steps at Midnight (00:00)
    if (clock_h == 0 && clock_m == 0) {
        if (!mid_reset_done) { bmi160_hal_reset_steps(); mid_reset_done = true; }
    } else { mid_reset_done = false; }

    if (now - last_tick >= 60000) { clock_m++; if (clock_m >= 60) { clock_m = 0; clock_h++; } if (clock_h >= 24) clock_h = 0; last_tick = now; }

    bool nd = (current_state != last_rendered_state);
    if (bt != BTN_NONE) {
        last_activity_time = now;
        if (is_dimmed_aod) {
            is_dimmed_aod = false; 
            // SNAP wake-up
            display_hal_backlight_set(brightness_ui_val); 
            power_manager_set_freq(FREQ_MID); last_rendered_state = (AppState)-1; last_min_val = -1;
            bt = BTN_NONE; 
        }
    }

    uint32_t to_ms = power_manager_get_auto_sleep_timeout() * 1000;
    if (to_ms > 0 && (now - last_activity_time > to_ms)) {
        if (is_dimmed_aod) { }
        else if (current_state == (AppState)STATE_TIMER_ALARM) {} 
        else if (current_state == STATE_WATCHFACE) {
            if (aod_allowed) { is_dimmed_aod = true; display_hal_backlight_set(15); last_rendered_state = (AppState)-1; last_min_val = -1; }
            else { power_manager_enter_deep_sleep(); }
        } else if (current_state != STATE_EXEC_STOPWATCH && current_state != STATE_EXEC_TIMER && current_state != STATE_EXEC_HR) {
            if (!aod_allowed) power_manager_enter_deep_sleep();
            else { is_dimmed_aod = true; display_hal_backlight_set(15); current_state = STATE_WATCHFACE; nd = true; }
        }
    }

    if (now - last_fps_time >= 1000) { current_fps = draw_count; draw_count = 0; last_fps_time = now; }
    
    if (current_state == STATE_EXEC_HR) {
        max30100_hal_update();
        if (max30100_hal_check_beat()) nd = true; // Force re-draw on heartbeat
    }

    AppState target = current_state; int d = 0;
    if (bt == BTN_RIGHT_CLICK) {
        if (current_state == STATE_SET_BRIGHTNESS) { brightness_ui_val += 15; if (brightness_ui_val > 255) brightness_ui_val = 255; display_hal_backlight_set(brightness_ui_val); nd = true; }
        else if (current_state == STATE_SET_TIMEOUT) { uint32_t t = power_manager_get_auto_sleep_timeout(); power_manager_set_auto_sleep_timeout(t + 5); nd = true; }
        else if (current_state == STATE_SET_TIMER_H) { timer_h = (timer_h + 1) % 24; nd = true; }
        else if (current_state == STATE_SET_TIMER_M) { timer_m = (timer_m + 1) % 60; nd = true; }
        else if (current_state == STATE_SET_TIMER_S) { timer_s = (timer_s + 1) % 60; nd = true; }
        else if (current_state == STATE_EXEC_STOPWATCH) { if (sw_running) { sw_elapsed_ms += (now - sw_start_ms); sw_running = false; } else { sw_start_ms = now; sw_running = true; } nd = true; }
        else if (current_state != STATE_EXEC_HR && current_state != STATE_EXEC_TIMER && (int)current_state != STATE_TIMER_ALARM) {
            d = 1;
            if (current_state == STATE_WATCHFACE)       target = STATE_MENU_HR;
            else if (current_state == STATE_MENU_HR)    target = STATE_MENU_TIMEOUT;
            else if (current_state == STATE_MENU_TIMEOUT) target = STATE_MENU_AOD;
            else if (current_state == STATE_MENU_AOD)   target = STATE_MENU_TIMER;
            else if (current_state == STATE_MENU_TIMER) target = STATE_MENU_STOPWATCH;
            else if (current_state == STATE_MENU_STOPWATCH) target = STATE_MENU_BRIGHTNESS;
            else if (current_state == STATE_MENU_BRIGHTNESS) target = STATE_EXEC_STEPS;
            else if (current_state == STATE_EXEC_STEPS) target = STATE_WATCHFACE;
        }
    } else if (bt == BTN_LEFT_CLICK) {
        if (current_state == STATE_SET_TIMER_H) { timer_h--; if (timer_h < 0) timer_h = 23; nd = true; }
        else if (current_state == STATE_SET_TIMER_M) { timer_m--; if (timer_m < 0) timer_m = 59; nd = true; }
        else if (current_state == STATE_SET_TIMER_S) { timer_s--; if (timer_s < 0) timer_s = 59; nd = true; }
        else if ((int)current_state == STATE_TIMER_ALARM) { display_hal_backlight_set(brightness_ui_val); target = STATE_MENU_TIMER; } 
        else if (current_state == STATE_SET_BRIGHTNESS) { brightness_ui_val -= 15; if (brightness_ui_val < 0) brightness_ui_val = 0; display_hal_backlight_set(brightness_ui_val); nd = true; }
        else if (current_state == STATE_SET_TIMEOUT) { uint32_t t = power_manager_get_auto_sleep_timeout(); if (t > 5) power_manager_set_auto_sleep_timeout(t - 5); nd = true; }
        else if (current_state == STATE_EXEC_STOPWATCH) { sw_running = false; sw_elapsed_ms = 0; nd = true; }
        else if (current_state == STATE_EXEC_HR) { 
            max30100_hal_shutdown(); 
            target = STATE_MENU_HR; 
        }
        else if (current_state != STATE_EXEC_HR && current_state != STATE_EXEC_TIMER && (int)current_state != STATE_TIMER_ALARM) {
            d = -1;
            if (current_state == STATE_WATCHFACE)        target = STATE_EXEC_STEPS; // [UI AGENT] Left to Steps (1-click)
            else if (current_state == STATE_MENU_HR)     target = STATE_WATCHFACE;
            else if (current_state == STATE_MENU_TIMEOUT) target = STATE_MENU_HR;
            else if (current_state == STATE_MENU_AOD)    target = STATE_MENU_TIMEOUT;
            else if (current_state == STATE_MENU_TIMER)  target = STATE_MENU_AOD;
            else if (current_state == STATE_MENU_STOPWATCH) target = STATE_MENU_TIMER;
            else if (current_state == STATE_MENU_BRIGHTNESS) target = STATE_MENU_STOPWATCH;
            else if (current_state == STATE_EXEC_STEPS) target = STATE_MENU_BRIGHTNESS;
            else if (current_state == STATE_WATCHFACE)  target = STATE_EXEC_STEPS;
        }
    } else if (bt == BTN_RIGHT_DOUBLE) {
        if (current_state == STATE_SET_TIMER_H) { target = STATE_SET_TIMER_M; }
        else if (current_state == STATE_SET_TIMER_M) { target = STATE_SET_TIMER_S; }
        else if (current_state == STATE_SET_TIMER_S) { target = STATE_SET_TIMER_H; }
    } else if (bt == BTN_RIGHT_HOLD) {
        if      (current_state == STATE_WATCHFACE)  { bmi160_hal_reset_steps(); nd = true; } 
        else if (current_state == STATE_EXEC_STEPS) { bmi160_hal_reset_steps(); nd = true; } // Reset in dash
        else if (current_state == STATE_MENU_HR)    { if (max30100_hal_init()) { target = STATE_EXEC_HR; hr_start_time = millis(); nd = true; } }
        else if (current_state == STATE_EXEC_HR)    { max30100_hal_shutdown(); target = STATE_MENU_HR; }
        else if (current_state == STATE_MENU_TIMEOUT) target = STATE_SET_TIMEOUT;
        else if (current_state == STATE_SET_TIMEOUT)  target = STATE_MENU_TIMEOUT;
        else if (current_state == STATE_MENU_AOD)   { aod_allowed = !aod_allowed; nd = true; }
        else if (current_state == STATE_MENU_BRIGHTNESS) target = STATE_SET_BRIGHTNESS;
        else if (current_state == STATE_SET_BRIGHTNESS)  target = STATE_MENU_BRIGHTNESS;
        else if (current_state == STATE_MENU_TIMER)  { timer_h = 0; timer_m = 0; timer_s = 0; target = STATE_SET_TIMER_H; }
        else if (current_state >= STATE_SET_TIMER_H && current_state <= STATE_SET_TIMER_S) { if (timer_h+timer_m+timer_s > 0) { timer_target_ms = now + (timer_h*3600 + timer_m*60 + timer_s)*1000; timer_running = true; target = STATE_EXEC_TIMER; } }
        else if (current_state == STATE_EXEC_TIMER)  { timer_running = false; target = STATE_MENU_TIMER; }
        else if (current_state == STATE_MENU_STOPWATCH) { if (!sw_running) { sw_start_ms = now; sw_elapsed_ms = 0; sw_running = true; } target = STATE_EXEC_STOPWATCH; }
        else if (current_state == STATE_EXEC_STOPWATCH) { target = STATE_MENU_STOPWATCH; }
    } else if (bt == BTN_LEFT_HOLD) {
        if (current_state >= STATE_SET_TIMER_H && current_state <= STATE_EXEC_TIMER) target = STATE_MENU_TIMER;
        else if (current_state == (int)STATE_TIMER_ALARM) { display_hal_backlight_set(brightness_ui_val); target = STATE_MENU_TIMER; }
        else if (current_state == STATE_EXEC_STOPWATCH) target = STATE_MENU_STOPWATCH;
        else if (current_state == STATE_EXEC_HR) { max30100_hal_shutdown(); target = STATE_MENU_HR; }
        else power_manager_enter_deep_sleep();
    }

    if (target != current_state) {
        if (d != 0 && current_state != STATE_WATCHFACE && target != STATE_WATCHFACE) animate_slide_transition(current_state, target, d);
        current_state = target; nd = true;
        if (current_state >= STATE_SET_TIMER_H && current_state <= STATE_SET_TIMER_S) button_hal_set_double_click(true);
        else button_hal_set_double_click(false);
    }

    uint32_t iv = (current_state == STATE_EXEC_TIMER || current_state == STATE_EXEC_STOPWATCH || (int)current_state == STATE_TIMER_ALARM) ? 50 : 2000;
    if (current_state == STATE_EXEC_HR || current_state == STATE_SET_BRIGHTNESS || current_state == STATE_SET_TIMEOUT) iv = 100;
    if (current_state == STATE_WATCHFACE) iv = 500;
    if (current_state == STATE_EXEC_STEPS) iv = 100; // Fast UI for animation
    static unsigned long lr = 0; 
    if (!nd && (now - lr > iv)) {
        if (current_state == STATE_WATCHFACE) { uint8_t cb = get_battery_percentage(); if (cb != last_batt_val || clock_m != last_min_val) nd = true; }
        else if (clock_m != last_min_val || current_state == STATE_EXEC_TIMER || current_state == STATE_EXEC_STOPWATCH || (int)current_state == STATE_TIMER_ALARM || (current_state == STATE_MENU_STOPWATCH && sw_running)) nd = true; 
    }
    if (nd) { render_current_state(); last_rendered_state = current_state; lr = now; draw_count++; }
}

static void render_current_state() {
    TFT_eSPI& tft = display_hal_get_tft(); unsigned long now = millis();
    if (current_state == STATE_WATCHFACE) { draw_watchface(tft, (current_state != last_rendered_state)); return; }
    power_manager_set_freq(FREQ_MID);
    
    bool sjc = (current_state != last_rendered_state);
    bool from_full_black = (last_rendered_state == STATE_EXEC_HR || last_rendered_state == STATE_EXEC_TIMER || last_rendered_state == STATE_EXEC_STOPWATCH || (int)last_rendered_state == STATE_TIMER_ALARM || (last_rendered_state >= STATE_SET_TIMER_H && last_rendered_state <= STATE_SET_TIMER_S));
    bool is_full_black_mode = (current_state == STATE_EXEC_HR || current_state == STATE_EXEC_TIMER || current_state == STATE_EXEC_STOPWATCH || (int)current_state == STATE_TIMER_ALARM || (current_state >= STATE_SET_TIMER_H && current_state <= STATE_SET_TIMER_S));
    
    if (sjc && !is_full_black_mode) {
        if (from_full_black || last_rendered_state == STATE_WATCHFACE || last_rendered_state == (AppState)-1) {
            tft.pushImage(0, 0, 240, 280, assets_get_wallpaper()); push_top_clock(true);
        }
    }

    switch ((int)current_state) {
        case STATE_EXEC_HR: {
            if (last_rendered_state != STATE_EXEC_HR) { 
                tft.fillScreen(C_BG); 
                tft.setTextColor(C_MUTED); tft.setTextSize(2); 
                tft.drawString("HR", 16, 18);
            }
            
            // 1. PREMIUM BEATING HEART ANIMATION (TALLER SHAPE)
            static uint32_t last_bt_t = 0; 
            static float h_scale = 1.0;
            static uint8_t beat_phase = 0;

            if (max30100_hal_check_beat()) { last_bt_t = now; beat_phase = 1; }

            uint32_t elap_bt = now - last_bt_t;
            if (beat_phase == 1 && elap_bt < 100) h_scale = 1.0f + (elap_bt / 100.0f) * 0.30f; // Rapid grow
            else if (beat_phase == 1 && elap_bt < 200) h_scale = 1.30f - ((elap_bt - 100) / 100.0f) * 0.15f;
            else if (beat_phase == 1 && elap_bt < 300) h_scale = 1.15f + ((elap_bt - 200) / 100.0f) * 0.10f; // Second 'dub' kick
            else if (beat_phase == 1 && elap_bt < 450) h_scale = 1.25f - ((elap_bt - 300) / 150.0f) * 0.25f;
            else { h_scale = 1.0f; beat_phase = 0; }

            heart_spr.fillSprite(C_BG);
            draw_heart_icon(heart_spr, 16, 14, (beat_phase > 0) ? 0xC800 : C_DIM, h_scale); // Elegant Red: 0xC800
            heart_spr.pushSprite(104, 32);

            // 2. BPM VALUE (CENTERED)
            uint8_t bpm = (uint8_t)max30100_hal_get_bpm();
            if (bpm != last_bpm_v || last_rendered_state != STATE_EXEC_HR) {
                tft.fillRect(0, 65, 240, 50, C_BG);
                tft.setTextDatum(TC_DATUM); tft.setTextColor(TFT_WHITE); tft.setTextSize(4); 
                char buf[8]; sprintf(buf, "%d", bpm); 
                tft.drawString(buf, 120, 68);
                tft.setTextSize(1); tft.setTextColor(C_MUTED); 
                tft.drawString("BPM", 120, 108);
                tft.drawFastHLine(16, 122, 208, C_DIVIDER);
                last_bpm_v = bpm;
            }

            // 3. WAVEFORM GRAPH (10Hz UPDATE)
            static uint32_t last_g_upd = 0;
            if (now - last_g_upd > 100 || last_rendered_state != STATE_EXEC_HR) {
                graph_spr.fillSprite(C_BG);
                float hist[60]; max30100_hal_get_history(hist);
                for(int i=0; i < 59; i++) {
                    if (hist[i] > 30 && hist[i+1] > 30) {
                        int y1 = 80 - (int)((constrain(hist[i], 40, 180) - 40) / 140.0 * 75);
                        int y2 = 80 - (int)((constrain(hist[i+1], 40, 180) - 40) / 140.0 * 75);
                        graph_spr.drawLine(i*3.5, y1+1, i*3.5, 80, 0x6003); // Area
                        if (i < 58) graph_spr.drawLine(i*3.5+1, (y1+y2)/2 + 1, i*3.5+1, 80, 0x6003); 
                        graph_spr.drawLine(i*3.5, y1, (i+1)*3.5, y2, C_HR_RED); // Line
                    }
                }
                graph_spr.pushSprite(16, 124); 
                tft.drawFastHLine(16, 214, 208, C_DIVIDER);
                last_g_upd = now;
            }

            // 4. BOTTOM SPO2 & TIMER
            uint8_t spo2 = max30100_hal_get_spo2(); 
            uint32_t elap_s = (now - hr_start_time) / 1000;
            static uint32_t last_timer_v = 999;
            
            if (spo2 != last_spo2_v || elap_s != last_timer_v || last_rendered_state != STATE_EXEC_HR) {
                bottom_spr.fillSprite(C_BG);
                bottom_spr.setTextDatum(TL_DATUM); bottom_spr.setTextColor(C_MUTED); bottom_spr.setTextSize(1); 
                bottom_spr.drawString("SPO2", 16, 6);
                bottom_spr.setTextColor(C_SPO2); bottom_spr.setTextSize(3); 
                char sb[8]; sprintf(sb, "%d%%", spo2); bottom_spr.drawString(sb, 16, 20);
                
                bottom_spr.setTextDatum(TR_DATUM); bottom_spr.setTextColor(C_MUTED); bottom_spr.setTextSize(1); 
                bottom_spr.drawString("ELAPSED", 224, 6);
                bottom_spr.setTextColor(C_DIM); bottom_spr.setTextSize(2); 
                char tb[16]; sprintf(tb, "%d:%02d", elap_s/60, elap_s%60); 
                bottom_spr.drawString(tb, 224, 20);
                
                bottom_spr.pushSprite(0, 215); 
                last_spo2_v = spo2;
                last_timer_v = elap_s;
            }
            return;
        }
        case STATE_SET_TIMER_H: case STATE_SET_TIMER_M: case STATE_SET_TIMER_S:
            if (last_rendered_state < STATE_SET_TIMER_H || last_rendered_state > STATE_SET_TIMER_S) tft.fillScreen(TFT_BLACK);
            tft.setTextDatum(MC_DATUM); tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.setTextSize(2); tft.drawString("TIMER SETUP", 120, 30);
            { char buf[32]; sprintf(buf, "%02d:%02d:%02d", timer_h, timer_m, timer_s); tft.setTextSize(4); tft.setTextPadding(220); tft.drawString(buf, 120, 100); }
            tft.setTextSize(1); tft.setTextColor(TFT_GOLD, TFT_BLACK); 
            tft.fillRect(0, 125, 240, 15, TFT_BLACK); 
            if (current_state == STATE_SET_TIMER_H) tft.drawString("^^", 120-56, 130); 
            else if (current_state == STATE_SET_TIMER_M) tft.drawString("^^", 120, 130); 
            else tft.drawString("^^", 120+56, 130);
            tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.drawString("R: + | L: - | R-DBL: SWITCH", 120, 180);
            tft.drawString("R-HOLD: START | L-HOLD: EXIT", 120, 205); return;
        case STATE_EXEC_TIMER:
            if (last_rendered_state != STATE_EXEC_TIMER) tft.fillScreen(TFT_BLACK);
            { long diff = (long)timer_target_ms - (long)now; if (diff <= 0) { current_state = (AppState)STATE_TIMER_ALARM; return; }
              int s = (diff / 1000) % 60; int m = (diff / 60000) % 60; int h = (diff / 3600000);
              tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.setTextSize(5); tft.setTextDatum(MC_DATUM); tft.setTextPadding(240);
              char buf[16]; sprintf(buf, "%02d:%02d:%02d", h, m, s); tft.drawString(buf, 120, 140);
              tft.setTextSize(2); tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString("TIMER RUNNING", 120, 60);
              tft.setTextSize(1); tft.setTextColor(TFT_DARKGREY, TFT_BLACK); tft.drawString("HOLD LEFT: CANCEL", 120, 220);
            } return;
        case STATE_TIMER_ALARM:
            if (now - last_alarm_flip > 500) {
                alarm_color_idx = (alarm_color_idx + 1) % 3; uint16_t cols[] = {TFT_WHITE, TFT_RED, TFT_BLUE};
                display_hal_backlight_set(255); tft.fillScreen(cols[alarm_color_idx]);
                tft.setTextColor(TFT_BLACK); tft.setTextSize(4); tft.setTextDatum(MC_DATUM); tft.drawString("TIME UP!", 120, 120);
                tft.setTextSize(2); tft.drawString("CLICK LEFT TO STOP", 120, 200); last_alarm_flip = now;
            } return;
        case STATE_EXEC_STOPWATCH:
            if (last_rendered_state != STATE_EXEC_STOPWATCH) { tft.fillScreen(TFT_BLACK); tft.setTextDatum(MC_DATUM); tft.setTextSize(2); tft.setTextColor(TFT_WHITE); tft.drawString("STOPWATCH", 120, 70); tft.setTextSize(1); tft.setTextColor(TFT_DARKGREY); tft.drawString("R:START/STOP | L:RESET", 120, 200); }
            { unsigned long t = sw_elapsed_ms; if (sw_running) t += (now - sw_start_ms);
              int ms = (t % 1000) / 10; int s = (t / 1000) % 60; int m = (t / 60000) % 60;
              tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.setTextSize(5); tft.setTextDatum(MC_DATUM); tft.setTextPadding(220);
              char buf[16]; sprintf(buf, "%02d:%02d.%02d", m, s, ms); tft.drawString(buf, 120, 140);
            } return;
        case STATE_SET_TIMEOUT:
            if (last_rendered_state != STATE_SET_TIMEOUT) build_menu_sprite(menu_spr, STATE_MENU_TIMEOUT, MENU_BX, MENU_BY);
            else { menu_spr.fillRect(40, 45, 120, 30, BOX_COL); } 
            menu_spr.setTextColor(TFT_GOLD); menu_spr.setTextSize(2); menu_spr.drawString("-", 30, 60); menu_spr.drawString("+", 170, 60);
            { char buf[16]; sprintf(buf, "%d s", power_manager_get_auto_sleep_timeout()); menu_spr.setTextColor(TFT_WHITE); menu_spr.drawString(buf, MENU_BW/2, 60); }
            menu_spr.pushSprite(MENU_BX, MENU_BY); break;
        case STATE_SET_BRIGHTNESS:
            build_menu_sprite(menu_spr, STATE_MENU_BRIGHTNESS, MENU_BX, MENU_BY);
            { int bar_w = (brightness_ui_val * 160) / 255; menu_spr.drawRoundRect(20, 75, 160, 12, 6, TFT_WHITE); menu_spr.fillRect(22, 77, 156, 8, BOX_COL); menu_spr.fillRoundRect(22, 77, bar_w > 4 ? bar_w - 4 : 0, 8, 4, TFT_GREEN); }
            menu_spr.pushSprite(MENU_BX, MENU_BY); break;
        case STATE_EXEC_STEPS:
            {
                if (last_rendered_state != STATE_EXEC_STEPS) {
                    tft.pushImage(0, 0, 240, 280, assets_get_wallpaper());
                    push_top_clock(true);
                }
                
                uint32_t steps = bmi160_hal_get_steps();
                uint32_t goal = 10000;
                float ratio = constrain(steps / (float)goal, 0.0f, 1.0f);
                
                tft.setTextDatum(MC_DATUM); tft.setTextColor(TFT_WHITE); tft.setTextSize(2); 
                tft.drawString("ACTIVITY", 120, 30);
                
                // Dashboard Center (Large Ring)
                int rx = 120, ry = 140;
                tft.drawCircle(rx, ry, 70, 0x18C3); 
                
                float end_angle = (ratio * 360.0f) - 90.0f;
                for (float a = -90.0f; a < end_angle; a += 1.5f) {
                    float rad = a * 0.0174533f;
                    uint16_t arc_col = tft.color565(127 + (int)(ratio * 85), 119 - (int)(ratio * 36), 221 - (int)(ratio * 95));
                    tft.fillCircle(rx + cos(rad)*70, ry + sin(rad)*70, 7, arc_col);
                }

                tft.setTextSize(4); tft.setTextColor(TFT_WHITE);
                char s_buf[16]; if (steps > 999) sprintf(s_buf, "%d,%03d", steps/1000, steps%1000); else sprintf(s_buf, "%d", steps);
                
                // [UI AGENT] GHOSTING FIX: Wipe text area before re-drawing
                const uint16_t* wall = assets_get_wallpaper();
                tft.pushImage(40, 110, 160, 50, (uint16_t*)(wall + (110 * 240) + 40), 240); 
                
                tft.drawString(s_buf, rx, ry - 10);
                tft.setTextSize(1); tft.setTextColor(C_MUTED); 
                tft.drawString("STEPS", rx, ry + 25);
                tft.setTextColor(0x7BDD); tft.drawString("GOAL 10K", rx, ry + 40);

                // Stats Bottom
                tft.drawFastHLine(20, 240, 200, C_DIVIDER);
                tft.setTextDatum(TL_DATUM); tft.setTextSize(1); tft.setTextColor(TFT_WHITE);
                sprintf(s_buf, "%d KCAL", (int)(steps * 0.044f)); tft.drawString(s_buf, 30, 250);
                tft.setTextDatum(TR_DATUM);
                sprintf(s_buf, "%.1f KM", steps * 0.00075f); tft.drawString(s_buf, 210, 250);

                // Animated Stickman (at bottom)
                float bounce = sin(millis() * 0.008f) * 2.0f;
                int sx = 120, sy = 250 + (int)bounce;
                tft.fillCircle(sx, sy-6, 2, 0x7BDD);
                tft.drawLine(sx, sy-4, sx, sy+2, 0x7BDD);
                tft.drawLine(sx-3, sy-2, sx+3, sy-2, 0x7BDD);
                tft.drawLine(sx, sy+2, sx-3, sy+6, 0x7BDD);
                tft.drawLine(sx, sy+2, sx+3, sy+6, 0x7BDD);
            }
            break;
        default:
            build_menu_sprite(menu_spr, current_state, MENU_BX, MENU_BY); menu_spr.pushSprite(MENU_BX, MENU_BY); break;
    }
}

static void draw_lightning_bolt(TFT_eSprite& spr, int x, int y, uint16_t color) {
    // Elegant small lightning bolt
    spr.drawLine(x+2, y, x, y+5, color);
    spr.drawLine(x, y+5, x+4, y+4, color);
    spr.drawLine(x+4, y+4, x+2, y+10, color);
}

static void draw_watchface(TFT_eSPI& tft, bool full_redraw) {
    if (!is_dimmed_aod) power_manager_set_freq(FREQ_MID);
    const uint16_t* wall = assets_get_wallpaper();
    if (full_redraw) { if (is_dimmed_aod) tft.fillScreen(TFT_BLACK); else tft.pushImage(0, 0, 240, 280, wall); }
    int cx = 30, cy = 60; if (is_dimmed_aod) clock_spr.fillSprite(TFT_BLACK); else clock_spr.pushImage(-cx, -cy, 240, 280, wall);
    char buf[8]; sprintf(buf, "%02d", clock_h); clock_spr.setTextDatum(MC_DATUM); clock_spr.setTextColor(COLOR_WATCH_HH); clock_spr.setTextSize(8); clock_spr.drawString(buf, 90, 35);
    sprintf(buf, "%02d", clock_m); clock_spr.setTextColor(COLOR_WATCH_MM); clock_spr.drawString(buf, 90, 105);
    if (!is_dimmed_aod) { clock_spr.setTextColor(COLOR_WATCH_DATE); clock_spr.setTextSize(2); clock_spr.drawString("TUE 17 MAR", 90, 145); }
    clock_spr.pushSprite(cx, cy); uint8_t batt = get_battery_percentage(); int sx = 165, sy = 15;
    bool charging = power_manager_is_charging();
    if (is_dimmed_aod) status_spr.fillSprite(TFT_BLACK); else status_spr.pushImage(-sx, -sy, 240, 280, wall);
    status_spr.pushImage(45, 5, 16, 16, assets_get_icon_battery()); 
    int fill_w = (batt * 10) / 100; uint16_t b_col = (batt < 20) ? TFT_RED : TFT_GREEN;
    if (charging) { 
        b_col = TFT_CYAN; 
        draw_lightning_bolt(status_spr, 36, 8, TFT_YELLOW); // Yellow Bolt!
    }
    status_spr.fillRect(47, 8, fill_w, 10, b_col);
    status_spr.setTextDatum(TR_DATUM); status_spr.setTextColor(charging ? TFT_CYAN : TFT_WHITE); status_spr.setTextSize(1); 
    char b_str[8]; sprintf(b_str, "%d", batt); 
    status_spr.drawString(b_str, 35, 9); status_spr.pushSprite(sx, sy);
    last_batt_val = batt; last_min_val = clock_m;
}

static uint8_t get_battery_percentage() { float v = power_manager_read_battery_voltage(); if (v >= 4.10) return 100; if (v <= 3.35) return 0; return (uint8_t)((v - 3.40) / (4.20 - 3.40) * 100); }
bool ui_manager_is_aod_enabled() { return aod_allowed; }

// [UI AGENT] Premium Taller Heart Icon
static void draw_heart_icon(TFT_eSPI &tft, int cx, int cy, uint16_t color, float scale) {
    int r = (int)(6 * scale); 
    int cy_adj = cy - (int)(3 * scale); // Nudged up slightly for fuller base
    tft.fillCircle(cx - r, cy_adj, r, color);
    tft.fillCircle(cx + r, cy_adj, r, color);
    // Fuller triangle tip (less sharp)
    tft.fillTriangle(cx - r*2, cy_adj, cx + r*2, cy_adj, cx, cy + (int)(13 * scale), color);
}
