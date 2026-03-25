#include <Wire.h>
#include <MAX30100_PulseOximeter.h>
#include "max30100_hal.h"
#include "app_config.h"

// MAX30100 Registers for manual override
#define MAX30100_I2C_ADDRESS     0x57
#define MAX30100_REG_MODE_CONFIG 0x06
#define MAX30100_MODE_SHUTDOWN   0x80
#define MAX30100_MODE_RESET      0x40

// Instance of PulseOximeter
static PulseOximeter pox;
static volatile bool sensor_running = false;
static TaskHandle_t pax_task_handle = NULL;

// Internal variables for HR and SpO2 tracking
static uint32_t lastBeatTime = 0;
#define FINGER_OFF_TIMEOUT_MS 4000

/**
 * @brief Helper to write to I2C register
 */
static void writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(MAX30100_I2C_ADDRESS);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

static void onBeatDetected() {
    lastBeatTime = millis();
}

/**
 * @brief Background Task Worker (100Hz polling)
 * [SENSOR AGENT] This decouples sensor updates from UI rendering latency.
 */
static void pox_task_worker(void* pvParameters) {
    while (sensor_running) {
        pox.update();
        vTaskDelay(pdMS_TO_TICKS(15)); // Poll at ~66Hz for stability
    }
    pax_task_handle = NULL;
    vTaskDelete(NULL);
}

bool max30100_hal_init() {
    if (sensor_running) return true;

    Wire.begin(PIN_SDA, PIN_SCL);
    
    // HARD RESET hardware chip
    writeRegister(MAX30100_REG_MODE_CONFIG, MAX30100_MODE_RESET);
    delay(50); 

    if (!pox.begin()) {
        if (Serial) Serial.println("[MAX30100] FAILED to start // [DEBUG]");
        return false;
    }

    pox.setIRLedCurrent(MAX30100_LED_CURR_50MA);
    pox.setOnBeatDetectedCallback(onBeatDetected);
    
    lastBeatTime = millis(); 
    sensor_running = true;

    // Create background task for sensor polling
    xTaskCreate(pox_task_worker, "POX_Task", 4096, NULL, 3, &pax_task_handle);

    return true;
}

void max30100_hal_update() {
    // Empty: Managed by background task for precision
}

void max30100_hal_shutdown() {
    if (!sensor_running) return;

    sensor_running = false;
    // Wait for task to clean itself up (minimal delay)
    vTaskDelay(pdMS_TO_TICKS(50));

    pox.shutdown();
    writeRegister(MAX30100_REG_MODE_CONFIG, MAX30100_MODE_SHUTDOWN);
    
    lastBeatTime = 0;
}

float max30100_hal_get_bpm() {
    if (!sensor_running || (millis() - lastBeatTime > FINGER_OFF_TIMEOUT_MS)) {
        return 0.0f;
    }
    return pox.getHeartRate();
}

uint8_t max30100_hal_get_spo2() {
    if (!sensor_running || (millis() - lastBeatTime > FINGER_OFF_TIMEOUT_MS)) {
        return 0;
    }
    return pox.getSpO2();
}
