---
name: LEDC Detach
description: Memastikan PWM timer di-detach dari pin sebelum sleep untuk mencegah leak.
---

# LEDC Detach

## Core Capabilities
- **Leakage Prevention**: Memahami bahwa pin PWM yang masih ter-attach ke hardware timer dapat mempertahankan status tegangan tertentu yang menyebabkan kebocoran arus melalui MOSFET backlight saat sleep.
- **Resource Cleanup**: Memanggil `ledcDetach()` pada channel yang digunakan untuk backlight GPIO 10.
- **State Restoration**: Mengetahui cara memasang kembali (*re-attach*) driver LEDC saat bangun dari sleep agar backlight dapat berfungsi kembali.
