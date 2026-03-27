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
    
    // Set initial frequency (standard 80MHz for startup)
    power_manager_set_freq(FREQ_MID);
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
    if (Serial) Serial.println("[POWER] System entering Deep Sleep now. // [DEBUG]");
    delay(100);
    esp_deep_sleep_start();
}

float power_manager_read_battery_voltage() {
    static uint32_t last_v_print = 0;
    
    // analogReadMilliVolts() use internal factory calibration (Vref)
    uint32_t raw_mv = analogReadMilliVolts(PIN_VBAT_ADC);
    float voltage = (raw_mv / 1000.0f) * 1.418f; // [CALIB] Sanwa-verified Multiplier (4.09V/2884mV)
    
    uint32_t now = millis();
    if (now - last_v_print > 2000) {
        if (Serial) Serial.printf("[POWER] VBAT mV: %d | Calibrated: %.3f V // [FINAL]\n", raw_mv, voltage);
        last_v_print = now;
    }
    return voltage;
}

bool power_manager_is_charging() {
    float v = power_manager_read_battery_voltage();
    // 1. Check hardware pin (Active LOW)
    pinMode(PIN_CHRG, INPUT_PULLUP);
    bool pin_chrg = (digitalRead(PIN_CHRG) == LOW);
    
    // 2. Voltage heuristic (V > 4.4V is usually charging on SuperMini)
    if (v > 4.35f || pin_chrg) return true;
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
