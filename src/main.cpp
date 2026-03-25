#include <Arduino.h>
#include <Wire.h>
#include "app_config.h"
#include "display_hal.h"
#include "button_hal.h"
#include "max30100_hal.h"
#include "ui_manager.h"
#include "power_manager.h"

void setup() {
    // 1. Core Serial
    Serial.begin(115200);
    delay(500);
    Serial.println("ESP32-C3 Smartwatch v2.0 Booting... // [DEBUG]");

    // 2. Hardware initialization
    // RULE-008: Wire.begin exactly once
    Wire.begin(PIN_SDA, PIN_SCL);
    
    // Power manager should be first to set stable clocks
    power_manager_init();
    display_hal_init();
    button_hal_init();
    
    // 3. Manager initialization
    ui_manager_init();
    
    Serial.println("System: Setup complete. // [DEBUG]");
}

void loop() {
    // Core state management
    ui_manager_update();
}
