#ifndef DISPLAY_HAL_H
#define DISPLAY_HAL_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "config_pins.h"

extern TFT_eSPI tft;

void display_init();
void display_set_brightness(uint8_t brightness); // 0-255
void display_fade_in(uint16_t duration_ms);
void display_fade_out(uint16_t duration_ms);
void display_all_off();

#endif // DISPLAY_HAL_H
