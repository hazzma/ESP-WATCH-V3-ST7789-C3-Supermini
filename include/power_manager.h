#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include "esp_sleep.h"
#include "config_pins.h"
#include "system_states.h"

void power_manager_init();
void power_enter_deep_sleep();
void power_set_cpu_freq(PowerMode mode);
void power_print_wakeup_reason();

#endif // POWER_MANAGER_H
