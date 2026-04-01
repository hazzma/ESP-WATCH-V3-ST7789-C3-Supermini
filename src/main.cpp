#include <Arduino.h>
#include <Wire.h>
#include "app_config.h"
#include "display_hal.h"
#include "button_hal.h"
#include "max30100_hal.h"
#include "bmi160_hal.h"
#include "ui_manager.h"
#include "power_manager.h"
#include "ble_hal.h" // [GUARD] Bluetooth engine
#include <LittleFS.h> // [GUARD] File System

void setup() {
    // Kill Backlight immediately
    pinMode(10, OUTPUT);
    digitalWrite(10, LOW);
    
    Serial.begin(115200);
    delay(800); 

    // Hardware initialization
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000); // [SENSOR AGENT] Turbo I2C (400kHz)
    
    if (bmi160_hal_init()) {
         if (Serial) Serial.println("Main: BMI160 Ready // [DEBUG]");
    }
    
    power_manager_init();
    display_hal_init();
    button_hal_init();
    LittleFS.begin(true); // [MOUNT] Expansion Storage
    ui_manager_init();
    ble_hal_init(); // [GUARD] Bluetooth engine start
    
    if (Serial) Serial.println("System: Ready for BLE. // [DEBUG]");
}

void loop() {
    // [SELECTIVE POLLING ON-DEMAND]
    // Sensors are now polled ONLY by ui_manager_update based on active menu.
    ui_manager_update();
}
