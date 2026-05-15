---
name: Non-blocking State Machine
description: Tracking press duration tanpa delay(). State: IDLE → PRESSED → HELD → RELEASED.
---

# Non-blocking State Machine

## Core Capabilities
- **State Tracking**: Mengelola transisi status tombol menggunakan variabel status internal (misal: `IDLE`, `PRESSED`, `HELD`).
- **Duration Calculation**: Menghitung lama tombol ditekan dengan mencatat `millis()` saat `PRESSED` dan membandingkannya saat `loop()` atau saat `RELEASED`.
- **Release Guard**: Memastikan sistem dapat mendeteksi apakah tombol masih dalam keadaan tertekan (penting untuk mencegah sistem masuk ke deep sleep saat user masih menekan tombol - Kepatuhan SLEEP-001).
- **Asynchronous**: Tidak menggunakan `delay()` dalam bentuk apapun, memastikan loop tetap berjalan lancar.
