#ifndef BLE_HAL_H
#define BLE_HAL_H

#include <Arduino.h>

extern bool ble_is_syncing;
extern bool ble_is_connected;

void ble_hal_init();
void ble_hal_update();

// [GUARD] Notify-back functions
void ble_hal_notify_hr(uint8_t bpm, uint8_t spo2);
void ble_hal_notify_steps(uint32_t steps);
void ble_hal_notify_battery(uint8_t percent, bool charging);

#endif // BLE_HAL_H
