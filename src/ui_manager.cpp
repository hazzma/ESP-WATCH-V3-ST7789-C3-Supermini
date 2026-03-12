#include "ui_manager.h"
#include "assets_wallpaper.h"
#include "assets_icons.h"

static bool ui_dirty = true;
static unsigned long last_render_time = 0;

void ui_init() {
    display_init();
    ui_dirty = true;
}

void ui_set_dirty() {
    ui_dirty = true;
}

void ui_render_battery(float pct, bool charging) {
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    
    char buf[16];
    snprintf(buf, sizeof(buf), "%.0f%%", pct);
    tft.drawString(buf, 185, 10, 2); 

    if (charging) {
        // Render 16x16 Bolt Icon further left of percentage
        // (x=160, y=10)
        tft.pushImage(160, 10, 16, 16, icon_charging_bolt); 
        // Serial.println(F("[UI] Rendering Chrg Icon"));
    }
}

void ui_render(SystemState state, float battery_pct, bool charging, struct tm* timeinfo, int bpm) {
    if (!ui_dirty) return;
    
    if (state == STATE_AOD) {
        // ... (AOD rendering remains same)
        tft.fillScreen(TFT_BLACK);
        char buf[8];
        tft.setTextColor(TFT_WHITE);
        snprintf(buf, sizeof(buf), "%02d", timeinfo->tm_hour);
        tft.drawCentreString(buf, 120, 60, 8); 
        snprintf(buf, sizeof(buf), "%02d", timeinfo->tm_min);
        tft.drawCentreString(buf, 120, 140, 8);
    } else {
        tft.pushImage(0, 0, 240, 280, wallpaper_main);
        
        if (state == STATE_WATCHFACE) {
            // ... (Watchface rendering)
            char buf[8];
            snprintf(buf, sizeof(buf), "%02d", timeinfo->tm_hour);
            tft.setTextColor(TFT_RED);
            tft.drawCentreString(buf, 120, 50, 8); 
            snprintf(buf, sizeof(buf), "%02d", timeinfo->tm_min);
            tft.setTextColor(TFT_WHITE);
            tft.drawCentreString(buf, 120, 130, 8); 
            
            char dateBuf[32];
            const char* days[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
            snprintf(dateBuf, sizeof(dateBuf), "%02d | %s", timeinfo->tm_mday, days[timeinfo->tm_wday]);
            tft.setTextColor(TFT_LIGHTGREY);
            tft.drawCentreString(dateBuf, 120, 215, 2);
        } else {
            tft.fillRect(0, 100, 240, 80, 0x0000); 
            tft.setTextSize(2);
            tft.setTextColor(TFT_WHITE, TFT_BLACK); 
        }
        
        ui_render_battery(battery_pct, charging);
    }
    
    switch (state) {
        case STATE_WATCHFACE:
        case STATE_AOD:
            break;
            
        case STATE_MENU_HR:
            tft.drawCentreString("MENU: HEART RATE", 120, 140, 2);
            break;
            
        case STATE_MENU_SETTINGS:
            tft.drawCentreString("AOD: PRESS TO TOGGLE", 120, 140, 2);
            break;
            
        case STATE_MENU_FORCE_SLEEP:
            tft.drawCentreString("MENU: FORCE SLEEP", 120, 140, 2);
            break;
            
        case STATE_EXECUTING_HR:
            if (bpm > 0) {
                char hrBuf[16];
                snprintf(hrBuf, sizeof(hrBuf), "HR: %d BPM", bpm);
                tft.setTextColor(TFT_RED, TFT_BLACK);
                tft.drawCentreString(hrBuf, 120, 140, 4);
            } else {
                tft.drawCentreString("NO FINGER DETECTED", 120, 140, 2);
            }
            break;
            
        default:
            break;
    }
    
    ui_dirty = false;
    last_render_time = millis();
}
