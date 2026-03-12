#include <Arduino.h>
#include <sys/time.h>
#include <time.h>
#include "config_pins.h"
#include "system_states.h"
#include "power_manager.h"
#include "ui_manager.h"
#include "button_hal.h"
#include "sensors_hal.h"

// System Globals
SystemState currentState = STATE_WATCHFACE;
PowerMode currentPowerMode = POWER_ACTIVE;

unsigned long lastInteractionTime = 0;
const unsigned long AUTO_SLEEP_TIMEOUT = 30000; // 30s per FSD Rule-2.3

float mockBatteryPct = 85.0; // Temporary mock until sensor_hal is ready
bool isCharging = false;
bool isAODEnabled = true; // Rule: Default ON when charging, setting toggleable

void sync_time_from_build() {
    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    
    char month[4];
    int day, year, hr, min, sec;
    sscanf(__DATE__, "%s %d %d", month, &day, &year);
    sscanf(__TIME__, "%d:%d:%d", &hr, &min, &sec);
    
    tm.tm_mday = day;
    tm.tm_year = year - 1900;
    tm.tm_hour = hr;
    tm.tm_min = min;
    tm.tm_sec = sec;
    
    const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    for (int i = 0; i < 12; i++) {
        if (strcmp(month, months[i]) == 0) {
            tm.tm_mon = i;
            break;
        }
    }
    
    time_t t = mktime(&tm);
    struct timeval now = { .tv_sec = t, .tv_usec = 0 };
    settimeofday(&now, NULL);
    Serial.println("[SYS] Time synced with Build Time.");
}

void setup() {
    Serial.begin(115200);
    // Langsung hidupkan layar agar user tahu jam sudah nyala (Rule-Simple)
    ui_init(); 
    display_fade_in(200); 
    Serial.println("[SYS] UI & Display Active.");

    // Minimalist Boot
    power_set_cpu_freq(POWER_ACTIVE);
    button_init();
    pinMode(CHRG_PIN, INPUT_PULLUP); 
    
    // Sinkronisasi waktu jika ini bukan bangun dari sleep
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_UNDEFINED) {
        sync_time_from_build();
    }

    lastInteractionTime = millis();
}

void handle_state_transitions(ButtonEvent event) {
    if (event == BTN_NONE) return;

    // Reset interaction timer on any button event (Rule-2.3)
    lastInteractionTime = millis();
    ui_set_dirty();

    switch (currentState) {
        case STATE_WATCHFACE:
            if (event == BTN_SHORT_CLICK) {
                currentState = STATE_MENU_HR;
            } else if (event == BTN_LONG_HOLD) {
                // Future shortcuts from watchface?
            }
            break;

        case STATE_MENU_HR:
            if (event == BTN_SHORT_CLICK) {
                currentState = STATE_MENU_SETTINGS;
            } else if (event == BTN_LONG_HOLD) {
                currentState = STATE_EXECUTING_HR;
                // Lazy-load the sensor only when starting measurement
                sensors_heartrate_init(); 
            }
            break;

        case STATE_MENU_SETTINGS:
            if (event == BTN_SHORT_CLICK) {
                currentState = STATE_MENU_FORCE_SLEEP;
            } else if (event == BTN_LONG_HOLD) {
                isAODEnabled = !isAODEnabled;
                Serial.printf("[SYS] AOD Toggled: %s\n", isAODEnabled ? "ON" : "OFF");
            }
            break;

        case STATE_AOD:
            if (event != BTN_NONE) {
                currentState = STATE_WATCHFACE;
                display_set_brightness(255);
                power_set_cpu_freq(POWER_ACTIVE);
            }
            break;
            
        case STATE_MENU_FORCE_SLEEP:
            if (event == BTN_SHORT_CLICK) {
                currentState = STATE_WATCHFACE;
            } else if (event == BTN_LONG_HOLD) {
                Serial.println("[SYS] Forced Sleep Triggered");
                display_fade_out(300);
                power_enter_deep_sleep();
                lastInteractionTime = millis();
                ui_set_dirty();
            }
            break;

        case STATE_EXECUTING_HR:
            if (event == BTN_SHORT_CLICK) {
                currentState = STATE_WATCHFACE;
                sensors_heartrate_deinit(); // Clean up I2C and turn off sensor
            }
            break;
            
        default:
            break;
    }
}

void loop() {
    // 0. Background Sensor tasks (PulseOximeter needs frequent updates)
    sensors_update();

    // 1. Process Input
    ButtonEvent event = button_update();
    
    // 2. State Machine Transitions
    handle_state_transitions(event);
    
    // 3. Battery and Charging Detection (Rule-2.5)
    float v_batt = sensors_read_battery_voltage();
    float p_batt = sensors_battery_to_percentage(v_batt);
    isCharging = (digitalRead(CHRG_PIN) == LOW);
    
    // 4. Heartrate Logic
    int currentBPM = 0;
    if (currentState == STATE_EXECUTING_HR) {
        currentBPM = sensors_read_heartrate();
        // If we have a reading, keep the screen awake longer
        if (currentBPM > 0) {
            lastInteractionTime = millis();
            ui_set_dirty();
        }
    }
    
    // 4.5 Debug BMI160 every 1 second over physical Serial
    static unsigned long last_bmi_test = 0;
    if (millis() - last_bmi_test >= 1000) {
        sensors_test_bmi160();
        last_bmi_test = millis();
    }

    // 5. Update UI
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm *timeinfo = localtime(&tv.tv_sec);
    
    ui_render(currentState, p_batt, isCharging, timeinfo, currentBPM);
    
    // 6. Auto-Sleep / AOD Control (Rule-2.3)
    if (currentState != STATE_AOD) {
        // Reduced timeout for HR measuring state? No, let's keep it 30s as per FSD
        if (millis() - lastInteractionTime >= AUTO_SLEEP_TIMEOUT) {
            if (isAODEnabled || isCharging) {
                Serial.println("[SYS] Switch to AOD Mode");
                currentState = STATE_AOD;
                display_set_brightness(30); 
                power_set_cpu_freq(POWER_AOD); 
                ui_set_dirty();
            } else {
                Serial.println("[SYS] Auto-Sleep Timeout -> Deep Sleep");
                display_fade_out(500);
                power_enter_deep_sleep();
                
                lastInteractionTime = millis();
                ui_set_dirty(); 
            }
        }
    }
    
    // 7. CPU Frequency Scaling
    if (currentState != STATE_AOD && currentState != STATE_EXECUTING_HR) {
        if (millis() - lastInteractionTime > 5000 && currentPowerMode != POWER_IDLE) {
            power_set_cpu_freq(POWER_IDLE); // 80MHz
            currentPowerMode = POWER_IDLE;
        } else if (millis() - lastInteractionTime <= 5000 && currentPowerMode != POWER_ACTIVE) {
            power_set_cpu_freq(POWER_ACTIVE); // 160MHz
            currentPowerMode = POWER_ACTIVE;
        }
    } else if (currentState == STATE_EXECUTING_HR) {
        // Force 160MHz for I2C and processing
        if (currentPowerMode != POWER_ACTIVE) {
            power_set_cpu_freq(POWER_ACTIVE);
            currentPowerMode = POWER_ACTIVE;
        }
    }
    
    // Minor delay to prevent watchdog reset or excessive loop speed
    delay(10);
}
