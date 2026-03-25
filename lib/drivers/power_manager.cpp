#include "power_manager.h"
#include <Arduino.h>
#include <esp_sleep.h>
#include <esp_pm.h>
#include "app_config.h"
#include "max30100_hal.h"
#include "bmi160_hal.h"
#include "display_hal.h"

void power_manager_init() {
    Serial.println("[POWER] Initializing Power Manager... // [DEBUG]");
    
    // Enable RTC domain power during sleep to allow certain wakeups
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    
    // Set initial frequency (standard 80MHz for startup)
    power_manager_set_freq(FREQ_MID);
}

void power_manager_set_freq(int mhz) {
    if (mhz == getCpuFrequencyMhz()) return;
    
    setCpuFrequencyMhz(mhz);
    Serial.printf("[POWER] DFS: CPU Frequency set to %d MHz // [DEBUG]\n", mhz);
}

void power_manager_enter_deep_sleep() {
    Serial.println("[POWER] Executing Pre-Sleep sequence... // [DEBUG]");

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
    Serial.println("[POWER] System entering Deep Sleep now. // [DEBUG]");
    delay(100);
    esp_deep_sleep_start();
}

float power_manager_read_battery_voltage() {
    // Formula: (analogRead(PIN_BATT_ADC) / 4095.0) * 3.3 * 2.0
    int raw = analogRead(PIN_VBAT_ADC);
    float voltage = (raw / 4095.0f) * 3.3f * 2.0f;
    return voltage;
}
