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

/**
 * @brief Remotely request a state change (e.g., from BLE command).
 * @param st Target state.
 */
void ui_manager_request_state(AppState st);

/**
 * @brief Trigger a sensor data report to the BLE app.
 * @param command 0x04 for steps, 0x05 for battery.
 */
void ui_manager_report_sensors(uint8_t command);
