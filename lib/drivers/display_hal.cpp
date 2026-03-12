#include "display_hal.h"

TFT_eSPI tft = TFT_eSPI();

// PWM Config for Backlight (Rule-2.3)
const uint8_t ledc_channel = 0;
const uint16_t ledc_freq = 5000;
const uint8_t ledc_res = 8;

void display_init() {
    // Initialize TFT
    tft.init();
    tft.setRotation(0); // Adjust as needed for specific hardware
    tft.setSwapBytes(true); // Fix the byte order for the wallpaper
    tft.fillScreen(TFT_BLACK);
    
    // PWM Backlight Setup (ledc)
    // ESP32-C3 uses specific ledc functions
    ledcSetup(ledc_channel, ledc_freq, ledc_res);
    ledcAttachPin(TFT_BL, ledc_channel);
    
    // Start with backlight OFF (Fade-in will trigger after boot)
    ledcWrite(ledc_channel, 0);
}

void display_set_brightness(uint8_t brightness) {
    ledcWrite(ledc_channel, brightness);
}

void display_fade_in(uint16_t duration_ms) {
    uint16_t steps = 50;
    uint32_t step_delay = duration_ms / steps;
    
    for (int i = 0; i <= 255; i += (255/steps)) {
        display_set_brightness(i);
        delay(step_delay);
    }
    display_set_brightness(255);
}

void display_fade_out(uint16_t duration_ms) {
    uint16_t steps = 50;
    uint32_t step_delay = duration_ms / steps;
    
    for (int i = 255; i >= 0; i -= (255/steps)) {
        display_set_brightness(i);
        delay(step_delay);
    }
    display_set_brightness(0);
}

void display_all_off() {
    display_set_brightness(0);
    digitalWrite(TFT_BL, LOW);
}
