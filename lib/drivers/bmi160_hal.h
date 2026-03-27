#pragma once

#include <Arduino.h>

/**
 * @brief Initialize the BMI160 stub.
 * BMI160 is RESERVED as per RULE-010.
 * @return false always.
 */
bool bmi160_hal_init();

/**
 * @brief Shutdown the BMI160.
 */
void bmi160_hal_shutdown();

/**
 * @brief Reset the step counter.
 */
void bmi160_hal_reset_steps();

/**
 * @brief Get the current step count (offset corrected).
 * @return Total steps.
 */
uint32_t bmi160_hal_get_steps();
