#include "display_hal.h"

// Instance of TFT_eSPI
static TFT_eSPI tft = TFT_eSPI();

// Backlight parameters
static const uint8_t BL_PIN = 10;
static const uint8_t BL_CHANNEL = 0;
static const uint32_t BL_FREQ = 5000;
static const uint8_t BL_RES = 8;
static uint8_t current_brightness = 0;

// LCD Pins from config
#define PIN_LCD_RES  1

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
    // 1. HARD ANALOG LOCK - Direct register level pull-down
    pinMode(BL_PIN, OUTPUT);
    digitalWrite(BL_PIN, LOW); 
    gpio_set_pull_mode((gpio_num_t)BL_PIN, GPIO_PULLDOWN_ONLY); // Hardware level insurance

    // 2. Hardware LCD Reset Sequence
    pinMode(PIN_LCD_RES, OUTPUT);
    digitalWrite(PIN_LCD_RES, LOW);
    delay(100);
    digitalWrite(PIN_LCD_RES, HIGH);
    delay(50);

    // 3. Driver INIT & Cold Masking
    tft.init();
    digitalWrite(BL_PIN, LOW); // RE-FORCE: Catch any library-induced high state
    tft.writecommand(0x28);    // DISPOFF
    
    // 4. Register Tuning (Color/Orientation)
    tft.setRotation(0); 
    tft.invertDisplay(true); 
    tft.setSwapBytes(true);  
    tft.fillScreen(TFT_BLACK); // Zero-out VRAM
    
    // 5. Backlight PWM Setup (Stay at 0)
    ledcSetup(BL_CHANNEL, BL_FREQ, BL_RES);
    ledcAttachPin(BL_PIN, BL_CHANNEL);
    ledcWrite(BL_CHANNEL, 0);

    if (Serial) Serial.println("Display HAL: ST7789 Initialized & Hard-Masked // [DEBUG]");
}

void display_hal_backlight_set(uint8_t brightness) {
    current_brightness = brightness;
    ledcWrite(BL_CHANNEL, brightness);
}

void display_hal_display_off() { tft.writecommand(0x28); }
void display_hal_display_on()  { tft.writecommand(0x29); }

void display_hal_backlight_fade_in(uint8_t target, int duration_ms) {
    if (fade_task_handle != NULL) { vTaskDelete(fade_task_handle); fade_task_handle = NULL; }
    FadeParams* params = new FadeParams{target, duration_ms};
    xTaskCreate(fade_task_worker, "BL_Fade", 2048, params, 2, &fade_task_handle);
}

void display_hal_backlight_fade_out() {
    if (fade_task_handle != NULL) { vTaskDelete(fade_task_handle); fade_task_handle = NULL; }
    
    // 1. Elegan Fade Out (Breathing out)
    uint8_t b = current_brightness;
    while (b > 5) {
        b -= 5;
        display_hal_backlight_set(b);
        delay(10);
    }
    display_hal_backlight_set(0);
    
    // 2. Scrub VRAM in the dark
    delay(50);
    tft.fillScreen(TFT_BLACK); 
    tft.writecommand(0x28); 

    // 3. Final Analog Kill
    ledcDetachPin(BL_PIN); 
    pinMode(BL_PIN, OUTPUT);
    digitalWrite(BL_PIN, LOW);
    
    if (Serial) Serial.println("Display HAL: Protected Elegant Shutdown // [DEBUG]");
}

TFT_eSPI& display_hal_get_tft() { return tft; }
