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
    
    // [STABILIZATION] Paling Penting: Beri waktu 2 detik murni untuk Windows
    // me-mount driver USB CDC *SEBELUM* ada data dikirim ke Serial
    delay(2000); 
    
    Serial.begin(115200);
    Serial.setTxTimeoutMs(0); // [FIX] Prevent CDC hang when buffer is full
    
    // [POWER AGENT] Priority Frequency Shift (160MHz) for CDC Stability
    power_manager_init(); 

    // Hardware initialization
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000); // [SENSOR AGENT] Turbo I2C (400kHz)
    
    if (bmi160_hal_init()) {
         if (Serial) Serial.println("Main: BMI160 Ready // [DEBUG]");
    }
    
    
    // power_manager_init(); // Moved to earlier boot phase
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
