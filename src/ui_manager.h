#pragma once

#include <Arduino.h>
#include "system_states.h"
#include "button_hal.h"

/**
 * @brief Initialize UI Manager and load persistent settings.
 */
void ui_manager_init();

/**
 * @brief Core UI/State Machine update function.
 * Handles inputs and rendering. Call every loop.
 */
void ui_manager_update();

/**
 * @brief Get current AOD setting.
 * @return true if AOD is enabled.
 */
bool ui_manager_is_aod_enabled();
/**
 * @brief Check if UI is under high CPU/RAM load (e.g., transitions).
 * @return true if busy.
 */
bool ui_manager_is_high_load();
/**
 * @brief Get current CPU frequency managed by UI.
 * @return Frequency in MHz.
 */
uint16_t ui_manager_get_cpu_freq();
