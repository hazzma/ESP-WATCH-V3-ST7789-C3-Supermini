#pragma once

#include <Arduino.h>
#include "app_config.h"

typedef enum { 
    BTN_NONE, 
    BTN_SHORT_CLICK, 
    BTN_LONG_HOLD 
} ButtonEvent;

/**
 * @brief Initialize button GPIO with internal pullup.
 */
void button_hal_init();

/**
 * @brief Polling function to read button events.
 * Fire events on RELEASE only.
 * @return ButtonEvent { BTN_NONE, BTN_SHORT_CLICK, BTN_LONG_HOLD }
 */
ButtonEvent button_hal_read();
