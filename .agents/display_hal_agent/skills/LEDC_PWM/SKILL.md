---
name: LEDC PWM (ESP32)
description: Backlight control via ledcAttach / ledcWrite / ledcDetach. Paham channel, frequency, resolution.
---

# LEDC PWM (ESP32)

## Core Capabilities
- **Backlight Control**: Mengatur intensitas cahaya layar menggunakan peripheral LEDC ESP32.
- **ESP32 Core 3.x API**: Menggunakan `ledcAttach()`, `ledcWrite()`, dan `ledcDetach()` (sesuai update terbaru Arduino-ESP32).
- **PWM Config**: Memilih frekuensi (misal 5000Hz) dan resolusi (misal 8-bit atau 12-bit) yang optimal untuk backlight.

## Fade Effects
- **Non-blocking Fade In**: Implementasi transisi terang layar tanpa menghentikan main loop (menggunakan timingMillis).
- **Synchronous Fade Out**: Melakukan pemadaman layar secara sinkron jika diperlukan sebelum sistem memasuki state kritis.
