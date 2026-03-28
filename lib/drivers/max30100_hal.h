#ifndef MAX30100_HAL_H
#define MAX30100_HAL_H

#include <Arduino.h>

/**
 * @brief Initialize the MAX30100 Heart Rate and SpO2 sensor.
 */
bool max30100_hal_init();

/**
 * @brief Run the internal sampling and history update loop.
 */
void max30100_hal_update();

/**
 * @brief Put the sensor in power-down mode (Turn IR/RED LEDs off).
 */
void max30100_hal_shutdown();

/**
 * @brief Get the current heart rate (BPM).
 */
float max30100_hal_get_bpm();

/**
 * @brief Get the current blood oxygen level (SpO2).
 */
uint8_t max30100_hal_get_spo2();

/**
 * @brief Check if a heart beat has been detected.
 */
bool max30100_hal_check_beat();

/**
 * @brief Get the last 60 HR samples for UI graphing.
 */
void max30100_hal_get_history(float* out_buf);

#endif
