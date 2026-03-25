#include "ui_manager.h"
#include "display_hal.h"
#include "button_hal.h"
#include "max30100_hal.h"
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
static unsigned long last_tick = 0;

// Sprites
static TFT_eSprite clock_spr  = TFT_eSprite(&display_hal_get_tft());
static TFT_eSprite status_spr = TFT_eSprite(&display_hal_get_tft());
static TFT_eSprite fps_spr    = TFT_eSprite(&display_hal_get_tft());
static TFT_eSprite top_clock_spr = TFT_eSprite(&display_hal_get_tft());
static TFT_eSprite menu_spr   = TFT_eSprite(&display_hal_get_tft()); 

// Helpers
static void render_current_state();
static uint8_t get_battery_percentage();
static void draw_watchface(TFT_eSPI& tft, bool full_redraw);
static void push_top_clock(bool force = false);
static void animate_slide_transition(AppState from, AppState to, int dir);
static void build_menu_sprite(TFT_eSprite& spr, AppState st, int bx, int by);
static void push_wallpaper_rect(int x, int y, int w, int h);
static void draw_brightness_icon(TFT_eSprite& spr, int x, int y, uint16_t color);

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
    
    // UI SNAP-ON Strategy Restoration:
    display_hal_display_off(); // Keep the mask on (Zero-Garbage)
    display_hal_backlight_set(0); 
    
    // Double scrubbing VRAM
    render_current_state(); 
    delay(50);
    render_current_state();
    delay(100); 

    display_hal_display_on(); // Unmask stable image
    delay(20);

    // SNAP! Instant turn on to hiding hardware start noise
    display_hal_backlight_set(brightness_ui_val);
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
    spr.fillRoundRect(0, 0, MENU_BW, MENU_BH, MENU_RADIUS, BOX_COL);
    spr.drawRoundRect(0, 0, MENU_BW, MENU_BH, MENU_RADIUS, spr.color565(80, 80, 80));
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
    power_manager_set_freq(FREQ_HIGH);
    TFT_eSprite combo = TFT_eSprite(&display_hal_get_tft());
    TFT_eSprite s1    = TFT_eSprite(&display_hal_get_tft());
    TFT_eSprite s2    = TFT_eSprite(&display_hal_get_tft());
    combo.createSprite(240, MENU_BH); combo.setSwapBytes(true);
    s1.createSprite(MENU_BW, MENU_BH); s1.setSwapBytes(true);
    s2.createSprite(MENU_BW, MENU_BH); s2.setSwapBytes(true);
    build_menu_sprite(s1, from, MENU_BX, MENU_BY); build_menu_sprite(s2, to, MENU_BX, MENU_BY);
    const uint16_t* wall = assets_get_wallpaper(); static const int steps = 14;
    for (int i = 0; i <= steps; i++) {
        int offset = (i * 240) / steps; combo.pushImage(0, -MENU_BY, 240, 280, wall);
        if (dir > 0) { s1.pushToSprite(&combo, MENU_BX-offset, 0); s2.pushToSprite(&combo, MENU_BX+240-offset, 0); }
        else { s1.pushToSprite(&combo, MENU_BX+offset, 0); s2.pushToSprite(&combo, MENU_BX-240+offset, 0); }
        combo.pushSprite(0, MENU_BY);
    }
    s1.deleteSprite(); s2.deleteSprite(); combo.deleteSprite(); power_manager_set_freq(FREQ_MID);
}

void ui_manager_update() {
    ButtonEvent bt = button_hal_read(); unsigned long now = millis();
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
        } else if (current_state != STATE_EXEC_STOPWATCH && current_state != STATE_EXEC_TIMER) {
            if (!aod_allowed) power_manager_enter_deep_sleep();
            else { is_dimmed_aod = true; display_hal_backlight_set(15); current_state = STATE_WATCHFACE; nd = true; }
        }
    }

    if (now - last_fps_time >= 1000) { current_fps = draw_count; draw_count = 0; last_fps_time = now; }
    if (current_state == STATE_EXEC_HR) max30100_hal_update();

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
            else if (current_state == STATE_MENU_BRIGHTNESS) target = STATE_WATCHFACE;
        }
    } else if (bt == BTN_LEFT_CLICK) {
        if (current_state == STATE_SET_TIMER_H) { timer_h--; if (timer_h < 0) timer_h = 23; nd = true; }
        else if (current_state == STATE_SET_TIMER_M) { timer_m--; if (timer_m < 0) timer_m = 59; nd = true; }
        else if (current_state == STATE_SET_TIMER_S) { timer_s--; if (timer_s < 0) timer_s = 59; nd = true; }
        else if ((int)current_state == STATE_TIMER_ALARM) { display_hal_backlight_set(brightness_ui_val); target = STATE_MENU_TIMER; } 
        else if (current_state == STATE_SET_BRIGHTNESS) { brightness_ui_val -= 15; if (brightness_ui_val < 0) brightness_ui_val = 0; display_hal_backlight_set(brightness_ui_val); nd = true; }
        else if (current_state == STATE_SET_TIMEOUT) { uint32_t t = power_manager_get_auto_sleep_timeout(); if (t > 5) power_manager_set_auto_sleep_timeout(t - 5); nd = true; }
        else if (current_state == STATE_EXEC_STOPWATCH) { sw_running = false; sw_elapsed_ms = 0; nd = true; }
        else if (current_state != STATE_EXEC_HR && current_state != STATE_EXEC_TIMER) {
            d = -1;
            if (current_state == STATE_WATCHFACE)        target = STATE_MENU_BRIGHTNESS;
            else if (current_state == STATE_MENU_HR)     target = STATE_WATCHFACE;
            else if (current_state == STATE_MENU_TIMEOUT) target = STATE_MENU_HR;
            else if (current_state == STATE_MENU_AOD)    target = STATE_MENU_TIMEOUT;
            else if (current_state == STATE_MENU_TIMER)  target = STATE_MENU_AOD;
            else if (current_state == STATE_MENU_STOPWATCH) target = STATE_MENU_TIMER;
            else if (current_state == STATE_MENU_BRIGHTNESS) target = STATE_MENU_STOPWATCH;
        }
    } else if (bt == BTN_RIGHT_DOUBLE) {
        if (current_state == STATE_SET_TIMER_H) { target = STATE_SET_TIMER_M; }
        else if (current_state == STATE_SET_TIMER_M) { target = STATE_SET_TIMER_S; }
        else if (current_state == STATE_SET_TIMER_S) { target = STATE_SET_TIMER_H; }
    } else if (bt == BTN_RIGHT_HOLD) {
        if      (current_state == STATE_MENU_HR)    { if (max30100_hal_init()) target = STATE_EXEC_HR; }
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
        case STATE_EXEC_HR:
            if (last_rendered_state != STATE_EXEC_HR) { tft.fillScreen(TFT_BLACK); tft.setTextColor(TFT_RED, TFT_BLACK); tft.setTextSize(2); tft.setTextDatum(MC_DATUM); tft.drawString("Measuring...", 120, 80); }
            tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.setTextSize(6); tft.setTextPadding(140); { char buf[16]; sprintf(buf, "%.0f", max30100_hal_get_bpm()); tft.drawString(buf, 120, 150); }
            tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.setTextSize(2); tft.setTextPadding(100); { char buf[16]; sprintf(buf, "SpO2: %d%%", max30100_hal_get_spo2()); tft.drawString(buf, 120, 220); } return;
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
        default:
            build_menu_sprite(menu_spr, current_state, MENU_BX, MENU_BY); menu_spr.pushSprite(MENU_BX, MENU_BY); break;
    }
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
    if (is_dimmed_aod) status_spr.fillSprite(TFT_BLACK); else status_spr.pushImage(-sx, -sy, 240, 280, wall);
    status_spr.pushImage(45, 5, 16, 16, assets_get_icon_battery()); int fill_w = (batt * 10) / 100; status_spr.fillRect(47, 8, fill_w, 10, (batt < 20) ? TFT_RED : TFT_GREEN);
    status_spr.setTextDatum(TR_DATUM); status_spr.setTextColor(TFT_WHITE); status_spr.setTextSize(1); status_spr.drawNumber(batt, 42, 9); status_spr.pushSprite(sx, sy);
    last_batt_val = batt; last_min_val = clock_m;
}

static uint8_t get_battery_percentage() { float v = power_manager_read_battery_voltage(); if (v >= 4.10) return 100; if (v <= 3.35) return 0; return (uint8_t)((v - 3.40) / (4.20 - 3.40) * 100); }
bool ui_manager_is_aod_enabled() { return aod_allowed; }
