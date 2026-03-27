# Functional Specification Document (FSD) - ESP32-C3 Smartwatch
## Version 3.5 (Checkpoint 6 - Final Lockdown)

### 🚨 STATUS: FULL LOCKDOWN
Seluruh file driver dan logic saat ini berada dalam status **LOCKED**. Perubahan hanya diperbolehkan per bagian dan **WAJIB SEIZIN USER**.

### 1. Input System (LOCKED)
- **Fast Mode**: 0ms latency on Menu/Watchface/Stopwatch.
- **Precision Mode**: 250ms gap allowed ONLY in Timer Setup for `BTN_RIGHT_DOUBLE`.
- **Hold**: 800ms (Always instant).
- **Checkpoint 6**: Double buffering input logic confirmed stable across all 16 states.

### 2. UI Rendering & Premium Engine (LOCKED)
- **Ghost Protection System (GPS)**: Full redraw wallpaper saat keluar dari "Black Mode".
- **Premium HR UI (Mockup Claude)**:
    - **Beating Heart**: Animasi jantung berdenyut berbasis pulse sensor.
    - **Filled Area Graph**: Grafik sejarah HR dengan bayangan (Area Chart) berbasis Sprite.
    - **Partial Redraw**: Hanya mengupdate nilai yang berubah (BPM, SpO2, Timer) untuk mencegah flicker.
- **Sprite System**: Terdiri dari 5 Sprite aktif (Clock, Status, FPS, Top Clock, Menu, Graph) dengan total RAM ~120KB (SAFE).

### 3. Sensor & Background Logic (LOCKED)
- **MAX30100 Stability**: 
    - **Burst Polling**: 5x sequential poll per cycle.
    - **History Buffer**: Internal circular buffer (60 samples) di level driver.
    - **Total Blackout**: Shutdown paksa LED via register 0x09 (0mA).
- **Silent Optimization**: Semua Serial debug dibungkus `if(Serial)` agar tidak blocking saat tanpa kabel.

### 4. Power & Sleep Management (LOCKED)
- **Anti-Sleep Guard**: Jam DILARANG tidur saat berada di mode Timer, Stopwatch, dan EXEC_HR. 
- **AOD (Always On Display)**: Backlight 15/255 saat idle di Watchface (jika aktif).
- **Snap-ON Wakeup**: Instan ON tanpa noise SPI setelah periode stabilitas VRAM.

---
*FSD v3.5 - Finalized by General Agent per User Request.*