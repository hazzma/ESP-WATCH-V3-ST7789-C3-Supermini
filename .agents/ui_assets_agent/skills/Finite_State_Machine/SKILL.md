---
name: Finite State Machine
description: Enum AppState, transisi dari ButtonEvent, dan global timers (millis).
---

# Finite State Machine

## Core Capabilities
- **Application State**: Mengelola state utama seperti `CLOCK_FACE`, `MENU_MAIN`, `EXEC_HR`, dan `SLEEP_PREP`.
- **Event Driven Transition**: Mengubah state berdasarkan input dari `ButtonEvent` (Short Click, Long Hold).
- **Non-blocking Timers**: 
    - Auto-sleep timer (misal 30 detik kembali ke Watchface atau sleep).
    - HR Timeout (misal 15 detik untuk pembacaan sensor).
- **Modular Logic**: Memisahkan logika "update" (perhitungan/statemachine) dan "draw" (visualisasi) untuk efisiensi.
