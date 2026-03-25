#pragma once

#include <Arduino.h>

/**
 * @brief Initialize the MAX30100 sensor.
 * Lazy init - HANYA dipanggil saat masuk state EXEC_HR.
 * @return true if initialization successful, false otherwise.
 */
bool max30100_hal_init();

/**
 * @brief Polling function to update MAX30100 data.
 * Wajib dipanggil setiap loop selama EXEC_HR aktif.
 */
void max30100_hal_update();

/**
 * @brief Put MAX30100 into shutdown mode to save power.
 */
void max30100_hal_shutdown();

/**
 * @brief Get the latest BPM (Beats Per Minute) value.
 * @return float BPM value.
 */
float max30100_hal_get_bpm();

/**
 * @brief Get the latest SpO2 (Oxygen Saturation) value.
 * @return uint8_t SpO2 percentage.
 */
uint8_t max30100_hal_get_spo2();
