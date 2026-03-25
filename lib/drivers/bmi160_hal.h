#pragma once

#include <Arduino.h>

/**
 * @brief Initialize the BMI160 stub.
 * BMI160 is RESERVED as per RULE-010.
 * @return false always.
 */
bool bmi160_hal_init();

/**
 * @brief Shutdown the BMI160 stub.
 */
void bmi160_hal_shutdown();
