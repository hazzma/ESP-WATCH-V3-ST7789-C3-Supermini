# MASTER KNOWLEDGE BASE — ESP32-C3 SMARTWATCH (V6.0)
*Reference document for development phase 4 (BLE, App Interface)*

> **SUPERVISION & SURGICAL EXECUTION RULE (MANDATORY)**
> 1. Master Agent **WAJIB** mendelegasikan tugas teknis kepada Sub-Agent (Power, UI/UX, Sensor, Display).
> 2. Semua eksekusi kode harus dilakukan secara **SURGICAL (Bedah Presisi)**. Tidak boleh ada bagian kode yang tidak relevan ikut terubah tanpa konfirmasi persetujuan dari USER.

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
*Laporan selesai by UI/UX Agent - Checkpoint 9 (Connectivity Ready).*

## 8. DEBT & BUGS TARGET (NEXT SPRINT) 🎯
Berikut adalah daftar kelemahan arsitektur (*Technical Debt*) & bug potensial yang saat ini masuk antrean prioritas untuk diselesaikan (*Surgical Target*):
1. **Critical CDC Hang pada Boot (COM Stuck)**: Sistem sering *freeze* dengan layar *blank hitam* (backlight nyala tapi idle) karena *deadlock* pada USB CDC, masalah *TxTimeout*, atau inisialisasi *hardware* (I2C/LittleFS) yang gagal saat komputer telat me-mount COM port.
2. **Race Conditions pada Variabel BLE**: Status `ble_is_connected` dan `ble_is_syncing` memicu *race condition* atau telat sinkron pada UI karena nilai tidak di-tag sebagai `volatile` (mengingat event dieksekusi di *background task* BLE).
3. **Bahaya Fragmentasi Heap dari Sprite**: Pemanggilan berulang `deleteSprite()` dan `createSprite()` di `ui_manager` bisa menyebabkan ruang *Free Space* RAM pecah-pecah (fragmented) sehingga sistem berpotensi *Out of Memory* dan *Crash*.
4. **Hardcoded Battery Offset Curve**: Logika kompensasi *charging* dengan angka mati `(v - 0.25f)` rentan kacau jika beda daya input atau kabel, membuat indikator persentase meloncat tidak beraturan.
5. **No-Yield Blocking Loop**: Animasi geser (`animate_slide_transition()`) berjalan membabi-buta tanpa porsi relaksasi `yield()`. Jika data BLE masuk saat geser menu, *Watchdog Timer* (WDT) bisa teriak karena proses lain nggak dikasih waktu napas.
6. **Step Counter Auto-Reset Bug**: Pada mode step counter, jumlah langkah kadang ter-reset sendiri ke 0 secara tiba-tiba tanpa pemicu yang jelas.
7. **Blank Screen on BLE Connect**: Terkadang saat jam baru saja terhubung ke koneksi BLE, UI jam hilang/nge-blank dan hanya menyisakan background (wallpaper) saja.
8. **Missing UI on Wake-up (Step Counter Mode)**: Jika layar dimatikan saat sedang berada di mode step counter, lalu dihidupkan kembali, sistem berhasil kembali ke state tersebut namun angka stat (step) tidak muncul (hanya terlihat ring/lingkaran dan wallpaper).
