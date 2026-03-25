#include <Arduino.h>
#include <Wire.h>
#include "app_config.h"
#include "display_hal.h"
#include "button_hal.h"
#include "max30100_hal.h"
#include "ui_manager.h"
#include "power_manager.h"

void setup() {
    // DISPLAY AGENT EMERGENCY FIX: 
    // Kill Backlight immediately at start of setup to prevent flicker during Serial delay.
    // Pin 10 is PIN_LCD_BL (mapped in app_config.h)
    pinMode(10, OUTPUT);
    digitalWrite(10, LOW);

    // 1. Core Serial
    Serial.begin(115200);
    // Serial wait delay - Backlight stays black during this 500ms window
    delay(500); 
    if (Serial) Serial.println("ESP32-C3 Smartwatch v2.1 (Zero-Flicker Boot) // [DEBUG]");

    // 2. Hardware initialization
    Wire.begin(PIN_SDA, PIN_SCL);
    
    power_manager_init();
    display_hal_init();
    button_hal_init();
    
    // 3. Manager initialization
    ui_manager_init();
    
    if (Serial) Serial.println("System: Setup complete. // [DEBUG]");
}

void loop() {
    ui_manager_update();
}
