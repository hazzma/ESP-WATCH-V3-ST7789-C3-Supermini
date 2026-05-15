# MASTER KNOWLEDGE BASE — ESP32-C3 SMARTWATCH (V6.0)
*Reference document for development phase 4 (BLE, App Interface)*

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
- **Smart Frequency Policy**:
    1. **Cold Boot**: 160MHz (10 detik) untuk CDC Handshaking.
    2. **Wake from Sleep**: Langsung 80MHz (FREQ_MID) untuk efisiensi navigasi.
- **Sleep Log**: Notifikasi visual di serial sebelum masuk Deep Sleep.

## 6. PHASE 4: INVARIANT RULES FOR AI AGENTS 🛡️
- **NO ARBITRARY REFACTORS**: Jangan mengubah core logic `display_hal.cpp` atau `ui_manager.cpp` tanpa alasan optimisasi yang kuat. 
- **SHUTDOWN PROTECTION**: Setiap sensor baru wajib memiliki fungsi `shutdown()` dan HARUS dipanggil oleh Power Manager sebelum tidur.
- **GUARDED SERIAL**: Semua `Serial.print` WAJIB dibungkus `if (Serial)` untuk menjaga performa bus USB-CDC.
- **I2C STABILITY**: Jangan gunakan delay panjang ( >10ms) saat polling I2C di loop utama agar FIFO MAX30100 tidak overflow.
- **BMI160 LOW-POWER MANDATE**: Step counter BMI160 wajib menggunakan mode `0x12` (Low Power). Mode `0x11` (Normal) tidak stabil untuk pedometer.
- **CDC HANDSHAKE GUARD**: `main.cpp` wajib memiliki `delay(10)` dan `setTxTimeoutMs(0)` sebelum/saat Serial init untuk mencegah hang.

## 7. PROJECT GOVERNANCE (MANDATORY) ⚖️
1. **NO UNAUTHORIZED CHANGES**: Dilarang keras mengubah code tanpa permintaan dan izin langsung dari USER.
2. **FILE OWNERSHIP**: Setiap file memiliki agent yang bertanggung jawab. Jika butuh edit file lain, WAJIB request melalui General Agent/USER.
3. **FLASH TEST**: Setiap kelar coding, wajib running `pio run` untuk memastikan tidak ada error compile.
4. **DOCS UPDATES**: Setelah di-acc USER, update dokumentasi di `docs/` dengan detail perubahan, larangan baru, dan potongan code.
5. **HISTORY PRESERVATION**: Dokumentasi dilarang dikurangi, hanya boleh ditambah. History pengembangan harus terjaga.

---
*Updated for Phase 4 — Built with Antigravity AI (General Agent Oversight).*

### 4. 240x240 Display Compatibility (Surgical Offset)
Ditemukan bug koordinat yang menyebabkan Dashboard Steps terpotong di layar 240x240:
*   **Ring Shift**: Pusat lingkaran digeser dari Y=80 ke **Y=75** agar radius 70 + dot 6 muat di dalam viewport sprite.
*   **Stats Relocation**: Baris KCAL/KM dipindah dari koordinat off-screen Y=240 ke **Y=205** (Presisi di atas border bawah).
*   **Efficiency Overdrive**: Stats bawah kini hanya menggunakan sprite 35px, menghemat transfer data LCD hingga 78% untuk baris data dinamis.

---
---
*Laporan selesai by UI/UX Agent - Checkpoint 9 (Connectivity Ready).*

### 5. BMI160 STABILITY LOCK (v8.5) 🔒
**Ketetapan Master Agent - 15 Mei 2026**
*   **Startup Sync**: Gyro Normal (0x15) wajib di-init selama 81ms SEBELUM mengaktifkan pedometer.
*   **Interrupt Release**: Wajib membaca `REG_INT_STATUS` (0x1C) di akhir init untuk memastikan GPIO 5 tidak terkunci (ghost button fix).
*   **Hardware Flush**: `STEP_CNT_CLR` (0xB2) membutuhkan delay minimal 10ms (2ms tidak cukup di hardware C3 Supermini).

### 6. UI/UX STABILITY LOCK (v8.6) 💎
**Ketetapan Master Agent - 15 Mei 2026**
*   **Vertical Breathing Room**: Judul menu wajib di **Y=42** untuk menghindari overlap dengan jam (Y=8-23).
*   **Surgical Safety**: Area pembersihan atas dimulai dari **Y=25** (Clock Safe Zone).
*   **Silent Jump Cleanup**: Transisi instan (tanpa animasi) dari mode List wajib melakukan pembersihan surgical border (Y=25-90 dan Y=190-280).
*   **Atomic Refresh**: Jam kecil selalu di-refresh terakhir via `push_top_clock(false)` untuk akurasi waktu tanpa kedipan.
