# ESP WATCH V3 ST7789 C3 Supermini - Version 1.0

Smartwatch firmware project for ESP32-C3 with ST7789 display, BMI160 IMU, and MAX30100 Heart Rate Sensor.

## 🛠 Features Implemented:
- **Apple Watch Style UI**: Stacked clock design (Red hours, White minutes).
- **Persistent Wallpaper**: Background stays across menus.
- **Improved Deep Sleep**: Uses native ESP-IDF `esp_deep_sleep_enable_gpio_wakeup` for Pin 7.
- **AOD Mode**: Always-On Display with 40MHz CPU scaling and reduced brightness.
- **Lazy-Load Sensors**: MAX30100 is only initialized when the Heart Rate menu is active to prevent boot hangs.
- **Battery Monitoring**: ADC reading with non-linear percentage calculation.

## ⚠️ Known Bugs in V1.0:

### 1. Cold Boot "Black Screen" Bug (Hardware Strapping)
- **Problem**: When power is first applied (USB/Battery), the screen remains black. The device requires a physical **RESET** button press to start.
- **Root Cause**: ESP32-C3 samples GPIO 9 at power-on. Since GPIO 8/9 are used for I2C, electrical leakage or capacitance from the sensors pulls GPIO 9 LOW during the power-up ramp, forcing the chip into "Download Mode".
- **Temporary Fix**: Press the Reset button after plugging in.
- **Future Hardware Fix**: Move I2C to non-strapping pins (e.g., GPIO 5, 20, 21) or use stronger external 3.3V pull-ups (1k-2.2k ohm).

### 2. MAX30100 Integration
- **Status**: Library switched to `MAX30100_PulseOximeter.h` (Oxullo) to match working test snippets.
- **Notice**: If initialization fails, check if the sensor is supplied with 5V or 3.3V correctly, as some modules require 5V for the IR LED.

## 🚀 Build Instructions:
- Platform: PlatformIO
- Framework: Arduino
- Board: esp32-c3-devkitm-1
- Platform version: espressif32@6.4.0
