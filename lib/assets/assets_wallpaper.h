#pragma once
#include <pgmspace.h>
#include <stdint.h>

// [STEP 5] Watchface background Architecture
// Returns dynamic buffer if available, else PROGMEM default.
const uint16_t* assets_get_wallpaper();
uint16_t assets_get_wallpaper_width();
uint16_t assets_get_wallpaper_height();

// Manual trigger to reload from LittleFS if file updated
void assets_wallpaper_clear_cache();
