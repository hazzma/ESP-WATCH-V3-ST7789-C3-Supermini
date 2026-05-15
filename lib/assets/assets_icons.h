#pragma once
#include <pgmspace.h>
#include <stdint.h>

// Charging bolt icon — 16x16, RGB565, PROGMEM
const uint16_t* assets_get_icon_charging();

// Battery icon — 16x16, RGB565, PROGMEM
const uint16_t* assets_get_icon_battery();

uint16_t assets_get_icon_width();   // All icons 16x16
uint16_t assets_get_icon_height();
