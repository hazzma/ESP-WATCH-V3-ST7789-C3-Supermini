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

#endif // POWER_MANAGER_H
