#include <Wire.h>
#include <MAX30100_PulseOximeter.h>
#include "max30100_hal.h"
#include "app_config.h"

// Registers for manual override
#define MAX30100_I2C_ADDRESS     0x57
#define MAX30100_REG_MODE_CONFIG 0x06
#define MAX30100_REG_LED_CONFIG  0x09
#define MAX30100_MODE_SHUTDOWN   0x80
#define MAX30100_MODE_RESET      0x40

// Instance of PulseOximeter
static PulseOximeter pox;
static bool sensor_active = false;
static uint32_t lastBeatTime = 0;
static bool beat_detected_ping = false;

// [SENSOR AGENT] HISTORY BUFFER
#define HR_HISTORY_SIZE 60
static float hr_history[HR_HISTORY_SIZE];
static uint32_t last_history_update = 0;

#define FINGER_OFF_TIMEOUT_MS 4000

static void writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(MAX30100_I2C_ADDRESS);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

static void onBeatDetected() {
    lastBeatTime = millis();
    beat_detected_ping = true; 
}

bool max30100_hal_init() {
    if (sensor_active) return true;

    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000); 
    
    writeRegister(MAX30100_REG_MODE_CONFIG, MAX30100_MODE_RESET);
    delay(100); 

    if (!pox.begin()) {
        return false;
    }

    pox.setIRLedCurrent(MAX30100_LED_CURR_50MA);
    pox.setOnBeatDetectedCallback(onBeatDetected);
    
    lastBeatTime = millis(); 
    sensor_active = true;
    beat_detected_ping = false;
    
    // Clear history on init
    for(int i=0; i<HR_HISTORY_SIZE; i++) hr_history[i] = 0;

    return true;
}

void max30100_hal_update() {
    if (!sensor_active) return;
    
    // Burst poll for stability
    for(int i=0; i<5; i++) {
        pox.update();
    }

    // [SENSOR AGENT] Auto-update history every 1 second
    uint32_t now = millis();
    if (now - last_history_update > 1000) {
        float current_bpm = max30100_hal_get_bpm();
        // Shift data
        for(int i=0; i < HR_HISTORY_SIZE-1; i++) {
            hr_history[i] = hr_history[i+1];
        }
        hr_history[HR_HISTORY_SIZE-1] = current_bpm;
        last_history_update = now;
    }
}

void max30100_hal_shutdown() {
    if (!sensor_active) return;
    writeRegister(MAX30100_REG_LED_CONFIG, 0x00);
    writeRegister(MAX30100_REG_MODE_CONFIG, MAX30100_MODE_SHUTDOWN);
    pox.shutdown();
    sensor_active = false;
    beat_detected_ping = false;
}

bool max30100_hal_check_beat() {
    if (beat_detected_ping) {
        beat_detected_ping = false;
        return true;
    }
    return false;
}

float max30100_hal_get_bpm() {
    if (!sensor_active || (millis() - lastBeatTime > FINGER_OFF_TIMEOUT_MS)) {
        return 0.0f;
    }
    float hr = pox.getHeartRate();
    if (hr < 40 || hr > 220) return 0.0f;
    return hr;
}

uint8_t max30100_hal_get_spo2() {
    if (!sensor_active || (millis() - lastBeatTime > FINGER_OFF_TIMEOUT_MS)) {
        return 0;
    }
    return pox.getSpO2();
}

// [SENSOR AGENT] Getter for UI Agent
void max30100_hal_get_history(float* out_buf) {
    for(int i=0; i < HR_HISTORY_SIZE; i++) {
        out_buf[i] = hr_history[i];
    }
}
