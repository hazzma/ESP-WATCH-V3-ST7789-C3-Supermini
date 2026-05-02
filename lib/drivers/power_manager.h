#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <stdint.h>

/**
 * @brief Initialize Power Management unit.
 */
void power_manager_init();

/**
 * @brief Set CPU frequency.
 * @param mhz Target frequency (e.g., 160, 80, 40, 20, 10)
 */
void power_manager_set_freq(int mhz);

/**
 * @brief Enter deep sleep with pre-defined 8-step sequence.
 */
void power_manager_enter_deep_sleep();

/**
 * @brief Read current battery voltage with optional load compensation.
 * @param apply_compensation If true, adds back the measured 60mV LCD sag (v6.6).
 * @return Filtered voltage (EMA) in volts.
 */
float power_manager_read_battery_voltage(bool apply_compensation = false);

/**
 * @brief Check if USB is charging.
 * @return True if charging.
 */
bool power_manager_is_charging();

/**
 * @brief Manage Auto Sleep logic.
 * @param seconds Timeout in seconds. Use 0 to disable.
 */
void power_manager_set_auto_sleep_timeout(uint32_t seconds);
uint32_t power_manager_get_auto_sleep_timeout();

bool power_manager_get_raise_to_wake();
void power_manager_set_raise_to_wake(bool enabled);

uint8_t power_manager_get_rtw_sensitivity();
void power_manager_set_rtw_sensitivity(uint8_t level);

uint8_t power_manager_get_rtw_mode();
void power_manager_set_rtw_mode(uint8_t mode);

bool power_manager_get_ble_enabled();
void power_manager_set_ble_enabled(bool enabled);

#endif // POWER_MANAGER_H
