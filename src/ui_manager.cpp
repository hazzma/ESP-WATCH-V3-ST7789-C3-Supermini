#include "ui_manager.h"
#include "display_hal.h"
#include "button_hal.h"
#include "max30100_hal.h"
#include "power_manager.h"
#include "app_config.h"
#include "assets_wallpaper.h"
#include "assets_icons.h"

// Persist
static RTC_DATA_ATTR bool aod_allowed = false; 
static AppState current_state = STATE_WATCHFACE;

// Runtime
static bool is_dimmed_aod = false; 
static unsigned long last_activity_time = 0;
static AppState last_rendered_state = (AppState)-1;
static float current_fps = 0;
static uint32_t draw_count = 0;
static uint32_t last_fps_time = 0;

// Change Detection & Time
static int last_min_val = -1;
static uint8_t last_batt_val = 255;
static int clock_h = 15; 
static int clock_m = 47;
static unsigned long last_tick = 0;

// Sprites
static TFT_eSprite clock_spr = TFT_eSprite(&display_hal_get_tft());
static TFT_eSprite status_spr = TFT_eSprite(&display_hal_get_tft());
static TFT_eSprite fps_spr = TFT_eSprite(&display_hal_get_tft());

// Helpers
static void render_current_state();
static uint8_t get_battery_percentage();
static void draw_watchface(TFT_eSPI& tft, bool full_redraw);

void ui_manager_init() {
    last_activity_time = millis();
    last_tick = millis();
    
    clock_spr.createSprite(180, 150);
    clock_spr.setSwapBytes(true); 
    status_spr.createSprite(65, 25);
    status_spr.setSwapBytes(true);
    fps_spr.createSprite(35, 15);
    fps_spr.setSwapBytes(true);
    
    render_current_state();
    display_hal_backlight_fade_in(255, BL_FADE_MS);
}

void ui_manager_update() {
    ButtonEvent bt = button_hal_read();
    unsigned long now = millis();

    // Simple Ticking Clock (Software based)
    if (now - last_tick >= 60000) {
        clock_m++;
        if (clock_m >= 60) { clock_m = 0; clock_h++; }
        if (clock_h >= 24) { clock_h = 0; }
        last_tick = now;
    }

    // Interaction resets AOD
    if (bt != BTN_NONE) {
        last_activity_time = now;
        if (is_dimmed_aod) {
            is_dimmed_aod = false;
            display_hal_backlight_set(255);
            power_manager_set_freq(FREQ_MID);
            last_rendered_state = (AppState)-1; 
            last_min_val = -1; // Force repaint
            bt = BTN_NONE;     // Consume event: first click only wakes up
        }
    }

    // Auto-AOD Entry
    if (aod_allowed && !is_dimmed_aod && (now - last_activity_time > AOD_TIMEOUT_MS)) {
        if (current_state == STATE_WATCHFACE) {
            is_dimmed_aod = true;
            display_hal_backlight_set(15); 
            last_rendered_state = (AppState)-1;
            last_min_val = -1; 
        }
    }

    // Stats
    if (now - last_fps_time >= 1000) {
        current_fps = (float)draw_count;
        draw_count = 0;
        last_fps_time = now;
    }

    // State Machine
    switch (current_state) {
        case STATE_WATCHFACE: if (bt == BTN_SHORT_CLICK) current_state = STATE_MENU_HR; break;
        case STATE_MENU_HR:
            if (bt == BTN_SHORT_CLICK) current_state = STATE_MENU_AOD;
            else if (bt == BTN_LONG_HOLD) { if (max30100_hal_init()) current_state = STATE_EXEC_HR; }
            break;
        case STATE_EXEC_HR:
            max30100_hal_update();
            if (bt == BTN_SHORT_CLICK) { max30100_hal_shutdown(); current_state = STATE_WATCHFACE; }
            break;
        case STATE_MENU_AOD:
            if (bt == BTN_SHORT_CLICK) current_state = STATE_MENU_SLEEP;
            else if (bt == BTN_LONG_HOLD) { aod_allowed = !aod_allowed; current_state = STATE_WATCHFACE; }
            break;
        case STATE_MENU_SLEEP: 
            if (bt == BTN_SHORT_CLICK) current_state = STATE_WATCHFACE; 
            else if (bt == BTN_LONG_HOLD) {
                // Enter Sleep triggered by UI
                power_manager_enter_deep_sleep();
            }
            break;
        default: break;
    }

    // Smart Render
    uint32_t interval = is_dimmed_aod ? 1000 : 500;
    if (current_state == STATE_EXEC_HR) interval = 100;
    
    static unsigned long last_refresh = 0;
    bool needs_draw = (current_state != last_rendered_state);
    
    if (!needs_draw && (current_state == STATE_WATCHFACE) && (now - last_refresh > interval)) {
        uint8_t current_batt = get_battery_percentage();
        if (current_batt != last_batt_val || clock_m != last_min_val || (now - last_refresh > 10000)) {
            needs_draw = true;
        }
    } else if (!needs_draw && (now - last_refresh > interval)) {
        needs_draw = true; 
    }
    
    if (needs_draw) {
        // Frequency Burst for rendering
        if (is_dimmed_aod) power_manager_set_freq(40); 
        
        render_current_state();
        
        if (is_dimmed_aod) power_manager_set_freq(FREQ_AOD); 
        
        last_rendered_state = current_state;
        last_refresh = now;
        draw_count++; 
    }
}

static void render_current_state() {
    TFT_eSPI& tft = display_hal_get_tft();
    
    if (current_state == STATE_WATCHFACE) {
        draw_watchface(tft, (current_state != last_rendered_state));
    } else {
        power_manager_set_freq(FREQ_MID);
        if (current_state != last_rendered_state) tft.fillScreen(TFT_BLACK);
        
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        
        if (current_state == STATE_MENU_HR) {
            tft.setTextSize(2); tft.drawString("MENU: Heart Rate", 120, 140);
        } else if (current_state == STATE_EXEC_HR) {
            power_manager_set_freq(FREQ_HIGH);
            tft.setTextColor(TFT_RED, TFT_BLACK); tft.setTextSize(2);
            tft.drawString("Measuring...", 120, 80);
            tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.setTextSize(6);
            tft.setTextPadding(140);
            char buf[16]; sprintf(buf, "%.0f", max30100_hal_get_bpm());
            tft.drawString(buf, 120, 150); 
            tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.setTextSize(2);
            tft.setTextPadding(100);
            sprintf(buf, "SpO2: %d%%", max30100_hal_get_spo2());
            tft.drawString(buf, 120, 220);
            tft.setTextPadding(0);
        } else if (current_state == STATE_MENU_AOD) {
            tft.setTextSize(2); tft.drawString("MENU: AOD Setting", 120, 140);
            tft.setTextColor(aod_allowed ? TFT_GREEN : TFT_RED);
            tft.drawString(aod_allowed ? "[AOD: READY]" : "[AOD: OFF]", 120, 170);
        } else { tft.drawString("MENU: SYSTEM Sleep", 120, 140); tft.setTextSize(1); tft.drawString("[HOLD TO SLEEP]", 120, 170); }
    }

    #if SHOW_FPS
    int fx = 35, fy = 260;
    if (current_state == STATE_WATCHFACE && !is_dimmed_aod) {
        fps_spr.pushImage(-fx, -fy, 240, 280, assets_get_wallpaper());
    } else { fps_spr.fillSprite(TFT_BLACK); }
    fps_spr.setTextColor(TFT_DARKGREY);
    fps_spr.setTextDatum(MC_DATUM);
    fps_spr.drawNumber((int)current_fps, 17, 7);
    fps_spr.pushSprite(fx, fy);
    #endif
}

static void draw_watchface(TFT_eSPI& tft, bool full_redraw) {
    if (!is_dimmed_aod) power_manager_set_freq(FREQ_MID);
    
    const uint16_t* wall = assets_get_wallpaper();

    if (full_redraw) {
        if (is_dimmed_aod) tft.fillScreen(TFT_BLACK);
        else tft.pushImage(0, 0, 240, 280, wall);
    }

    int cx = 30, cy = 60;
    if (is_dimmed_aod) clock_spr.fillSprite(TFT_BLACK);
    else clock_spr.pushImage(-cx, -cy, 240, 280, wall);
    
    char time_buf[8];
    clock_spr.setTextDatum(MC_DATUM);
    clock_spr.setTextColor(COLOR_WATCH_HH);
    clock_spr.setTextSize(8);
    sprintf(time_buf, "%02d", clock_h);
    clock_spr.drawString(time_buf, 90, 35);
    
    clock_spr.setTextColor(COLOR_WATCH_MM);
    sprintf(time_buf, "%02d", clock_m);
    clock_spr.drawString(time_buf, 90, 105);
    
    if (!is_dimmed_aod) {
        clock_spr.setTextColor(COLOR_WATCH_DATE);
        clock_spr.setTextSize(2);
        clock_spr.drawString("TUE 17 MAR", 90, 145);
    }
    clock_spr.pushSprite(cx, cy);

    uint8_t batt = get_battery_percentage();
    int sx = 165, sy = 15;
    
    if (is_dimmed_aod) status_spr.fillSprite(TFT_BLACK);
    else status_spr.pushImage(-sx, -sy, 240, 280, wall);

    status_spr.pushImage(45, 5, 16, 16, assets_get_icon_battery());
    int fill_w = (batt * 10) / 100;
    status_spr.fillRect(47, 8, fill_w, 10, (batt < 20) ? TFT_RED : TFT_GREEN);
    
    status_spr.setTextDatum(TR_DATUM);
    status_spr.setTextColor(TFT_WHITE);
    status_spr.setTextSize(1);
    status_spr.drawNumber(batt, 42, 9);
    status_spr.pushSprite(sx, sy);
    
    last_batt_val = batt;
    last_min_val = clock_m;
}

static uint8_t get_battery_percentage() {
    float v = power_manager_read_battery_voltage();
    if (v >= 4.10) return 100;
    if (v <= 3.35) return 0;
    return (uint8_t)((v - 3.40) / (4.20 - 3.40) * 100);
}

bool ui_manager_is_aod_enabled() { return aod_allowed; }
