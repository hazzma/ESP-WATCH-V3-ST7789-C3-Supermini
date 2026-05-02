#ifndef CALIBRATION_MANAGER_H
#define CALIBRATION_MANAGER_H

#include <Arduino.h>

struct CalibrationPacket {
    uint8_t type;            // 0x01=App->Watch, 0x02=Watch->App
    uint8_t wake_threshold;  // byte (mg/3.91)
    uint8_t wake_duration;   // 0-3 (1-4 samples)
    uint8_t wake_axis_mask;  // bitmask bit0=X, 1=Y, 2=Z
    uint8_t screen_timeout;  // seconds
    uint8_t step_cal_mode;   // 0=default, 1=manual, 2=auto
    uint16_t stride_length;  // mm
    uint8_t height_cm;
    uint8_t weight_kg;
    uint8_t step_tune_option; // 1, 2, 3
    uint8_t reserved;
    uint16_t checksum;
} __attribute__((packed));

void calibration_manager_init();
bool calibration_manager_apply(uint8_t* data, size_t len);
void calibration_manager_send_current();
void calibration_manager_reset();

// Getters for other modules
uint8_t calibration_get_wake_threshold();
uint8_t calibration_get_wake_duration();
uint8_t calibration_get_wake_axis();
uint8_t calibration_get_screen_timeout();
uint8_t calibration_get_step_tune();
uint32_t calibration_calculate_stride();

#endif // CALIBRATION_MANAGER_H
