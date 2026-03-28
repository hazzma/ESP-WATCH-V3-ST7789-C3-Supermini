---
name: AOD Logic
description: Logika Always-On Display dengan state di RTC memory.
---

# AOD Logic

## Core Capabilities
- **Persistent State**: Menyimpan preferensi AOD (ON/OFF) di `RTC_DATA_ATTR` agar tidak hilang saat sistem bangun dari deep sleep.
- **Auto-AOD on Charging**: Logika otomatis mengaktifkan tampilan (dimmed) saat kabel charger terhubung (GPIO0 detection).
- **UI Toggle**: Menyediakan menu khusus untuk user mengubah status AOD.
- **Dimming Strategy**: Mengatur intensitas backlight ke level minimal saat di state AOD untuk efisiensi daya.
