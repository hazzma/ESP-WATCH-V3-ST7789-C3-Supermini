---
name: GPIO Polling ESP32
description: digitalRead dengan timing millis(). Tau INPUT_PULLUP behavior di ESP32-C3.
---

# GPIO Polling ESP32

## Core Capabilities
- **Digital Read**: Mengambil status logika pin menggunakan `digitalRead()` dalam loop utama tanpa memblokir proses lain.
- **Active LOW Logic**: Memahami bahwa tombol yang menggunakan `INPUT_PULLUP` akan menghasilkan logika `LOW` saat ditekan.
- **ESP32-C3 Pinout**: Mengetahui batasan pin pada ESP32-C3 dan memastikan pin tombol dikonfigurasi dengan benar untuk mendukung wake-up EXT0 jika diperlukan.
