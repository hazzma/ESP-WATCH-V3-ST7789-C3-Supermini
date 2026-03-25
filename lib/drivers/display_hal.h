#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

/**
 * @brief Initialize the display hardware and driver.
 * Follows RULE-002: No blocking delays.
 */
void display_hal_init();

/**
 * @brief Set the backlight brightness level.
 * @param brightness PWM value (0-255).
 */
void display_hal_backlight_set(uint8_t brightness);

/**
 * @brief Fade in the backlight from current value to target.
 * @param target Target brightness (0-255).
 * @param duration_ms Total fade time.
 * Follows RULE-002: Non-blocking using task/millis logic.
 */
void display_hal_backlight_fade_in(uint8_t target, int duration_ms);

/**
 * @brief Fade out the backlight and detach LEDC.
 * Follows RULE-005: Set LOW and detach to prevent current leak.
 */
void display_hal_backlight_fade_out();

/**
 * @brief Get reference to the TFT_eSPI instance.
 * @return TFT_eSPI&
 */
TFT_eSPI& display_hal_get_tft();
