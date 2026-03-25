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
 * @note [SENSOR AGENT] Removed Serial.println to prevent USB CDC blocking 
 * when Serial Monitor is not connected.
 */
static void onBeatDetected() {
    lastBeatTime = millis();
    // NEVER put Serial.println here for production/deep-sleep devices
}

bool max30100_hal_init() {
    // [SENSOR AGENT] We rely on the Wire.begin already called in main.cpp
    // But we keep it here just in case this init is called standalone
    Wire.begin(PIN_SDA, PIN_SCL);
    
    if (!pox.begin()) {
        return false;
    }

    pox.setIRLedCurrent(MAX30100_LED_CURR_50MA);
    pox.setOnBeatDetectedCallback(onBeatDetected);
    lastBeatTime = millis(); 

    return true;
}

void max30100_hal_update() {
    pox.update();
}

void max30100_hal_shutdown() {
    pox.shutdown();
}

float max30100_hal_get_bpm() {
    if (millis() - lastBeatTime > FINGER_OFF_TIMEOUT_MS) {
        return 0.0f;
    }
    return pox.getHeartRate();
}

uint8_t max30100_hal_get_spo2() {
    if (millis() - lastBeatTime > FINGER_OFF_TIMEOUT_MS) {
        return 0;
    }
    return pox.getSpO2();
}
