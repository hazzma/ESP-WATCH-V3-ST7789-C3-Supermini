# MASTER KNOWLEDGE BASE — ESP32-C3 SMARTWATCH (V3.5)
*Reference document for development phase 2 (Step Counter, BLE, App Interface)*

## 1. Hardware Architecture (LOCKED)
- **MCU**: ESP32-C3 SuperMini (Core 0 only).
- **Display**: ST7789 TFT (240x280) — SPI (MOSI:6, SCK:4, DC:2, RST:1, BL:10 PWM).
- **I2C Shared**: SDA:8, SCL:9 (Used by MAX30100 & BMI160).
- **Buttons**: Left(7), Right(5) — Active LOW (Internal Pull-up).

## 2. System States Logic (include/system_states.h)
Menu terbagi menjadi 16 states utama:
- **Watchface**: Home screen.
- **Menu HR (Page 1)** -> **Exec HR**: Premium monitoring with graphs.
- **Menu Timer (Page 2)** -> **Setup** -> **Exec Timer**: Deep sleep protected.
- **Menu Stopwatch (Page 3)** -> **Exec SW**: High precision.
- **Settings**: Timeout, Brightness, AOD Toggle.

## 3. High-Performance UI (ui_manager.cpp)
- **Sprite Optimized**: Clock(180x150), Menu(200x100), Graph(208x80).
- **Zero-Flicker Strategy**:
    1. Mask backlight (Set 0) during init/wake.
    2. Scrub VRAM using 2x sequential render calls.
    3. SNAP-ON backlight to hide display noise.
- **Partial Refresh**: Only updates numerical values or specific graph lines during measurement to save CPU cycles.

## 4. Sensor Protection (max30100_hal.cpp)
- **Burst Polling**: Menghindari overflow FIFO dengan memanggil polling 5x tiap siklus.
- **History Data**: Tersedia buffer 60 detik HR (`max30100_hal_get_history`) untuk rendering grafik.
- **Silent Mode**: Driver otomatis mendeteksi koneksi USB (`if(Serial)`)—jika tidak ada kabel, debug logging dinonaktifkan total untuk mencegah hardware blocking.

## 5. Power Policy (power_manager.cpp)
- **Anti-Sleep Guard**: Sistem deteksi aktivitas aktif yang mencegah auto-sleep jika user sedang di dalam mode pengukuran (HR) atau timer aktif.
- **AOD Support**: Backlight dimming (15/255) sebagai alternatif Deep Sleep jika user mengaktifkan mode AOD.

## 6. PHASE 2: INVARIANT RULES FOR AI AGENTS 🛡️
- **NO ARBITRARY REFACTORS**: Jangan mengubah core logic `display_hal.cpp` atau `ui_manager.cpp` tanpa alasan optimisasi yang kuat. 
- **SHUTDOWN PROTECTION**: Setiap sensor baru wajib memiliki fungsi `shutdown()` dan HARUS dipanggil oleh Power Manager sebelum tidur.
- **GUARDED SERIAL**: Semua `Serial.print` WAJIB dibungkus `if (Serial)` untuk menjaga performa bus USB-CDC.
- **I2C STABILITY**: Jangan gunakan delay panjang ( >10ms) saat polling I2C di loop utama agar FIFO MAX30100 tidak overflow.

## 7. PROJECT GOVERNANCE (MANDATORY) ⚖️
1. **NO UNAUTHORIZED CHANGES**: Dilarang keras mengubah code tanpa permintaan dan izin langsung dari USER.
2. **FILE OWNERSHIP**: Setiap file memiliki agent yang bertanggung jawab. Jika butuh edit file lain, WAJIB request melalui General Agent/USER.
3. **FLASH TEST**: Setiap kelar coding, wajib running `pio run` untuk memastikan tidak ada error compile.
4. **DOCS UPDATES**: Setelah di-acc USER, update dokumentasi di `docs/` dengan detail perubahan, larangan baru, dan potongan code.
5. **HISTORY PRESERVATION**: Dokumentasi dilarang dikurangi, hanya boleh ditambah. History pengembangan harus terjaga.

---
*Updated for Phase 2 — Built with Antigravity AI (General Agent Oversight).*
