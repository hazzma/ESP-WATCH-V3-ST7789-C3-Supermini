/* 
 * 🛑 [CRITICAL: SENSOR AGENT ONLY] 🛑
 * ─────────────────────────────────────────────────────────────
 * WARNING: SACRED HARDWARE DRIVER (BMI160 BARE-METAL)
 * ─────────────────────────────────────────────────────────────
 * DO NOT MODIFY THIS FILE UNLESS YOU ARE THE SENSOR AGENT.
 * 
 * This file contains the exact 'REPEATED START' I2C protocol 
 * (endTransmission false) calibrated for ESP32-C3 hardware. 
 * Any alteration by other agents will result in garbage data 
 * and pedometer desync.
 * ─────────────────────────────────────────────────────────────
 */
#include "bmi160_hal.h"
#include <Wire.h>
#include <esp_sleep.h>

#define BMI160_ADDR 0x69

// Registers
#define REG_CHIP_ID      0x00
#define REG_ERR          0x02
#define REG_PMU_STATUS   0x03
#define REG_ACC_CONF     0x40
#define REG_ACC_RANGE    0x41
#define REG_STEP_CNT_0   0x78
#define REG_STEP_CNT_1   0x79
#define REG_STEP_CONF_0  0x7A
#define REG_STEP_CONF_1  0x7B
#define REG_CMD          0x7E

// Values
#define CMD_SOFT_RESET   0xB6
#define CMD_ACC_NORMAL   0x11
#define STEP_ENABLE      0x15 

// [SENSOR AGENT] Verified Bare-Metal I2C Helpers
static bool bmi160_write_reg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(BMI160_ADDR);
    Wire.write(reg);
    Wire.write(val);
    uint8_t err = Wire.endTransmission();
    if (err != 0) {
        if (Serial) Serial.printf("[BMI160] I2C Write Error: %d // [DEBUG]\n", err);
        return false;
    }
    return true;
}

static bool bmi160_read_bytes(uint8_t reg, uint8_t *buf, uint8_t len) {
    Wire.beginTransmission(BMI160_ADDR);
    Wire.write(reg);
    // Important: REPEATED START. Returns 0 on success.
    uint8_t err = Wire.endTransmission(false);
    if (err != 0) {
        // If bus hangs, don't block. Return immediately.
        return false; 
    }
    
    uint8_t received = Wire.requestFrom((uint8_t)BMI160_ADDR, len);
    if (received != len) {
        // Clear buffer just in case
        while(Wire.available()) Wire.read();
        return false;
    }
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = Wire.read();
    }
    return true;
}

static uint8_t readReg(uint8_t reg) {
    uint8_t val = 0;
    bmi160_read_bytes(reg, &val, 1);
    return val;
}

static void writeReg(uint8_t reg, uint8_t val) {
    bmi160_write_reg(reg, val);
}

bool bmi160_hal_init() {
    bool is_wake = (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_UNDEFINED);

    // 1. Verify Chip ID
    uint8_t chip_id = readReg(REG_CHIP_ID);
    if (chip_id != 0xD1) return false;

    // 2. Full Init ONLY on cold boot
    if (!is_wake) {
        writeReg(REG_CMD, 0xB6); // Soft Reset
        delay(10);
        
        writeReg(REG_CMD, 0x11); // Acc Normal
        delay(4);
        
        writeReg(REG_CMD, 0x15); // Gyro Normal
        delay(81);
        
        writeReg(REG_CMD, 0xB2); // Step Reset
        delay(1);
        
        writeReg(REG_STEP_CONF_0, 0x15);
        delay(1);
        writeReg(REG_STEP_CONF_1, 0x0B);
        delay(1);
    } else {
        // Just re-enable on Wake
        writeReg(REG_CMD, 0x11);
        delay(4);
        writeReg(REG_STEP_CONF_0, 0x15);
        writeReg(REG_STEP_CONF_1, 0x0B);
    }

    if (Serial) Serial.printf("[BMI160] Init OK (Address: 0x69, Wake: %d) // [DEBUG]\n", is_wake);
    return true;
}

void bmi160_hal_shutdown() {
    writeReg(REG_CMD, 0x10); // Suspend Accel
    if (Serial) Serial.println("[BMI160] Suspended // [DEBUG]");
}

void bmi160_hal_reset_steps() {
    writeReg(REG_CMD, 0xB2); // [0xB2] STEP_CNT_CLR - Reset to 0
    if (Serial) Serial.println("[BMI160] Hardware Steps Reset // [DEBUG]");
}

static uint32_t cached_steps = 0;
static uint32_t last_step_poll = 0;

void bmi160_hal_update() {
    uint32_t now = millis();
    if (now - last_step_poll < 500) return; // Poll Every 500ms
    
    uint8_t buf[2];
    if (bmi160_read_bytes(REG_STEP_CNT_0, buf, 2)) {
        cached_steps = (uint32_t)(buf[0] | ((uint16_t)buf[1] << 8));
    }
    last_step_poll = now;
}

uint32_t bmi160_hal_get_steps() {
    return cached_steps;
}
