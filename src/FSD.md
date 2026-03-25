# Functional Specification Document (FSD) - ESP32-C3 Smartwatch
## Version 3.3 (Checkpoint 2 - Premium UI & Power)

### 1. Input System (LOCKED)
- **Fast Mode**: 0ms latency on Menu/Watchface/Stopwatch.
- **Precision Mode**: 250ms gap allowed ONLY in Timer Setup for `BTN_RIGHT_DOUBLE`.
- **Hold**: 800ms (Always instant).
*   **Checkpoint 2 (LOCKED)**: UI/UX finalized, AOD Anti-Sleep, Fast/Precision Switching, Ghost Protection (GPS).
*   **Checkpoint 3 (LOCKED)**: **Zero-Flicker Boot & Snap-ON Wakeup**.
*   **Checkpoint 4 (LOCKED - CURRENT)**: **Sensor Logic & Silent Optimization**.
    *   **Silent Mode**: All Serial logs wrapped in `if (Serial)` to prevent power waste and USB-CDC blocking.
    *   **MAX30100 Logic**: Removed all Serial calls from sensor callbacks (onBeatDetected) to allow standalone operation.
    *   **System Stability**: Optimized DFS (Dynamic Frequency Scaling) logs for zero-latency during transitions.

### 2. UI Rendering & Ghost Protection (LOCKED)
- **Ghost Protection System (GPS)**:
  - Redraw Wallpaper 100% saat keluar dari "Black Mode" (Timer/SW/HR).
  - Redraw Wallpaper 100% saat pindah DARI Watchface (Home) menuju Menu manapun untuk membersihkan sprite jam besar.
  - **Menu-to-Menu**: Redraw wallpaper dinonaktifkan untuk menjaga kemulusan animasi slide (Zero Flicker).
- **Surgical Refresh**: Update nilai Timeout/Brightness hanya memperbarui area angka di dalam sprite menggunakan `fillRect(BOX_COL)`.

### 3. Power & Sleep Management (LOCKED)
- **AOD (Always On Display)**:
  - Jika `aod_allowed` AKTIF: Jam tidak akan pernah masuk Deep Sleep secara otomatis. Hanya meredupkan backlight (15/255) saat timeout di Watchface.
  - Jika masuk ke menu dari kondisi AOD, jam langsung kembali ke kecerahan normal.
- **Deep Sleep Auto-Lock**: Hanya aktif jika `aod_allowed` MATI.

### 4. Wake-Up Aesthetics
- **Cinematic Start**: Backlight dimulai dari 0 (gelap total), ada jeda stabilitas 50ms, lalu melakukan *fade-in* selama 700ms (BL_FADE_MS + 200) untuk efek bernapas (*breathing*).