#pragma once

// GPIO Mapping
#define PIN_BTN          7   // Button (Active LOW)
#define PIN_TFT_BL      10   // Backlight
#define PIN_SDA          8
#define PIN_SCL          9
#define PIN_VBAT_ADC     3
#define PIN_CHRG         0
#define PIN_WAKE_BTN     5   // New wake-up button

// Button Timing (ms)
#define DEBOUNCE_MS           20
#define SHORT_CLICK_MAX_MS   400
#define LONG_HOLD_MIN_MS    1500

// UI Settings
#define SHOW_FPS         1
#define BL_FADE_MS       500
#define AUTO_SLEEP_MS    0
#define AOD_TIMEOUT_MS   60000  // 1 Minute to enter AOD
#define HR_TIMEOUT_MS    15000

// UI Colors
#define COLOR_WATCH_HH   0xF800 // Red
#define COLOR_WATCH_MM   0xFFFF // White
#define COLOR_WATCH_DATE 0xCE79 // Brighter Light Gray

// Power Settings
#define FREQ_HIGH     160
#define FREQ_MID       80
#define FREQ_LOW       40
#define FREQ_AOD       40
