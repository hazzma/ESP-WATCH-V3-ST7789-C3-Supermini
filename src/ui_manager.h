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
