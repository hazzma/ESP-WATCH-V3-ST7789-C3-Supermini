#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <Arduino.h>
#include <time.h>
#include "system_states.h"
#include "display_hal.h"

void ui_init();
void ui_render(SystemState state, float battery_pct, bool charging, struct tm* timeinfo, int bpm);
void ui_set_dirty(); // Signal that UI needs a redraw (Rule-2.4)

#endif // UI_MANAGER_H
