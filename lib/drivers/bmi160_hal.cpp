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
#include "power_manager.h"
#include "calibration_manager.h"

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
#define REG_INT_EN_0     0x50
#define REG_INT_OUT_CTRL 0x53
#define REG_INT_LATCH    0x54
#define REG_INT_MAP_0    0x55
#define REG_INT_ANYMO_DUR   0x5F
#define REG_INT_ANYMO_THRES 0x60
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

static RTC_DATA_ATTR uint32_t total_steps = 0;
static RTC_DATA_ATTR uint16_t last_hw_steps = 0;
static RTC_DATA_ATTR bool rtc_valid = false;
static uint32_t last_step_poll = 0;

#define TUNE_OPTION  1   // ← ganti 1, 2, atau 3 untuk test

#if TUNE_OPTION == 1
  // [REVERT] Balik ke buffer 5 (lebih "ringan" dibanding 7)
  #define STEP_CONF_0_VAL  0x25
  #define STEP_CONF_1_VAL  0x0D
#elif TUNE_OPTION == 2
  // Opsi B: tengah-tengah (lebih sensitif dari Opsi A)
  #define STEP_CONF_0_VAL  0x39
  #define STEP_CONF_1_VAL  0x0B
#elif TUNE_OPTION == 3
  // Opsi C: threshold rendah tapi ada jeda antar step
  #define STEP_CONF_0_VAL  0x21
  #define STEP_CONF_1_VAL  0x0B
#endif

// [SENSOR AGENT] Accel data helper with Z-flip for Bottom PCB mounting
static bool bmi160_read_accel(int16_t *ax, int16_t *ay, int16_t *az) {
    uint8_t buf[6];
    if (!bmi160_read_bytes(0x12, buf, 6)) return false;
    
    *ax =  (int16_t)(buf[0] | (buf[1] << 8));
    *ay =  (int16_t)(buf[2] | (buf[3] << 8));
    *az = -(int16_t)(buf[4] | (buf[5] << 8)); // ← flip Z karena bottom PCB
    return true;
}

bool bmi160_hal_init() {
    bool is_wake = (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_UNDEFINED);
    
    Wire.setTimeOut(100); 
    Wire.beginTransmission(BMI160_ADDR);
    if (Wire.endTransmission() != 0) return false;

    uint8_t chip_id = readReg(REG_CHIP_ID);
    if (chip_id != 0xD1) return false;

    if (!is_wake) {
        writeReg(REG_CMD, 0xB6); // Soft Reset
        delay(10);
        
        writeReg(REG_CMD, 0x11); // Acc Normal (sementara, untuk init)
        delay(4);
        
        // ✅ FIX: Gyro Normal SEBELUM step conf, bukan sesudah
        writeReg(REG_CMD, 0x15); // Gyro Normal
        delay(81);

        writeReg(REG_CMD, 0xB2); // Step Counter Clear
        delay(10); // ✅ kasih waktu lebih — 2ms kadang kurang
        
        // Step Counter Configuration
        uint8_t tune = calibration_get_step_tune();
        uint8_t sc0 = 0x15, sc1 = 0x03; // Defaults (Tune 1)
        if (tune == 2) { sc0 = 0x39; sc1 = 0x0B; }
        else if (tune == 3) { sc0 = 0x21; sc1 = 0x0B; }

        writeReg(REG_STEP_CONF_0, sc0);
        writeReg(REG_STEP_CONF_1, sc1);
        delay(1);

        // ✅ FIX: Switch ke Acc Low Power — step counter BUTUH ini!
        // Acc Normal mode tidak support step counter dengan benar
        writeReg(REG_CMD, 0x12); // Acc Low Power Mode
        delay(4);

        last_hw_steps = 0;
        rtc_valid = true;

        // Interrupt config
        writeReg(REG_INT_OUT_CTRL, 0x0C); // Open-drain, active low
        delay(1);
        // Latch Mode: 0x00 (non-latched / pulse)
        // [v7.14] Ganti ke non-latched agar tidak mengunci GPIO 5
        writeReg(REG_INT_LATCH, 0x00); 
        delay(1);
        writeReg(REG_INT_MAP_0, 0x44);
        delay(1);

        // Any-motion: Threshold (0x60) & Duration (0x5F)
        uint8_t wake_thres = calibration_get_wake_threshold();
        uint8_t wake_dur   = calibration_get_wake_duration();
        uint8_t wake_axis  = calibration_get_wake_axis();

        writeReg(REG_INT_ANYMO_DUR,   wake_dur & 0x03); 
        writeReg(REG_INT_ANYMO_THRES, wake_thres);
        delay(1);

        // Orientation config (v8.1)
        writeReg(0x65, 0x18); // hysteresis + threshold
        writeReg(0x66, 0x0D); // symmetrical mode
        delay(1);

        // RTW Mode Selection
        uint8_t rtw_mode = power_manager_get_rtw_mode();
        if (rtw_mode == 1) {
            writeReg(REG_INT_EN_0, 0x40); // Enable Orientation
        } else {
            writeReg(REG_INT_EN_0, wake_axis & 0x07); // Enable Any-motion (X/Y/Z)
        }
        delay(1);

        if (Serial) Serial.println("[BMI160] INT1 Configured // [DEBUG]");

        // Disable mapping saat awake (anti ghost button)
        writeReg(REG_INT_MAP_0, 0x00);
        delay(1);
        writeReg(REG_CMD, 0x0A); // ✅ Clear latched interrupt so GPIO 5 is released!

    } else {
        // Wake path
        writeReg(REG_CMD, 0x11); // Acc Normal sebentar
        delay(4);
        writeReg(REG_CMD, 0x15); // Gyro Normal
        delay(81);

        // ✅ Kembalikan ke Low Power untuk step counter
        writeReg(REG_CMD, 0x12);
        delay(4);

        writeReg(REG_INT_MAP_0, 0x00);
        delay(1);
        writeReg(REG_CMD, 0x0A); // ✅ Clear latched interrupt on wake path


        if (!rtc_valid) {
            uint8_t buf[2];
            if (bmi160_read_bytes(REG_STEP_CNT_0, buf, 2)) {
                last_hw_steps = (uint16_t)(buf[0] | ((uint16_t)buf[1] << 8));
            }
            rtc_valid = true;
            if (Serial) Serial.println("[BMI160] RTC resynced // [DEBUG]");
        }
    }

    if (Serial) {
        Serial.printf("[BMI160] Init OK (Wake: %d) // [DEBUG]\n", is_wake);
        
        // ✅ Verifikasi step counter aktif
        uint8_t pmu = readReg(REG_PMU_STATUS);
        uint8_t sc1 = readReg(REG_STEP_CONF_1);
        Serial.printf("[BMI160] PMU=0x%02X, STEP_CONF_1=0x%02X (cnt_en=%d) // [DEBUG]\n",
                      pmu, sc1, (sc1 >> 3) & 1);
    }
    return true;
}
void bmi160_hal_shutdown() {
    if (power_manager_get_raise_to_wake()) {
        writeReg(REG_INT_MAP_0, 0x44); // Re-enable interrupt mapping (Any-Motion)
    } else {
        writeReg(REG_INT_MAP_0, 0x00); // Disable interrupt mapping so it doesn't wake
    }
    delay(1);
    
    writeReg(REG_CMD, 0x14); // Gyro Suspend
    delay(1);
    // ✅ Acc sudah di Low Power dari init, tidak perlu tulis 0x12 lagi
    // tapi tulis lagi tidak masalah sebagai safety
    writeReg(REG_CMD, 0x12); // Acc Low Power (pastikan)
    
    if (Serial) Serial.println("[BMI160] Sleep: Gyro Suspended, Acc LP // [DEBUG]");
}

void bmi160_hal_reset_steps() {
    writeReg(REG_CMD, 0xB2); // [0xB2] STEP_CNT_CLR - Reset to 0
    delay(2);  // kasih waktu hardware flush
    total_steps = 0;
    last_hw_steps = 0;
    rtc_valid = true;  // state valid setelah reset eksplisit
    if (Serial) Serial.println("[BMI160] Accumulator & Hardware Reset // [DEBUG]");
}

void bmi160_hal_update() {
    uint32_t now = millis();
    if (now - last_step_poll < 500) return; // Poll Every 500ms
    
    uint8_t buf[2];
    if (bmi160_read_bytes(REG_STEP_CNT_0, buf, 2)) {
        uint16_t current_hw = (uint16_t)(buf[0] | ((uint16_t)buf[1] << 8));
        
        uint16_t diff = current_hw - last_hw_steps;
        
        // sanity check — diff > 500 dalam 500ms itu mustahil
        // Ini tangkap kalau masih ada underflow yang lolos
        if (diff > 500) {
            if (Serial) Serial.printf(
                "[BMI160] Suspicious diff=%u (hw=%u last=%u), resyncing // [DEBUG]\n",
                diff, current_hw, last_hw_steps
            );
            last_hw_steps = current_hw;  // buang diff ini, resync saja
            last_step_poll = now;
            return;
        }
        
        total_steps += (uint32_t)diff;
        last_hw_steps = current_hw;
    }
    last_step_poll = now;
}

uint32_t bmi160_hal_get_steps() {
    return total_steps;
}

bool bmi160_hal_was_motion_wake() {
    uint8_t status0 = readReg(0x1C); // INT_STATUS[0]
    // Bit 2: Any-motion, Bit 6: Orientation
    return (status0 & 0x44) != 0;
}