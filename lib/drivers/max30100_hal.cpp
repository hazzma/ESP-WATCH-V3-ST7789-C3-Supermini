#include <Wire.h>
#include <MAX30100_PulseOximeter.h>
#include "max30100_hal.h"
#include "app_config.h"

// Instance of PulseOximeter
static PulseOximeter pox;

// Internal variables for HR and SpO2 tracking
static uint32_t lastBeatTime = 0;
#define FINGER_OFF_TIMEOUT_MS 4000

/**
 * @brief Callback when a beat is detected.
 */
static void onBeatDetected() {
    lastBeatTime = millis();
    Serial.println("[MAX30100] Beat! // [DEBUG]");
}

bool max30100_hal_init() {
    Serial.println("[MAX30100] Init started... // [DEBUG]");

    // Use exact working pattern: Single Wire.begin followed by pox.begin
    Wire.begin(PIN_SDA, PIN_SCL);
    
    if (!pox.begin()) {
        Serial.println("[MAX30100] FAILED! // [DEBUG]");
        return false;
    }

    pox.setIRLedCurrent(MAX30100_LED_CURR_50MA);
    pox.setOnBeatDetectedCallback(onBeatDetected);
    lastBeatTime = millis(); 

    Serial.println("[MAX30100] OK - Sensor ready // [DEBUG]");
    return true;
}

void max30100_hal_update() {
    pox.update();
}

void max30100_hal_shutdown() {
    pox.shutdown();
    Serial.println("[MAX30100] Shutdown // [DEBUG]");
}

float max30100_hal_get_bpm() {
    // BUG 4: Finger-off check
    if (millis() - lastBeatTime > FINGER_OFF_TIMEOUT_MS) {
        return 0.0f;
    }
    return pox.getHeartRate();
}

uint8_t max30100_hal_get_spo2() {
    // BUG 4: Finger-off check
    if (millis() - lastBeatTime > FINGER_OFF_TIMEOUT_MS) {
        return 0;
    }
    return pox.getSpO2();
}
