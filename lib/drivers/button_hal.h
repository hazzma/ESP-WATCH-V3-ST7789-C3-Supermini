#ifndef BUTTON_HAL_H
#define BUTTON_HAL_H

#include <Arduino.h>
#include "config_pins.h"

enum ButtonEvent {
    BTN_NONE,
    BTN_SHORT_CLICK,
    BTN_LONG_HOLD
};

void button_init();
ButtonEvent button_update();
bool button_is_pressed();

#endif // BUTTON_HAL_H
