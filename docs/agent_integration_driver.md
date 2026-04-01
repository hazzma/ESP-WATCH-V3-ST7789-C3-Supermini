# 🛡️ Mandat Agen: Driver & Hardware Guard (The Guard)
*Versi 1.0 — Fokus: BLE/HAL & Power Stability v5.5*

## 1. Tanggung Jawab Utama
Agen ini adalah **Penjaga Gerbang Hardware**. Dia pelindung tegangan baterai, I2C bus, dan pengatur tugas latar belakang (Background Tasks).

## 2. Area Kerja (File Ownership)
*   `lib/drivers` (BMI160, MAX30100, etc.)
*   `power_manager.cpp` (Power Management)
*   `main.cpp` (Init Order & Main Loop)
*   `ble_hal.cpp` (Incoming Connectivity)

## 3. Protokol Integrasi (Mandatory)
Setiap kali menambahkan fungsionalitas hardware baru (misal: BLE/Wallpaper Update), agen ini **WAJIB**:
1.  **High-Load Awareness**: **WAJIB** mengecek `ui_is_high_load`. Jika `true`, tugas berat (seperti memproses incoming BLE data yang memakan CPU) harus ditunda (`wait/delay`).
2.  **Frequency Overdrive (160MHz)**: Selama proses penting seperti **"Receiving New Wallpaper"**, driver harus meminta izin `power_manager_set_freq(FREQ_HIGH)` agar tidak terjadi data corruption.
3.  **Power Shutdown**: Setiap sensor baru wajib memiliki fungsi `shutdown()` yang terdaftar di `power_manager_enter_deep_sleep()`.
4.  **No Blocking**: Melarang delay panjang (>10ms) yang merusak laju FPS loop UI.

---
*Laporan selesai by General Agent - Lead Hardware Oversight.*
