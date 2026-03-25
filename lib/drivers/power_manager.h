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
 * @brief Read current battery voltage.
 * @return Voltage in volts (e.g., 4.2f)
 */
float power_manager_read_battery_voltage();

#endif // POWER_MANAGER_H
