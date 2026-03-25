#ifndef DISPLAY_HAL_H
#define DISPLAY_HAL_H

#include <TFT_eSPI.h>

/**
 * @brief Initialize Display and Backlight PWM.
 */
void display_hal_init();

/**
 * @brief Get reference to the TFT instance.
 */
TFT_eSPI& display_hal_get_tft();

/**
 * @brief Set Backlight level (0-255).
 */
void display_hal_backlight_set(uint8_t brightness);

/**
 * @brief Cinematic fade in for Backlight (Non-blocking).
 */
void display_hal_backlight_fade_in(uint8_t target, int duration_ms);

/**
 * @brief Fade out and detach PWM/VCC for sleep.
 */
void display_hal_backlight_fade_out();

/**
 * @brief Display OFF - Software mask to panel (Data remains in VRAM).
 */
void display_hal_display_off();

/**
 * @brief Display ON - Release mask to panel.
 */
void display_hal_display_on();

#endif
