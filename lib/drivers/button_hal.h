#pragma once

#include <Arduino.h>
#include "app_config.h"

typedef enum { 
    BTN_NONE, 
    BTN_LEFT_CLICK, 
    BTN_LEFT_HOLD,
    BTN_RIGHT_CLICK, 
    BTN_RIGHT_HOLD 
} ButtonEvent;

/**
 * @brief Initialize button GPIO with internal pullup.
 */
void button_hal_init();

/**
 * @brief Polling function to read button events.
 * @return ButtonEvent
 */
ButtonEvent button_hal_read();
