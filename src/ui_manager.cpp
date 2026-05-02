#include "ui_manager.h"
#include "display_hal.h"
#include "button_hal.h"
#include "max30100_hal.h"
#include "bmi160_hal.h"
#include "power_manager.h"
#include "app_config.h"
#include "assets_wallpaper.h"
#include "assets_icons.h"
#include <sys/time.h>
#include <time.h>
#include "ble_hal.h" // [GUARD] Synchronous Data Reporting
#include "calibration_manager.h"

// States extension (Internal)
#define STATE_TIMER_ALARM 99 

// Persist - RTC_DATA_ATTR
static RTC_DATA_ATTR bool aod_allowed = false; 
static RTC_DATA_ATTR bool aod_timed_mode = false; 
static RTC_DATA_ATTR uint32_t aod_duration_sec = 30;
static RTC_DATA_ATTR int brightness_aod_val = 50;
static RTC_DATA_ATTR int brightness_ui_val = 127; 
RTC_DATA_ATTR AppState current_state = STATE_WATCHFACE;

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
static RTC_DATA_ATTR int timer_h = 0, timer_m = 0, timer_s = 0;
static RTC_DATA_ATTR unsigned long timer_target_ms = 0;
static RTC_DATA_ATTR bool timer_running = false;
static uint8_t alarm_color_idx = 0;
static unsigned long last_alarm_flip = 0;

// Stopwatch Vars
static RTC_DATA_ATTR unsigned long sw_start_ms = 0;
static RTC_DATA_ATTR unsigned long sw_elapsed_ms = 0;
static RTC_DATA_ATTR bool sw_running = false;

// Runtime
bool ui_is_high_load = false; // [DISPLAY AGENT] High CPU/RAM load flag
bool ble_is_syncing = false;
bool ble_is_connected = false; // [DRIVER AGENT TARGET] Connection established flag
static uint16_t ui_cpu_freq = FREQ_MID; // [UI AGENT] Track current speed
static bool is_dimmed_aod = false; 
static unsigned long last_activity_time = 0;
static AppState last_rendered_state = (AppState)-1;
static float current_fps = 0;
static uint32_t draw_count = 0;
static uint32_t last_fps_time = 0;
static uint32_t boot_stabilize_counter = 0; // [UI AGENT] Cold Boot Guard
static int settings_menu_idx = 0; // [UI AGENT] Settings menu selected index
static int sub_menu_idx = 0;

// Time
static RTC_DATA_ATTR int clock_h = 10, clock_m = 10, clock_s = 0;
static RTC_DATA_ATTR bool clock_initialized = false;
static int last_min_val = -1;
static uint8_t last_batt_val = 255;
static RTC_DATA_ATTR int last_reset_yday = -1; 

// Sprites
static TFT_eSprite canvas_spr = TFT_eSprite(&display_hal_get_tft()); // [UI AGENT] Unified Shared Canvas (USA)
static TFT_eSprite status_spr = TFT_eSprite(&display_hal_get_tft());
static TFT_eSprite fps_spr    = TFT_eSprite(&display_hal_get_tft());
static TFT_eSprite top_clock_spr = TFT_eSprite(&display_hal_get_tft());
static TFT_eSprite heart_spr  = TFT_eSprite(&display_hal_get_tft()); // Small animation sprite (kept separate)

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
static void push_sub_sprite(int tx, int ty, int w, int h);

// ─── CONSTANTS ──────────────────────────────────────────────────────────
static const int MENU_BX = 20, MENU_BY = 90, MENU_BW = 200, MENU_BH = 100;
static const int MENU_RADIUS = 15;
static const uint16_t BOX_COL = 0x18C3; 

// ─── INIT ────────────────────────────────────────────────────────────────
void ui_manager_init() {
    last_activity_time = millis(); 
    
    calibration_manager_init();
    power_manager_set_auto_sleep_timeout(calibration_get_screen_timeout());

    // [POWER AGENT] Smart Throttling Intuition
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause == ESP_SLEEP_WAKEUP_UNDEFINED) {
        boot_stabilize_counter = 0; // Cold boot: Enable CDC guard
        if (Serial) Serial.println("SYSTEM: COLD BOOT (Manual RE-INJECT v7.2) - Stay High 160MHz for CDC... // [SAFE]");
    } else {
        boot_stabilize_counter = 300; // Regular wakeup: Skip guard, stay at 80MHz. 
        if (Serial) Serial.println("SYSTEM: WAKEUP - Bypassing CDC Guard, starting at 80MHz... // [STABLE]");
    }

    // [UI AGENT] SCRUB: Delete sprites to prevent leaks before re-creating
    if (canvas_spr.created()) canvas_spr.deleteSprite();
    if (status_spr.created()) status_spr.deleteSprite();
    if (fps_spr.created())    fps_spr.deleteSprite();
    if (top_clock_spr.created()) top_clock_spr.deleteSprite();
    if (heart_spr.created())  heart_spr.deleteSprite();

    // [UI AGENT] INSANE COMPOSITOR (Fixed Maximum Size 240x160 to prevent Heap Fragmentation)
    canvas_spr.createSprite(240, 160); canvas_spr.setSwapBytes(true); 
    status_spr.createSprite(65, 25);  status_spr.setSwapBytes(true);
    fps_spr.createSprite(35, 15);     fps_spr.setSwapBytes(true);
    top_clock_spr.createSprite(60, 15); top_clock_spr.setSwapBytes(true);
    heart_spr.createSprite(32, 32);  heart_spr.setSwapBytes(true); 
    
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

    // [POWER AGENT] RTC System Sync: Set start time if cold boot
    if (!clock_initialized) {
        struct timeval tv = { .tv_sec = (clock_h * 3600) + (clock_m * 60), .tv_usec = 0 };
        settimeofday(&tv, NULL);
        clock_initialized = true;
    }
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

static void push_sub_sprite(int tx, int ty, int w, int h) {
    canvas_spr.pushSprite(tx, ty, 0, 0, w, h);
}

static void draw_brightness_icon(TFT_eSprite& spr, int x, int y, uint16_t color) {
    spr.drawCircle(x, y, 6, color);
    for (int i = 0; i < 360; i += 45) {
        float rad = i * 0.0174533f;
        spr.drawLine(x + cos(rad)*8,  y + sin(rad)*8, x + cos(rad)*11, y + sin(rad)*11, color);
    }
}

// [UI AGENT] New Unified Drawing Logic: Draws a menu card directly over an existing sprite
static void draw_menu_card(TFT_eSprite& spr, AppState st, int x, int y) {
    // 1. Card Body
    if (st != STATE_EXEC_STEPS) {
        spr.fillRoundRect(x, y, MENU_BW, MENU_BH, MENU_RADIUS, BOX_COL);
        spr.drawRoundRect(x, y, MENU_BW, MENU_BH, MENU_RADIUS, spr.color565(80, 80, 80));
    }

    const char* title = ""; const char* sub = ""; bool draws_sw = false;
    switch(st) {
        case STATE_MENU_TIMER:      title = "TIMER";      sub = "[HOLD TO SETUP]"; break;
        case STATE_MENU_HR:         title = "HEART RATE"; sub = "[HOLD TO START]"; break;
        case STATE_MENU_TIMEOUT:    title = "TIMEOUT";    sub = "[HOLD TO ADJUST]"; break;
        case STATE_MENU_AOD:        title = "AOD MODE";   sub = aod_allowed ? "[ON]" : "[OFF]"; break;
        case STATE_MENU_STOPWATCH:  title = "STOPWATCH";  sub = sw_running ? "[RUNNING]" : "[HOLD TO START]"; draws_sw = true; break;
        case STATE_MENU_BRIGHTNESS: title = "BRIGHTNESS";  sub = "[HOLD TO ADJUST]"; break;
        case STATE_MENU_SYNC: {
            if (ble_is_connected) {
                title = "CONNECTED"; sub = "[SYNC SUCCESS]";
                spr.fillCircle(x + MENU_BW/2, y + 25, 8, spr.color565(0, 255, 100)); // Success Green Dot
            } else if (ble_is_syncing) {
                title = "SYNCING..."; sub = "[TRANSFERRING...]";
                float p = ble_hal_get_sync_progress();
                int pw = (int)(p * (MENU_BW - 40));
                spr.fillRect(x + 20, y + 75, pw, 6, spr.color565(0, 180, 255)); // Sky Blue Progress
                spr.drawRect(x + 20, y + 75, MENU_BW - 40, 6, C_DIM);
            } else {
                title = "CONNECTIVITY"; sub = "[HOLD TO PAIR]";
            }
            break;
        }
        case STATE_MENU_SETTINGS:   title = "SETTINGS";   sub = "[HOLD TO ENTER]"; break;
        case STATE_EXEC_STEPS:      title = "ACTIVITY";   sub = "[STEPS]"; break; 
        default: break;
    }
    spr.setTextDatum(MC_DATUM); spr.setTextColor(TFT_WHITE); spr.setTextSize(2);
    bool is_set = (st == STATE_MENU_TIMEOUT && current_state == STATE_SET_TIMEOUT);
    
    if (st == STATE_EXEC_STEPS) {
        spr.drawCircle(x + MENU_BW/2, y + MENU_BH/2 + 20, 35, 0x18C3); 
        spr.drawString(title, x + MENU_BW/2, y + 15);
    } else if (strlen(title) > 0) {
        spr.drawString(title, x + MENU_BW/2, y + (is_set ? 20 : MENU_BH/2 - 5));
    }
    
    if (strlen(sub) > 0 && st != STATE_EXEC_STEPS) { spr.setTextSize(1); spr.setTextColor(TFT_GOLD); spr.drawString(sub, x + MENU_BW/2, y + MENU_BH - 20); }
    if (st == STATE_MENU_BRIGHTNESS) draw_brightness_icon(spr, x + MENU_BW/2, y + 25, TFT_GOLD);
    if (draws_sw) {
        unsigned long t = sw_elapsed_ms; if (sw_running) t += (millis() - sw_start_ms);
        int s = (t / 1000) % 60; int m = (t / 60000) % 60;
        char buf[16]; sprintf(buf, "%02d:%02d", m, s);
        spr.setTextSize(1); spr.setTextColor(TFT_YELLOW); spr.drawString(buf, x + MENU_BW/2, y + 25);
    }
}

static void animate_slide_transition(AppState from, AppState to, int dir) {
    ui_is_high_load = true; 
    TFT_eSPI& tft = display_hal_get_tft();
    
    // [UI AGENT] Ensure 160MHz during animation for butter-smooth transition
    power_manager_set_freq(FREQ_HIGH); ui_cpu_freq = FREQ_HIGH;
    
    if (from == STATE_EXEC_STEPS) {
        const uint16_t* wall = assets_get_wallpaper();
        tft.pushImage(0, 0, 240, 90, wall, 240); 
        tft.pushImage(0, 190, 240, 90, (uint16_t*)(wall + (190 * 240)), 240); 
    }

    const uint16_t* wall = assets_get_wallpaper(); 
    static const int steps = 10; 
    
    int target_h = (current_state == STATE_EXEC_STEPS) ? 160 : 
                   ((current_state == STATE_SYNCING) ? 120 : MENU_BH);

    for (int i = 0; i <= steps; i++) {
        int offset = (i * 240) / steps; 
        canvas_spr.pushImage(0, -90, 240, 280, wall); 
        
        if (dir > 0) { 
            draw_menu_card(canvas_spr, from, MENU_BX - offset, 0);
            draw_menu_card(canvas_spr, to, MENU_BX + 240 - offset, 0);
        } else { 
            draw_menu_card(canvas_spr, from, MENU_BX + offset, 0);
            draw_menu_card(canvas_spr, to, MENU_BX - 240 + offset, 0);
        }
        push_sub_sprite(0, 90, 240, target_h);
        yield(); // [BUG FIX] Relieve Watchdog and ensure smooth rendering
    }
    ui_is_high_load = false;
}

void ui_manager_update() {
    ButtonEvent bt = button_hal_read(); unsigned long now = millis();
    bool charging = power_manager_is_charging(); // [POWER AGENT] Track charging status
    
    // Auto-AOD on Charging
    if (charging) aod_allowed = true;

    // [POWER AGENT] Sync Local Vars with Hardware RTC
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    time_t raw_time = tv_now.tv_sec;
    struct tm* ti = localtime(&raw_time);
    clock_h = ti->tm_hour;
    clock_m = ti->tm_min;
    clock_s = ti->tm_sec;
    
    // [POWER AGENT] Auto-Reset steps on Day Change (Midnight Fix)
    if (last_reset_yday != ti->tm_yday) {
        // Reset steps only if time is synced (tm_year > 120 means > 2020)
        if (ti->tm_year > 120) {
            bmi160_hal_reset_steps();
            last_reset_yday = ti->tm_yday;
            if (Serial) Serial.printf("[POWER] New Day Detected (%d), Steps Reset // [DEBUG]\n", ti->tm_yday);
        } else {
            // If not synced, just update the yday to prevent massive reset on first sync
            last_reset_yday = ti->tm_yday;
        }
    }
    
    bool nd = (current_state != last_rendered_state);
    
    // [UI AGENT] Real-time Status Watcher for Bluetooth Events
    static bool last_ble_connected = false;
    static bool last_ble_syncing = false;
    if (ble_is_connected != last_ble_connected || ble_is_syncing != last_ble_syncing) {
        nd = true;
        last_ble_connected = ble_is_connected;
        last_ble_syncing = ble_is_syncing;
        if (Serial) Serial.println("UI: Status Change detected // [REFRESH]");
    }
    if (bt != BTN_NONE) {
        if (Serial) Serial.printf("UI: Button Event %d detected // [DEBUG]\n", (int)bt);
        last_activity_time = now;
        if (is_dimmed_aod) {
            is_dimmed_aod = false; 
            display_hal_backlight_set(brightness_ui_val); 
            power_manager_set_freq(FREQ_MID); ui_cpu_freq = FREQ_MID;
            last_rendered_state = (AppState)-1; last_min_val = -1;
            bt = BTN_NONE; 
        }
    }

    uint32_t to_ms = power_manager_get_auto_sleep_timeout() * 1000;
    if (to_ms > 0 && (now - last_activity_time > to_ms)) {
        if (is_dimmed_aod) { 
            if (aod_timed_mode && (now - last_activity_time > to_ms + (aod_duration_sec * 1000))) {
                power_manager_enter_deep_sleep();
            }
        }
        else if (current_state == (AppState)STATE_TIMER_ALARM) {} 
        else if (current_state == STATE_WATCHFACE) {
            if (aod_allowed) { is_dimmed_aod = true; display_hal_backlight_set(brightness_aod_val); last_rendered_state = (AppState)-1; last_min_val = -1; }
            else { power_manager_enter_deep_sleep(); }
        } else if (current_state != STATE_EXEC_STOPWATCH && current_state != STATE_EXEC_TIMER && current_state != STATE_EXEC_HR) {
            if (!aod_allowed) power_manager_enter_deep_sleep();
            else { is_dimmed_aod = true; display_hal_backlight_set(brightness_aod_val); current_state = STATE_WATCHFACE; nd = true; }
        }
    }

    if (now - last_fps_time >= 1000) { current_fps = draw_count; draw_count = 0; last_fps_time = now; }
    
    if (current_state == STATE_EXEC_HR) {
        max30100_hal_update(); // [ON-DEMAND]
        if (max30100_hal_check_beat()) nd = true; 
    }
    static unsigned long last_bg_poll = 0;
    if (now - last_bg_poll > 60000) { // [BG SYNC] Poll sensor every 60s for 32-bit accumulation
        bmi160_hal_update(); 
        last_bg_poll = now;
    }

    if (current_state == STATE_EXEC_STEPS) {
        bmi160_hal_update(); // [ON-DEMAND]
    }

    AppState target = current_state; int d = 0;
    if (bt == BTN_RIGHT_CLICK) {
        if (current_state == STATE_SET_BRIGHTNESS) { brightness_ui_val += 15; if (brightness_ui_val > 255) brightness_ui_val = 255; display_hal_backlight_set(brightness_ui_val); nd = true; }
        else if (current_state == STATE_SET_TIMEOUT) { uint32_t t = power_manager_get_auto_sleep_timeout(); power_manager_set_auto_sleep_timeout(t + 5); nd = true; }
        else if (current_state == STATE_SET_TIMER_H) { timer_h = (timer_h + 1) % 24; nd = true; }
        else if (current_state == STATE_SET_TIMER_M) { timer_m = (timer_m + 1) % 60; nd = true; }
        else if (current_state == STATE_SET_TIMER_S) { timer_s = (timer_s + 1) % 60; nd = true; }
        else if (current_state == STATE_EXEC_STOPWATCH) { if (sw_running) { sw_elapsed_ms += (now - sw_start_ms); sw_running = false; } else { sw_start_ms = now; sw_running = true; } nd = true; }
        else if (current_state == STATE_EXEC_SETTINGS) { settings_menu_idx = (settings_menu_idx + 1) % 4; nd = true; } // 4 items in Settings
        else if (current_state == STATE_SET_AOD_CONFIG) { sub_menu_idx = (sub_menu_idx + 1) % 3; nd = true; }
        else if (current_state == STATE_SET_AOD_BRIGHTNESS) { brightness_aod_val += 15; if (brightness_aod_val > 255) brightness_aod_val = 255; nd = true; }
        else if (current_state == STATE_SET_RAISE2WAKE) { sub_menu_idx = (sub_menu_idx + 1) % 3; nd = true; }
        else if (current_state != STATE_EXEC_HR && current_state != STATE_EXEC_TIMER && (int)current_state != STATE_TIMER_ALARM) {
            d = 1;
            if (current_state == STATE_WATCHFACE)       target = STATE_MENU_AOD;
            else if (current_state == STATE_MENU_AOD)   target = STATE_MENU_HR;
            else if (current_state == STATE_MENU_HR)    target = STATE_MENU_TIMER;
            else if (current_state == STATE_MENU_TIMER) target = STATE_MENU_STOPWATCH;
            else if (current_state == STATE_MENU_STOPWATCH) target = STATE_MENU_SETTINGS;
            else if (current_state == STATE_MENU_SETTINGS) target = STATE_MENU_BRIGHTNESS;
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
        else if (current_state == STATE_SET_AOD_BRIGHTNESS) { brightness_aod_val -= 15; if (brightness_aod_val < 1) brightness_aod_val = 1; nd = true; }
        else if (current_state == STATE_EXEC_STOPWATCH) { sw_running = false; sw_elapsed_ms = 0; nd = true; }
        else if (current_state == STATE_EXEC_HR) { 
            max30100_hal_shutdown(); 
            target = STATE_MENU_HR; 
        }
        else if (current_state == STATE_EXEC_SETTINGS) { settings_menu_idx--; if (settings_menu_idx < 0) settings_menu_idx = 3; nd = true; }
        else if (current_state == STATE_SET_AOD_CONFIG) { sub_menu_idx--; if (sub_menu_idx < 0) sub_menu_idx = 2; nd = true; }
        else if (current_state == STATE_SET_RAISE2WAKE) { sub_menu_idx--; if (sub_menu_idx < 0) sub_menu_idx = 2; nd = true; }
        else if (current_state != STATE_EXEC_HR && current_state != STATE_EXEC_TIMER && (int)current_state != STATE_TIMER_ALARM) {
            d = -1;
            if (current_state == STATE_WATCHFACE)        target = STATE_EXEC_STEPS; 
            else if (current_state == STATE_MENU_AOD)    target = STATE_WATCHFACE;
            else if (current_state == STATE_MENU_HR)     target = STATE_MENU_AOD;
            else if (current_state == STATE_MENU_TIMER)  target = STATE_MENU_HR;
            else if (current_state == STATE_MENU_STOPWATCH) target = STATE_MENU_TIMER;
            else if (current_state == STATE_MENU_SETTINGS) target = STATE_MENU_STOPWATCH;
            else if (current_state == STATE_MENU_BRIGHTNESS) target = STATE_MENU_SETTINGS;
            else if (current_state == STATE_EXEC_STEPS) target = STATE_MENU_BRIGHTNESS;
        }
    } else if (bt == BTN_RIGHT_DOUBLE) {
        if (current_state == STATE_SET_TIMER_H) { target = STATE_SET_TIMER_M; }
        else if (current_state == STATE_SET_TIMER_M) { target = STATE_SET_TIMER_S; }
        else if (current_state == STATE_SET_TIMER_S) { target = STATE_SET_TIMER_H; }
    } else if (bt == BTN_RIGHT_HOLD) {
        if      (current_state == STATE_EXEC_STEPS) { bmi160_hal_reset_steps(); nd = true; } // Reset ONLY in dash
        else if (current_state == STATE_MENU_HR)    { if (max30100_hal_init()) { target = STATE_EXEC_HR; hr_start_time = millis(); nd = true; } }
        else if (current_state == STATE_EXEC_HR)    { max30100_hal_shutdown(); target = STATE_MENU_HR; }
        else if (current_state == STATE_SET_TIMEOUT)  target = STATE_EXEC_SETTINGS; // [UI AGENT] Return to settings
        else if (current_state == STATE_MENU_AOD)   { aod_allowed = !aod_allowed; nd = true; }
        else if (current_state == STATE_MENU_BRIGHTNESS) target = STATE_SET_BRIGHTNESS;
        else if (current_state == STATE_SET_BRIGHTNESS)  target = STATE_MENU_BRIGHTNESS;
        else if (current_state == STATE_MENU_TIMER)  { timer_h = 0; timer_m = 0; timer_s = 0; target = STATE_SET_TIMER_H; }
        else if (current_state >= STATE_SET_TIMER_H && current_state <= STATE_SET_TIMER_S) { if (timer_h+timer_m+timer_s > 0) { timer_target_ms = now + (timer_h*3600 + timer_m*60 + timer_s)*1000; timer_running = true; target = STATE_EXEC_TIMER; } }
        else if (current_state == STATE_EXEC_TIMER)  { timer_running = false; target = STATE_MENU_TIMER; }
        else if (current_state == STATE_MENU_STOPWATCH) { if (!sw_running) { sw_start_ms = now; sw_elapsed_ms = 0; sw_running = true; } target = STATE_EXEC_STOPWATCH; }
        else if (current_state == STATE_EXEC_STOPWATCH) { target = STATE_MENU_STOPWATCH; }
        else if (current_state == STATE_MENU_SYNC) { ble_is_syncing = !ble_is_syncing; nd = true; } // [UI AGENT] Toggle Mode
        else if (current_state == STATE_MENU_SETTINGS) { target = STATE_EXEC_SETTINGS; settings_menu_idx = 0; }
        else if (current_state == STATE_EXEC_SETTINGS) {
            // Action inside settings menu
            if (settings_menu_idx == 0) { target = STATE_SET_AOD_CONFIG; sub_menu_idx = 0; } 
            else if (settings_menu_idx == 1) { target = STATE_SET_RAISE2WAKE; sub_menu_idx = 0; }
            else if (settings_menu_idx == 2) { 
                power_manager_set_ble_enabled(!power_manager_get_ble_enabled());
                ble_hal_update_enabled();
                nd = true; 
            }
            else if (settings_menu_idx == 3) { target = STATE_SET_TIMEOUT; }
        }
        else if (current_state == STATE_SET_AOD_CONFIG) {
            if (sub_menu_idx == 0) { aod_timed_mode = !aod_timed_mode; nd = true; }
            else if (sub_menu_idx == 1) { target = STATE_SET_AOD_BRIGHTNESS; }
            else if (sub_menu_idx == 2 && aod_timed_mode) { aod_duration_sec += 10; if (aod_duration_sec > 120) aod_duration_sec = 10; nd = true; }
        }
        else if (current_state == STATE_SET_AOD_BRIGHTNESS) { target = STATE_SET_AOD_CONFIG; }
        else if (current_state == STATE_SET_RAISE2WAKE) {
            if (sub_menu_idx == 0) { power_manager_set_raise_to_wake(!power_manager_get_raise_to_wake()); nd = true; }
            else if (sub_menu_idx == 1) { uint8_t m = power_manager_get_rtw_mode(); m = (m + 1) % 2; power_manager_set_rtw_mode(m); nd = true; }
            else if (sub_menu_idx == 2) { uint8_t s = power_manager_get_rtw_sensitivity(); s = (s + 1) % 3; power_manager_set_rtw_sensitivity(s); nd = true; }
        }
    } else if (bt == BTN_LEFT_HOLD) {
        if (current_state >= STATE_SET_TIMER_H && current_state <= STATE_EXEC_TIMER) target = STATE_MENU_TIMER;
        else if (current_state == (int)STATE_TIMER_ALARM) { display_hal_backlight_set(brightness_ui_val); target = STATE_MENU_TIMER; }
        else if (current_state == STATE_EXEC_STOPWATCH) target = STATE_MENU_STOPWATCH;
        else if (current_state == STATE_EXEC_HR) { max30100_hal_shutdown(); target = STATE_MENU_HR; }
        else if (current_state == STATE_SYNCING) target = STATE_MENU_SYNC; // [UI AGENT] Emergency Exit for Testing
        else if (current_state == STATE_EXEC_SETTINGS) target = STATE_MENU_SETTINGS;
        else if (current_state == STATE_SET_AOD_BRIGHTNESS) { target = STATE_SET_AOD_CONFIG; }
        else if (current_state == STATE_SET_AOD_CONFIG || current_state == STATE_SET_RAISE2WAKE) target = STATE_EXEC_SETTINGS;
        else if (current_state == STATE_MENU_SYNC) target = STATE_EXEC_SETTINGS; // Exit sync to settings
        else if (current_state == STATE_SET_TIMEOUT) target = STATE_EXEC_SETTINGS; // Exit timeout to settings
        else power_manager_enter_deep_sleep();
    }

    if (target != current_state) {
        if (d != 0 && current_state != STATE_WATCHFACE && target != STATE_WATCHFACE) animate_slide_transition(current_state, target, d);
        current_state = target; nd = true;
        if (current_state >= STATE_SET_TIMER_H && current_state <= STATE_SET_TIMER_S) button_hal_set_double_click(true);
        else button_hal_set_double_click(false);
    }

    uint32_t iv = (current_state == STATE_EXEC_TIMER || current_state == STATE_EXEC_STOPWATCH || (int)current_state == STATE_TIMER_ALARM || (current_state == STATE_MENU_SYNC && ble_is_syncing)) ? 50 : 2000;
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
    
    // [POWER AGENT] Smart Shift Guard
    if (boot_stabilize_counter < 300) { boot_stabilize_counter++; power_manager_set_freq(FREQ_HIGH); ui_cpu_freq = FREQ_HIGH; }
    else { power_manager_set_freq(FREQ_MID); ui_cpu_freq = FREQ_MID; }
    
    bool sjc = (current_state != last_rendered_state);
    bool from_full_black = (last_rendered_state == STATE_EXEC_HR || last_rendered_state == STATE_EXEC_TIMER || last_rendered_state == STATE_EXEC_STOPWATCH || (int)last_rendered_state == STATE_TIMER_ALARM || (last_rendered_state >= STATE_SET_TIMER_H && last_rendered_state <= STATE_SET_TIMER_S));
    bool is_full_black_mode = (current_state == STATE_EXEC_HR || current_state == STATE_EXEC_TIMER || current_state == STATE_EXEC_STOPWATCH || (int)current_state == STATE_TIMER_ALARM || (current_state >= STATE_SET_TIMER_H && current_state <= STATE_SET_TIMER_S));
    
    if (sjc && !is_full_black_mode) {
        if (from_full_black || last_rendered_state == STATE_WATCHFACE || last_rendered_state == STATE_EXEC_SETTINGS || last_rendered_state == STATE_SET_AOD_CONFIG || last_rendered_state == STATE_SET_AOD_BRIGHTNESS || last_rendered_state == STATE_SET_RAISE2WAKE || last_rendered_state == STATE_EXEC_STEPS || last_rendered_state == (AppState)-1) {
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

            // 2. BPM & SPO2 VALUE (CENTERED)
            uint8_t bpm = (uint8_t)max30100_hal_get_bpm();
            uint8_t spo2 = max30100_hal_get_spo2(); 
            if (bpm != last_bpm_v || last_rendered_state != STATE_EXEC_HR) {
                tft.fillRect(0, 65, 240, 50, C_BG);
                tft.setTextDatum(TC_DATUM); tft.setTextColor(TFT_WHITE); tft.setTextSize(4); 
                char buf[8]; sprintf(buf, "%d", bpm); 
                tft.drawString(buf, 120, 68);
                tft.setTextSize(1); tft.setTextColor(C_MUTED); 
                tft.drawString("BPM", 120, 108);
                tft.drawFastHLine(16, 122, 208, C_DIVIDER);
                last_bpm_v = bpm;
                ble_hal_notify_hr(bpm, spo2); // [SYNC] Live stream to app
            }

            // 3. WAVEFORM GRAPH (10Hz UPDATE)
            static uint32_t last_g_upd = 0;
            if (now - last_g_upd > 100 || last_rendered_state != STATE_EXEC_HR) {
                canvas_spr.fillRect(0, 0, 208, 80, C_BG);
                float hist[60]; max30100_hal_get_history(hist);
                for(int i=0; i < 59; i++) {
                    if (hist[i] > 30 && hist[i+1] > 30) {
                        int y1 = 80 - (int)((constrain(hist[i], 40, 180) - 40) / 140.0 * 75);
                        int y2 = 80 - (int)((constrain(hist[i+1], 40, 180) - 40) / 140.0 * 75);
                        canvas_spr.drawLine(i*3.5, y1+1, i*3.5, 80, 0x6003); // Area
                        if (i < 58) canvas_spr.drawLine(i*3.5+1, (y1+y2)/2 + 1, i*3.5+1, 80, 0x6003); 
                        canvas_spr.drawLine(i*3.5, y1, (i+1)*3.5, y2, C_HR_RED); // Line
                    }
                }
                push_sub_sprite(16, 124, 208, 80); 
                tft.drawFastHLine(16, 214, 208, C_DIVIDER);
                last_g_upd = now;
            }

            // 4. BOTTOM SPO2 & TIMER (Reuse Canvas)
            spo2 = max30100_hal_get_spo2(); 
            uint32_t elap_s = (now - hr_start_time) / 1000;
            static uint32_t last_timer_v = 999;
            
            if (spo2 != last_spo2_v || elap_s != last_timer_v || last_rendered_state != STATE_EXEC_HR) {
                canvas_spr.fillRect(0, 0, 240, 65, C_BG);
                canvas_spr.setTextDatum(TL_DATUM); canvas_spr.setTextColor(C_MUTED); canvas_spr.setTextSize(1); 
                canvas_spr.drawString("SPO2", 16, 6);
                canvas_spr.setTextColor(C_SPO2); canvas_spr.setTextSize(3); 
                char sb[8]; sprintf(sb, "%d%%", spo2); canvas_spr.drawString(sb, 16, 20);
                
                canvas_spr.setTextDatum(TR_DATUM); canvas_spr.setTextColor(C_MUTED); canvas_spr.setTextSize(1); 
                canvas_spr.drawString("ELAPSED", 224, 6);
                canvas_spr.setTextColor(C_DIM); canvas_spr.setTextSize(2); 
                char tb[16]; sprintf(tb, "%d:%02d", elap_s/60, elap_s%60); 
                canvas_spr.drawString(tb, 224, 20);
                
                push_sub_sprite(0, 215, 240, 65); 
                last_spo2_v = spo2; last_timer_v = elap_s;
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
            if (last_rendered_state != STATE_SET_TIMEOUT) {
                canvas_spr.pushImage(-MENU_BX, -MENU_BY, 240, 280, assets_get_wallpaper());
                draw_menu_card(canvas_spr, STATE_MENU_TIMEOUT, 0, 0);
            } else { canvas_spr.fillRect(40, 45, 120, 30, BOX_COL); } 
            canvas_spr.setTextColor(TFT_GOLD); canvas_spr.setTextSize(2); canvas_spr.drawString("-", 30, 60); canvas_spr.drawString("+", 170, 60);
            { char buf[16]; sprintf(buf, "%d s", power_manager_get_auto_sleep_timeout()); canvas_spr.setTextColor(TFT_WHITE); canvas_spr.drawString(buf, MENU_BW/2, 60); }
            push_sub_sprite(MENU_BX, MENU_BY, MENU_BW, MENU_BH); break;
        case STATE_SET_BRIGHTNESS:
            canvas_spr.pushImage(-MENU_BX, -MENU_BY, 240, 280, assets_get_wallpaper());
            draw_menu_card(canvas_spr, STATE_MENU_BRIGHTNESS, 0, 0);
            { int bar_w = (brightness_ui_val * 160) / 255; canvas_spr.drawRoundRect(20, 75, 160, 12, 6, TFT_WHITE); canvas_spr.fillRect(22, 77, 156, 8, BOX_COL); canvas_spr.fillRoundRect(22, 77, bar_w > 4 ? bar_w - 4 : 0, 8, 4, TFT_GREEN); }
            push_sub_sprite(MENU_BX, MENU_BY, MENU_BW, MENU_BH); break;
        case STATE_EXEC_SETTINGS: {
            if (last_rendered_state != STATE_EXEC_SETTINGS) {
                tft.pushImage(0, 0, 240, 280, assets_get_wallpaper());
                tft.setTextDatum(MC_DATUM); tft.setTextColor(TFT_WHITE); tft.setTextSize(2);
                tft.drawString("SETTINGS", 120, 30);
                tft.setTextSize(1); tft.setTextColor(TFT_DARKGREY);
                tft.drawString("L/R: SCROLL | R-HOLD: ENTER", 120, 250);
            }
            canvas_spr.pushImage(0, -60, 240, 280, assets_get_wallpaper());
            const char* options[] = {"AOD", "Raise2Wake", "BLE / Sync", "Timeout"};
            for (int i=0; i<4; i++) {
                int y_pos = 10 + i * 35;
                if (i == settings_menu_idx) {
                    canvas_spr.fillRoundRect(20, y_pos, 200, 30, 8, TFT_GOLD);
                    canvas_spr.setTextColor(TFT_BLACK);
                } else {
                    canvas_spr.fillRoundRect(20, y_pos, 200, 30, 8, BOX_COL);
                    canvas_spr.setTextColor(TFT_WHITE);
                }
                canvas_spr.setTextDatum(ML_DATUM); canvas_spr.setTextSize(2);
                canvas_spr.drawString(options[i], 30, y_pos + 15);
                
                canvas_spr.setTextDatum(MR_DATUM);
                if (i == 0) { canvas_spr.drawString(aod_allowed ? "ON" : "OFF", 210, y_pos + 15); }
                else if (i == 1) { canvas_spr.drawString(power_manager_get_raise_to_wake() ? "ON" : "OFF", 210, y_pos + 15); }
                else if (i == 2) { canvas_spr.drawString(power_manager_get_ble_enabled() ? "ON" : "OFF", 210, y_pos + 15); }
                else if (i == 3) { char buf[8]; sprintf(buf, "%ds", power_manager_get_auto_sleep_timeout()); canvas_spr.drawString(buf, 210, y_pos + 15); }
            }
            push_sub_sprite(0, 60, 240, 160);
            break;
        }
        case STATE_SET_AOD_CONFIG: {
            if (last_rendered_state != STATE_SET_AOD_CONFIG) {
                tft.pushImage(0, 0, 240, 280, assets_get_wallpaper());
                tft.setTextDatum(MC_DATUM); tft.setTextColor(TFT_WHITE); tft.setTextSize(2);
                tft.drawString("AOD CONFIG", 120, 30);
                tft.setTextSize(1); tft.setTextColor(TFT_DARKGREY);
                tft.drawString("L-HOLD: BACK | R-HOLD: TOGGLE", 120, 250);
            }
            canvas_spr.pushImage(0, -60, 240, 280, assets_get_wallpaper());
            const char* options[] = {"Mode", "Brightness", "Duration"};
            for (int i=0; i<3; i++) {
                int y_pos = 20 + i * 40;
                if (i == sub_menu_idx) {
                    canvas_spr.fillRoundRect(20, y_pos, 200, 30, 8, TFT_GOLD);
                    canvas_spr.setTextColor(TFT_BLACK);
                } else {
                    canvas_spr.fillRoundRect(20, y_pos, 200, 30, 8, BOX_COL);
                    if (i == 2 && !aod_timed_mode) canvas_spr.setTextColor(C_DIM);
                    else canvas_spr.setTextColor(TFT_WHITE);
                }
                canvas_spr.setTextDatum(ML_DATUM); canvas_spr.setTextSize(2);
                canvas_spr.drawString(options[i], 30, y_pos + 15);
                
                canvas_spr.setTextDatum(MR_DATUM);
                if (i == 0) { canvas_spr.drawString(aod_timed_mode ? "TIMED" : "ALWAYSON", 210, y_pos + 15); }
                else if (i == 1) { char buf[8]; sprintf(buf, "%d", brightness_aod_val); canvas_spr.drawString(buf, 210, y_pos + 15); }
                else if (i == 2) { 
                    if (!aod_timed_mode) canvas_spr.drawString("-", 210, y_pos + 15);
                    else { char buf[8]; sprintf(buf, "%ds", aod_duration_sec); canvas_spr.drawString(buf, 210, y_pos + 15); }
                }
            }
            push_sub_sprite(0, 60, 240, 160);
            break;
        }
        case STATE_SET_AOD_BRIGHTNESS: {
            if (last_rendered_state != STATE_SET_AOD_BRIGHTNESS) {
                tft.pushImage(0, 0, 240, 280, assets_get_wallpaper());
                tft.setTextDatum(MC_DATUM); tft.setTextColor(TFT_WHITE); tft.setTextSize(2);
                tft.drawString("AOD BRIGHTNESS", 120, 30);
                tft.setTextSize(1); tft.setTextColor(TFT_DARKGREY);
                tft.drawString("L/R: ADJUST | R-HOLD: SAVE", 120, 250);
            }
            canvas_spr.pushImage(-MENU_BX, -MENU_BY, 240, 280, assets_get_wallpaper());
            draw_menu_card(canvas_spr, STATE_MENU_BRIGHTNESS, 0, 0); // Reuse brightness card
            { 
                int bar_w = (brightness_aod_val * 160) / 255; 
                canvas_spr.drawRoundRect(20, 75, 160, 12, 6, TFT_WHITE); 
                canvas_spr.fillRect(22, 77, 156, 8, BOX_COL); 
                canvas_spr.fillRoundRect(22, 77, bar_w > 4 ? bar_w - 4 : 0, 8, 4, 0xF96A); // Pinkish red for AOD
            }
            push_sub_sprite(MENU_BX, MENU_BY, MENU_BW, MENU_BH);
            break;
        }
        case STATE_SET_RAISE2WAKE: {
            if (last_rendered_state != STATE_SET_RAISE2WAKE) {
                tft.pushImage(0, 0, 240, 280, assets_get_wallpaper());
                tft.setTextDatum(MC_DATUM); tft.setTextColor(TFT_WHITE); tft.setTextSize(2);
                tft.drawString("RAISE TO WAKE", 120, 30);
                tft.setTextSize(1); tft.setTextColor(TFT_DARKGREY);
                tft.drawString("L-HOLD: BACK | R-HOLD: TOGGLE", 120, 250);
            }
            canvas_spr.pushImage(0, -60, 240, 280, assets_get_wallpaper());
            const char* options[] = {"Toggle", "Mode", "Sensitivity"};
            for (int i=0; i<3; i++) {
                int y_pos = 10 + i * 45;
                if (i == sub_menu_idx) {
                    canvas_spr.fillRoundRect(20, y_pos, 200, 35, 8, TFT_GOLD);
                    canvas_spr.setTextColor(TFT_BLACK);
                } else {
                    canvas_spr.fillRoundRect(20, y_pos, 200, 35, 8, BOX_COL);
                    canvas_spr.setTextColor(TFT_WHITE);
                }
                canvas_spr.setTextDatum(ML_DATUM); canvas_spr.setTextSize(2);
                canvas_spr.drawString(options[i], 30, y_pos + 17);
                
                canvas_spr.setTextDatum(MR_DATUM);
                if (i == 0) { canvas_spr.drawString(power_manager_get_raise_to_wake() ? "ON" : "OFF", 210, y_pos + 17); }
                else if (i == 1) { canvas_spr.drawString(power_manager_get_rtw_mode() == 1 ? "TILIT" : "ANYMOV", 210, y_pos + 17); }
                else if (i == 2) { 
                    uint8_t s = power_manager_get_rtw_sensitivity();
                    canvas_spr.drawString(s == 0 ? "HIGH" : (s == 1 ? "MED" : "LOW"), 210, y_pos + 17); 
                }
            }
            push_sub_sprite(0, 60, 240, 160);
            break;
        }
        case STATE_EXEC_STEPS:
            {
                // [POWER AGENT] Smart Shift Guard
                if (boot_stabilize_counter < 300) { boot_stabilize_counter++; power_manager_set_freq(FREQ_HIGH); ui_cpu_freq = FREQ_HIGH; }
                else { power_manager_set_freq(FREQ_MID); ui_cpu_freq = FREQ_MID; }

                static uint32_t step_warmup_timer = 0;
                
                // 1. RE-MASK (Top Overlay)
                if (last_rendered_state != STATE_EXEC_STEPS || clock_m != last_min_val) {
                    tft.pushImage(0, 0, 240, 60, assets_get_wallpaper()); 
                    push_top_clock(true);
                    tft.setTextDatum(MC_DATUM); tft.setTextColor(TFT_WHITE); tft.setTextSize(2); 
                    tft.drawString("ACTIVITY", 120, 30);
                    
                    // [STEP 5] Progress Bar if syncing
                    if (ble_is_syncing) {
                        float p = ble_hal_get_sync_progress();
                        int pw = (int)(200 * p);
                        tft.drawRect(20, 185, 200, 4, C_DIM);
                        tft.fillRect(20, 185, pw, 4, 0x5D9B); // Sky Blue Progress
                    }
                    step_warmup_timer = millis(); 
                }
                
                // 2. CONTINUOUS POLLING (v7.2)
                static uint32_t last_poll_t = 0;
                if (millis() - last_poll_t > 500) { bmi160_hal_update(); last_poll_t = millis(); }
                uint32_t steps = bmi160_hal_get_steps();

                uint32_t goal = 10000;
                float ratio = constrain(steps / (float)goal, 0.0f, 1.0f);
                if (isnan(ratio)) ratio = 0.0f;

                // 3. USA SURGICAL DRAWING (Zero-Flicker Ring)
                static uint32_t last_disp_steps = 999999;
                if (steps != last_disp_steps || last_rendered_state != STATE_EXEC_STEPS) {
                    // [UI AGENT] Removed redundant createSprite here (Handled by global guard)
                    
                    // A. Draw Ring into Sprite (Y offset 60, Height 145 for ring)
                    canvas_spr.pushImage(0, -60, 240, 280, assets_get_wallpaper());
                    int rx = 120, ry = 75; // [UI AGENT] Centered Y=75 relative to 145h sprite
                    canvas_spr.drawCircle(rx, ry, 70, 0x18C3); 
                    float end_angle = (ratio * 360.0f) - 90.0f;
                    for (float a = -90.0f; a <= end_angle; a += 10.0f) {
                        float rad = a * 0.0174533f;
                        uint16_t arc_col = tft.color565(255 - (int)(ratio * 100), 45 + (int)(ratio * 150), 100);
                        canvas_spr.fillCircle(rx + cos(rad)*70, ry + sin(rad)*70, 6, arc_col);
                    }
                    canvas_spr.setTextDatum(MC_DATUM); canvas_spr.setTextColor(TFT_WHITE);
                    canvas_spr.setTextSize(4); char s_buf[16]; sprintf(s_buf, "%d", steps);
                    canvas_spr.drawString(s_buf, rx, ry - 5);
                    canvas_spr.setTextSize(1); canvas_spr.setTextColor(C_MUTED); 
                    canvas_spr.drawString("STEPS TODAY", rx, ry + 25);
                    push_sub_sprite(0, 60, 240, 145);

                    // B. Draw Bottom Stats using small efficient Sprite (Y offset 205, Height 35)
                    canvas_spr.pushImage(0, -205, 240, 280, assets_get_wallpaper()); // Surgical Crop at Y=205
                    canvas_spr.setTextDatum(TL_DATUM); canvas_spr.setTextSize(1); canvas_spr.setTextColor(TFT_WHITE);
                    char k_buf[32]; sprintf(k_buf, "%d KCAL", (int)(steps * 0.044f)); 
                    canvas_spr.drawString(k_buf, 30, 215 - 205); // Y=215
                    canvas_spr.setTextDatum(TR_DATUM); 
                    char m_buf[32]; 
                    float distance_km = (steps * calibration_calculate_stride()) / 1000000.0f;
                    sprintf(m_buf, "%.1f KM", distance_km); 
                    canvas_spr.drawString(m_buf, 210, 215 - 205); // Y=215
                    push_sub_sprite(0, 205, 240, 35); 

                    last_disp_steps = steps;
                }
            }
            break;
        default:
            canvas_spr.pushImage(-MENU_BX, -MENU_BY, 240, 280, assets_get_wallpaper());
            draw_menu_card(canvas_spr, current_state, 0, 0); 
            push_sub_sprite(MENU_BX, MENU_BY, MENU_BW, MENU_BH); break;
    }
}

static void draw_lightning_bolt(TFT_eSprite& spr, int x, int y, uint16_t color) {
    // Elegant small lightning bolt
    spr.drawLine(x+2, y, x, y+5, color);
    spr.drawLine(x, y+5, x+4, y+4, color);
    spr.drawLine(x+4, y+4, x+2, y+10, color);
}

static void draw_watchface(TFT_eSPI& tft, bool full_redraw) {
    // [UI AGENT] SAFE BOOT: Stay at 160MHz for first ~5 seconds to stabilize USB CDC
    if (boot_stabilize_counter < 300) {
        boot_stabilize_counter++;
        power_manager_set_freq(FREQ_HIGH); ui_cpu_freq = FREQ_HIGH;
    } else if (!is_dimmed_aod) {
        power_manager_set_freq(FREQ_MID); ui_cpu_freq = FREQ_MID;
    }
    const uint16_t* wall = assets_get_wallpaper();
    if (full_redraw) { if (is_dimmed_aod) tft.fillScreen(TFT_BLACK); else tft.pushImage(0, 0, 240, 280, wall); }
    int cx = 30, cy = 60; 
    if (is_dimmed_aod) canvas_spr.fillRect(0, 0, 180, 150, TFT_BLACK); else canvas_spr.pushImage(-cx, -cy, 240, 280, wall);
    char buf[8]; sprintf(buf, "%02d", clock_h); canvas_spr.setTextDatum(MC_DATUM); canvas_spr.setTextColor(COLOR_WATCH_HH); canvas_spr.setTextSize(8); canvas_spr.drawString(buf, 90, 35);
    sprintf(buf, "%02d", clock_m); canvas_spr.setTextColor(COLOR_WATCH_MM); canvas_spr.drawString(buf, 90, 105);
    if (!is_dimmed_aod) { canvas_spr.setTextColor(COLOR_WATCH_DATE); canvas_spr.setTextSize(2); canvas_spr.drawString("TUE 17 MAR", 90, 145); }
    push_sub_sprite(cx, cy, 180, 150); uint8_t batt = get_battery_percentage(); int sx = 165, sy = 15;
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

static uint8_t get_battery_percentage() { 
    // [v6.6] Apply 60mV compensation if not in AOD mode (since LCD is drawing current)
    float v = power_manager_read_battery_voltage(!is_dimmed_aod); 
    bool charging = power_manager_is_charging();
    
    // [ANTI-SPIKE] Sanwa-observed jump: 0.27V. Using -0.25V compensation.
    float real_v = charging ? (v - 0.25f) : v;
    
    if (real_v >= 4.10f) return 100; // Sanwa 4.10V anchor
    if (real_v <= 3.35f) return 0;
    
    // [REAL CURVE] Li-ion Plateau mapping
    if (real_v > 3.90f) return (uint8_t)(80 + (real_v - 3.90f) / (4.10f - 3.90f) * 20);
    if (real_v > 3.65f) return (uint8_t)(40 + (real_v - 3.65f) / (3.90f - 3.65f) * 40);
    return (uint8_t)((real_v - 3.35f) / (3.65f - 3.35f) * 40);
}
bool ui_manager_is_aod_enabled() { return aod_allowed; }
bool ui_manager_is_high_load() { return ui_is_high_load; }
uint16_t ui_manager_get_cpu_freq() { return ui_cpu_freq; }

// [UI AGENT] Premium Taller Heart Icon
static void draw_heart_icon(TFT_eSPI &tft, int cx, int cy, uint16_t color, float scale) {
    int r = (int)(6 * scale); 
    int cy_adj = cy - (int)(3 * scale); // Nudged up slightly for fuller base
    tft.fillCircle(cx - r, cy_adj, r, color);
    tft.fillCircle(cx + r, cy_adj, r, color);
    // Fuller triangle tip (less sharp)
    tft.fillTriangle(cx - r*2, cy_adj, cx + r*2, cy_adj, cx, cy + (int)(13 * scale), color);
}

// [3.4] Remote Connectivity Commands
void ui_manager_request_state(AppState st) {
    if (is_dimmed_aod) {
        is_dimmed_aod = false;
        display_hal_backlight_set(brightness_ui_val);
        power_manager_set_freq(FREQ_MID);
    }
    
    // [POWER GUARD] Ensure sensor is off if leaving HR monitoring
    if (current_state == STATE_EXEC_HR && st != STATE_EXEC_HR) {
        max30100_hal_shutdown();
    }

    last_rendered_state = (AppState)-1; // Force full redraw on switch
    last_min_val = -1;
    
    if (st == STATE_EXEC_HR) {
        if (max30100_hal_init()) {
            current_state = st;
            hr_start_time = millis();
        }
    } else {
        current_state = st;
    }
}

// [3.3] Sensor Reporting on Request
void ui_manager_report_sensors(uint8_t command) {
    if (command == 0x05) { // Battery
        ble_hal_notify_battery(get_battery_percentage(), power_manager_is_charging());
    } else if (command == 0x04) { // Steps
        ble_hal_notify_steps(bmi160_hal_get_steps());
    }
}
