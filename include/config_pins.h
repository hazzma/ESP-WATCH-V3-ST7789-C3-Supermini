#ifndef CONFIG_PINS_H
#define CONFIG_PINS_H

#include <Arduino.h>

// SPI TFT Display Pins
#define TFT_MOSI 6
#define TFT_SCLK 4
#define TFT_DC   2
#define TFT_RST  1
#define TFT_CS   -1  // Hardwired to GND
#define TFT_BL   10  // Backlight via MOSFET (PWM)

// I2C Bus Pins
#define I2C_SDA  8   
#define I2C_SCL  9

// Input Pins
#define BTN_PIN     7   // Single Push Button (EXT0 Wake)
#define CHRG_PIN    0   // Charging Status (WARNING: Boot Strap!)

// Analog Pins
#define B_READ_PIN  3   // Battery ADC (Voltage Divider)

#endif // CONFIG_PINS_H
