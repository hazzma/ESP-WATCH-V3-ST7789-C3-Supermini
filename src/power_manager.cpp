#include "power_manager.h"
#include <Arduino.h>
#include "esp_sleep.h"
#include "config_pins.h"
#include "driver/gpio.h"
#include "display_hal.h"

void power_manager_init() {
    // Release any previous holds
    gpio_hold_dis((gpio_num_t)BTN_PIN);
    gpio_deep_sleep_hold_dis();
    
    pinMode(BTN_PIN, INPUT_PULLUP);
}

void power_enter_deep_sleep() {
    // 1. Stability: Lock CPU freq to 160MHz before sleep transition (Rule-Gpt)
    power_set_cpu_freq(POWER_ACTIVE); 
    Serial.println(F("[PWR] Deep Sleep Initiation Sequence..."));
    
    // Per Rule-005: Close Backlight
    digitalWrite(TFT_BL, LOW);
    
    // 2. Release Guard (CRITICAL): Ensure button is released to trigger a fresh level change later
    if (digitalRead(BTN_PIN) == LOW) {
        Serial.println(F("[PWR] Release Button to Sleep..."));
        while (digitalRead(BTN_PIN) == LOW) {
            delay(10);
        }
    }
    
    // 3. Configure C3 Digital Wakeup Source (GPIO 0-21 valid for C3)
    // We use the ESP-IDF native digital wakeup circuit.
    gpio_set_direction((gpio_num_t)BTN_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode((gpio_num_t)BTN_PIN, GPIO_PULLUP_ONLY); // Support physical pullup
    
    // API specific to C3/S3 for deep sleep wakeup on digital pins
    esp_deep_sleep_enable_gpio_wakeup(1ULL << BTN_PIN, ESP_GPIO_WAKEUP_GPIO_LOW);
    
    // 4. Power Domain Logic: 
    // On ESP32-C3, RTC_PERIPH must be ON to feed the digital wakeup logic in sleep.
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    
    Serial.println(F("[PWR] Entering Deep Sleep. See you!"));
    Serial.flush();
    delay(200); 
    
    esp_deep_sleep_start();
}

void power_set_cpu_freq(PowerMode mode) {
    uint32_t freq_mhz = 160; 
    switch (mode) {
        case POWER_ACTIVE: freq_mhz = 160; break;
        case POWER_IDLE:   freq_mhz = 80;  break;
        case POWER_AOD:    freq_mhz = 40;  break;
        default:           freq_mhz = 160; break;
    }
    setCpuFrequencyMhz(freq_mhz);
    Serial.printf("[PWR] CPU Frequency: %u MHz\n", freq_mhz);
}

void power_print_wakeup_reason() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    
    switch(wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0: 
            Serial.println(F("[PWR] Wakeup: Button (EXT0/RTC)")); 
            break;
        case ESP_SLEEP_WAKEUP_GPIO: 
            Serial.println(F("[PWR] Wakeup: Digital GPIO")); 
            break;
        case ESP_SLEEP_WAKEUP_UNDEFINED:
            Serial.println(F("[PWR] Wakeup: Power-On/Reset"));
            break;
        default: 
            Serial.printf("[PWR] Wakeup code: %d\n", wakeup_reason); 
            break;
    }
}