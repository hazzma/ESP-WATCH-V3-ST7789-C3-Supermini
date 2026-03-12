#ifndef SENSORS_HAL_H
#define SENSORS_HAL_H

#include <Arduino.h>
#include <Wire.h>
#include "config_pins.h"

void sensors_init();
void sensors_update();
void sensors_prepare_sleep();
float sensors_read_battery_voltage();
float sensors_battery_to_percentage(float voltage);
int sensors_read_heartrate();
void sensors_test_bmi160();

bool sensors_heartrate_init();     // On-demand start
void sensors_heartrate_deinit();   // Stop and release

#endif // SENSORS_HAL_H
