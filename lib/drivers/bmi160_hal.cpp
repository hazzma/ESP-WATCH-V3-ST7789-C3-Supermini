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
#define STEP_ENABLE      0x15 // Enable step counter/detector

static void writeReg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(BMI160_ADDR);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

static uint8_t readReg(uint8_t reg) {
    Wire.beginTransmission(BMI160_ADDR);
    Wire.write(reg);
    Wire.endTransmission();
    Wire.requestFrom((int)BMI160_ADDR, (int)1);
    return Wire.read();
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

// Internal raw getter
static uint32_t get_raw_steps() {
    Wire.beginTransmission(BMI160_ADDR);
    Wire.write(REG_STEP_CNT_0);
    Wire.endTransmission();
    Wire.requestFrom((int)BMI160_ADDR, (int)2);
    
    uint16_t LSB = Wire.read();
    uint16_t MSB = Wire.read();
    return (MSB << 8) | LSB;
}

void bmi160_hal_reset_steps() {
    writeReg(REG_CMD, 0xB2); // [0xB2] STEP_CNT_CLR - Reset to 0
    if (Serial) Serial.println("[BMI160] Hardware Steps Reset // [DEBUG]");
}

uint32_t bmi160_hal_get_steps() {
    return get_raw_steps();
}
