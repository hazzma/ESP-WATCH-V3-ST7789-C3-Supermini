#include "power_manager.h"
#include <Arduino.h>
#include <esp_sleep.h>
#include <esp_pm.h>
#include "app_config.h"
#include "max30100_hal.h"
#include "bmi160_hal.h"
#include "display_hal.h"

void power_manager_init() {
    if (Serial) Serial.println("[POWER] Initializing Power Manager (vCalibration)... // [DEBUG]");
    
    // Set ADC Attenuation to 11dB (allows reading up to ~3.1V on pin)
    analogSetAttenuation(ADC_11db);
    
    // Enable RTC domain power during sleep to allow certain wakeups
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    
    // [POWER AGENT] Smart Logic: 
    // - Cold Boot (UNDEFINED): Stay at 160MHz for CDC initial handshake stability.
    // - Regular Wakeup: Skip 160MHz, go straight to 80MHz (Sync PWM/SPI Bus).
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause == ESP_SLEEP_WAKEUP_UNDEFINED) {
        power_manager_set_freq(FREQ_HIGH); // Initial peak for CDC
    } else {
        power_manager_set_freq(FREQ_MID);  // Direct to cruise speed
    }
}

void power_manager_set_freq(int mhz) {
    if (mhz == getCpuFrequencyMhz()) return;
    
    setCpuFrequencyMhz(mhz);
    if (Serial) Serial.printf("[POWER] DFS: CPU Frequency set to %d MHz // [DEBUG]\n", mhz);
}

void power_manager_enter_deep_sleep() {
    if (Serial) Serial.println("[POWER] Executing Pre-Sleep sequence... // [DEBUG]");

    // 1. Spin-wait sampai GPIO 5 = HIGH (release guard) — RULE-006
    pinMode(PIN_BTN_RIGHT, INPUT_PULLUP);
    while(digitalRead(PIN_BTN_RIGHT) == LOW) {
        delay(1); 
    }

    // 2. max30100_hal_shutdown() — RULE-004
    max30100_hal_shutdown();

    // 3. bmi160_hal_shutdown() — RULE-004
    bmi160_hal_shutdown();

    // 4. display_hal_backlight_fade_out() — RULE-005
    display_hal_backlight_fade_out();

    // 5. digitalWrite(10, LOW) — RULE-005
    pinMode(PIN_TFT_BL, OUTPUT);
    digitalWrite(PIN_TFT_BL, LOW);

    // 6. ledcDetachPin(10) — RULE-005
    ledcDetachPin(PIN_TFT_BL);

    // 7. esp_deep_sleep_enable_gpio_wakeup(1ULL << 5, ESP_GPIO_WAKEUP_GPIO_LOW) — RULE-006
    esp_deep_sleep_enable_gpio_wakeup(1ULL << PIN_BTN_RIGHT, ESP_GPIO_WAKEUP_GPIO_LOW);

    // 8. esp_deep_sleep_start() — RULE-001
    if (Serial) {
        Serial.println("\n[!] ========================================");
        Serial.println("[!]      SYSTEM: ENTERING DEEP SLEEP        ");
        Serial.println("[!] ========================================\n");
    }
    delay(100);
    esp_deep_sleep_start();
}

float power_manager_read_battery_voltage() {
    static uint32_t last_v_print = 0;
    
    // analogReadMilliVolts() use internal factory calibration (Vref)
    uint32_t raw_mv = analogReadMilliVolts(PIN_VBAT_ADC);
    
    // [CALIB 2-POINT] Interpolation (Sanwa Lab Verified Final - v6.5.1)
    // Point 1: 1964mV -> 3.90V | Point 2: 2061mV -> 4.10V
    float voltage = 3.90f + ((float)raw_mv - 1964.0f) * 0.0020618f;
    
    uint32_t now = millis();
    if (now - last_v_print > 2000) {
        if (Serial) {
            // [DIAGNOSTIC] Sanwa v6.5.1 Consistent Curve
            float v_log = voltage;
            int pct = 0;
            if (v_log >= 4.10f) pct = 100;
            else if (v_log <= 3.35f) pct = 0;
            else if (v_log > 3.90f) pct = (int)(80 + (v_log - 3.90f) / (4.10f - 3.90f) * 20);
            else if (v_log > 3.65f) pct = (int)(40 + (v_log - 3.65f) / (3.90f - 3.65f) * 40);
            else pct = (int)((v_log - 3.35f) / (3.65f - 3.35f) * 40);

            Serial.printf("[POWER] ADC: %dmV | VBAT: %.3fV | BATT: %d%% // [DIAGNOSTIC]\n", raw_mv, voltage, pct);
        }
        last_v_print = now;
    }
    return voltage;
}

bool power_manager_is_charging() {
    float v = power_manager_read_battery_voltage();
    // 1. Check hardware pin (Active LOW)
    pinMode(PIN_CHRG, INPUT_PULLUP);
    bool pin_chrg = (digitalRead(PIN_CHRG) == LOW);
    
    // 2. Voltage heuristic (V > 4.16V is definitely charging on SuperMini/Aneng)
    if (v > 4.16f || pin_chrg) return true;
    return false;
}

// Persist Auto Sleep Timeout (Default: 15s)
static RTC_DATA_ATTR uint32_t auto_sleep_sec = 15;

void power_manager_set_auto_sleep_timeout(uint32_t seconds) {
    auto_sleep_sec = seconds;
    if (Serial) Serial.printf("[POWER] Auto-Sleep set to %d seconds // [DEBUG]\n", seconds);
}

uint32_t power_manager_get_auto_sleep_timeout() {
    return auto_sleep_sec;
}
