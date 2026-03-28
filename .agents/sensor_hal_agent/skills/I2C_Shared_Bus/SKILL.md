---
name: I2C Shared Bus
description: Berbagi bus Wire yang sudah dinit oleh main.cpp tanpa konflik (RULE-008).
---

# I2C Shared Bus (RULE-008)

## Core Capabilities
- **No Double Init**: Menghindari pemanggilan `Wire.begin()` di dalam driver. Driver hanya menggunakan instance `Wire` yang sudah ada.
- **Address Management**: Memahami alamat I2C masing-masing sensor (MAX30100 dan BMI160) untuk pengalamatan data yang tepat.
- **Multi-device Conflict Avoidance**: Memastikan transaksi I2C diselesaikan dengan benar sebelum perangkat lain di bus yang sama mencoba berkomunikasi.
