#include "display_hal.h"

// Instance of TFT_eSPI
static TFT_eSPI tft = TFT_eSPI();

// Backlight parameters
static const uint8_t BL_PIN = 10;
static const uint8_t BL_CHANNEL = 0;
static const uint32_t BL_FREQ = 5000;
static const uint8_t BL_RES = 8;
static uint8_t current_brightness = 0;

// Task handle for fade operation
static TaskHandle_t fade_task_handle = NULL;

struct FadeParams {
    uint8_t target;
    int duration;
};

/**
 * @brief Internal task to handle non-blocking fade.
 */
static void fade_task_worker(void* pvParameters) {
    FadeParams* params = (FadeParams*)pvParameters;
    uint8_t start = current_brightness;
    uint8_t target = params->target;
    int duration = params->duration;
    
    if (duration <= 0) {
        display_hal_backlight_set(target);
    } else {
        unsigned long start_time = millis();
        while (millis() - start_time < (unsigned long)duration) {
            float progress = (float)(millis() - start_time) / duration;
            uint8_t val = start + (uint8_t)((target - start) * progress);
            display_hal_backlight_set(val);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        display_hal_backlight_set(target);
    }
    
    delete params;
    fade_task_handle = NULL;
    vTaskDelete(NULL);
}

void display_hal_init() {
    // Initialize SPI and Driver
    tft.init();
    tft.setRotation(0); 
    tft.invertDisplay(true); // Invert colors (Required for most 1.69" / 240x280 ST7789)
    tft.setSwapBytes(true);  // Swap bytes for RGB565 word order
    tft.fillScreen(TFT_BLACK);
    
    // PWM Configuration for Backlight (ESP32-C3)
    // Using 2.x API as per espressif32@6.4.0 constraint
    ledcSetup(BL_CHANNEL, BL_FREQ, BL_RES);
    ledcAttachPin(BL_PIN, BL_CHANNEL);
    display_hal_backlight_set(0); 
    
    Serial.println("Display HAL: Initialized ST7789 240x280 // [DEBUG]");
}

void display_hal_backlight_set(uint8_t brightness) {
    current_brightness = brightness;
    ledcWrite(BL_CHANNEL, brightness);
}

void display_hal_backlight_fade_in(uint8_t target, int duration_ms) {
    // Cancel existing fade if any
    if (fade_task_handle != NULL) {
        vTaskDelete(fade_task_handle);
        fade_task_handle = NULL;
    }
    
    FadeParams* params = new FadeParams{target, duration_ms};
    
    // Create non-blocking task for fade in
    BaseType_t res = xTaskCreate(
        fade_task_worker, 
        "BL_Fade", 
        2048, 
        params, 
        1, 
        &fade_task_handle
    );
    
    if (res != pdPASS) {
        delete params;
        display_hal_backlight_set(target); // Fallback to instant set
    }
    
    Serial.printf("Display HAL: Fade in to %d start (%d ms) // [DEBUG]\n", target, duration_ms);
}

void display_hal_backlight_fade_out() {
    // Stop any active fade in
    if (fade_task_handle != NULL) {
        vTaskDelete(fade_task_handle);
        fade_task_handle = NULL;
    }

    // Synchronous fade out before sleep (skill specific behavior)
    uint8_t b = current_brightness;
    while (b > 0) {
        b = (b > 10) ? (b - 10) : 0;
        display_hal_backlight_set(b);
        delay(15); // Small sync delay allowed for shutdown sequence
    }
    
    // RULE-005: Close MOSFET leak path
    digitalWrite(BL_PIN, LOW);
    ledcDetachPin(BL_PIN); 
    
    Serial.println("Display HAL: Backlight Detached (RULE-005) // [DEBUG]");
}

TFT_eSPI& display_hal_get_tft() {
    return tft;
}
