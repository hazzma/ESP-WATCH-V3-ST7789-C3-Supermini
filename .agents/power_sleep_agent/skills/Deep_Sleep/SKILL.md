---
name: ESP32 Deep Sleep
description: esp_deep_sleep_start(), EXT0 wakeup, dan PD config.
---

# ESP32 Deep Sleep

## Core Capabilities
- **Wakeup Configuration**: Menggunakan `esp_sleep_enable_ext0_wakeup()` untuk mengatur tombol sebagai pemicu bangun dari tidur.
- **Power Domain Control**: Mengatur `esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON)` jika diperlukan agar pull-up internal tetap aktif saat sleep.
- **Sleep Entry**: Memanggil `esp_deep_sleep_start()` sebagai instruksi terakhir setelah semua persiapan selesai.
- **Post-Wake Diagnostics**: Mampu mendeteksi penyebab sistem bangun (misal: wakeup dari timer vs wakeup dari pin EXT0).
