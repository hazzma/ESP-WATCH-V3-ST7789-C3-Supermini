#include "calibration_manager.h"
#include <Preferences.h>
#include "ble_hal.h"
#include "bmi160_hal.h"
#include "power_manager.h"

// Characteristic pointer from ble_hal (I'll need to expose this or add a notify function)
extern void ble_hal_notify_calibration(uint8_t* data, size_t len);

static Preferences prefs;

void calibration_manager_init() {
    prefs.begin("c3watch_cal", false);
    // If not initialized, set defaults
    if (!prefs.isKey("wake_thres")) {
        calibration_manager_reset();
    }
}

static uint16_t calc_checksum(uint8_t* data, size_t len) {
    uint16_t sum = 0;
    for (size_t i = 0; i < len; i++) sum += data[i];
    return sum;
}

bool calibration_manager_apply(uint8_t* data, size_t len) {
    if (len != 14) return false;
    CalibrationPacket* p = (CalibrationPacket*)data;
    
    // Validate checksum
    uint16_t sum = calc_checksum(data, 12);
    if (sum != p->checksum) {
        if (Serial) Serial.printf("CAL: Checksum Fail! Got %d, Exp %d\n", p->checksum, sum);
        return false;
    }

    // Store in NVS
    prefs.putUChar("wake_thres", p->wake_threshold);
    prefs.putUChar("wake_dur", p->wake_duration);
    prefs.putUChar("wake_axis", p->wake_axis_mask);
    prefs.putUChar("scr_timeout", p->screen_timeout);
    prefs.putUChar("step_mode", p->step_cal_mode);
    prefs.putUShort("stride_mm", p->stride_length);
    prefs.putUChar("height_cm", p->height_cm);
    prefs.putUChar("weight_kg", p->weight_kg);
    prefs.putUChar("tune_opt", p->step_tune_option);

    if (Serial) Serial.println("CAL: Settings saved to NVS // [SUCCESS]");

    // Apply to hardware immediately
    bmi160_hal_init(); // [RE-INIT] applies new calibration
    power_manager_set_auto_sleep_timeout(p->screen_timeout);

    // Echo back
    calibration_manager_send_current();
    return true;
}

void calibration_manager_send_current() {
    CalibrationPacket p;
    p.type = 0x02;
    p.wake_threshold = prefs.getUChar("wake_thres", 64);
    p.wake_duration = prefs.getUChar("wake_dur", 3);
    p.wake_axis_mask = prefs.getUChar("wake_axis", 0x07);
    p.screen_timeout = prefs.getUChar("scr_timeout", 10);
    p.step_cal_mode = prefs.getUChar("step_mode", 0x00);
    p.stride_length = prefs.getUShort("stride_mm", 700);
    p.height_cm = prefs.getUChar("height_cm", 165);
    p.weight_kg = prefs.getUChar("weight_kg", 60);
    p.step_tune_option = prefs.getUChar("tune_opt", 1);
    p.reserved = 0;
    p.checksum = calc_checksum((uint8_t*)&p, 12);

    ble_hal_notify_calibration((uint8_t*)&p, 14);
}

void calibration_manager_reset() {
    prefs.putUChar("wake_thres", 64);
    prefs.putUChar("wake_dur", 3);
    prefs.putUChar("wake_axis", 0x07);
    prefs.putUChar("scr_timeout", 10);
    prefs.putUChar("step_mode", 0x00);
    prefs.putUShort("stride_mm", 700);
    prefs.putUChar("height_cm", 165);
    prefs.putUChar("weight_kg", 60);
    prefs.putUChar("tune_opt", 1);
    
    bmi160_hal_init();
    power_manager_set_auto_sleep_timeout(10);
    
    if (Serial) Serial.println("CAL: Reset to Factory Defaults // [DONE]");
    calibration_manager_send_current();
}

uint8_t calibration_get_wake_threshold() { return prefs.getUChar("wake_thres", 64); }
uint8_t calibration_get_wake_duration() { return prefs.getUChar("wake_dur", 3); }
uint8_t calibration_get_wake_axis() { return prefs.getUChar("wake_axis", 0x07); }
uint8_t calibration_get_screen_timeout() { return prefs.getUChar("scr_timeout", 10); }
uint8_t calibration_get_step_tune() { return prefs.getUChar("tune_opt", 1); }

uint32_t calibration_calculate_stride() {
    uint8_t mode = prefs.getUChar("step_mode", 0x00);
    if (mode == 0x01) return prefs.getUShort("stride_mm", 700);
    if (mode == 0x02) {
        uint8_t height = prefs.getUChar("height_cm", 165);
        return (uint32_t)(height * 4.14f); // height_cm * 0.414 * 10 (for mm)
    }
    return 700; // Default
}
