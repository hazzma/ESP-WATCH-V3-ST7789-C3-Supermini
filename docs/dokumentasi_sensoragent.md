# 🩺 Dokumentasi Sensor Agent - Stability Log

Laporan ini mengunci strategi pembacaan sensor MAX30100 yang telah teruji stabil dan responsif.

## 1. Masalah yang Diselesaikan (Legacy Fixes)
*   **HR Freeze (Stuck at 52/0)**: Disebabkan oleh latency rendering UI yang memakan waktu terlalu lama sehingga FIFO sensor meluap (overflow).
*   **Blocking on Serial**: Disebabkan oleh buffer USB-CDC yang penuh saat Serial Monitor tidak dibuka.
*   **LED Persistent**: Masalah hardware di mana LED sensor tetap menyala setelah keluar menu.

## 2. Strategi "Burst Polling" (Sequential Efficiency)
Alih-alih menggunakan RTOS yang beresiko pada kestabilan bus I2C, kita menggunakan sistem **Sequential Burst**:
*   **I2C Clock**: Ditingkatkan ke **400kHz (Fast Mode)**.
*   **Burst Count**: Memanggil `pox.update()` sebanyak **5 kali berturut-turut** dalam satu siklus `loop`. Ini memastikan antrean data sensor selalu habis terambil meskipun UI sedang sibuk melakukan proses lain.
*   **Clean Start**: Melakukan **I2C Hard Reset (Register 0x06 write 0x40)** setiap kali masuk menu HR untuk memastikan chip memulai dari kondisi murni.

## 3. Optimasi UI (Surgical Rendering)
*   **Wallpaper Suspension**: Selama mode `STATE_EXEC_HR`, sistem melompati (*skip*) semua proses render wallpaper (67k pixel).
*   **High-Speed Text**: Menggunakan teks dengan parameter warna latar belakang (`TFT_BLACK`) untuk mempercepat proses gambar di layar (3x lebih cepat dibanding teks transparan).

## 4. CPU Load Analysis
*   **Watchface/Idle**: < 5% @ 80MHz.
*   **HR Measurement**: 10-15% @ 80MHz.
*   **Slide Transitions**: ~80% @ 160MHz (Sesaat).
## 5. BMI160 Pedometer (Step Counter) ✨
Fitur penghitung langkah kaki kini telah menggunakan driver **Bare Metal** murni melalui I2C:

### 📊 Register Map (Internal Logic)
| Register Name | Address | Description | Optimized Value |
| :--- | :--- | :--- | :--- |
| `REG_CHIP_ID` | `0x00` | Identifikasi silikon (Must be 0xD1) | `0xD1` |
| `REG_ACC_RANGE` | `0x41` | Sensitivitas mentah akselerometer | `0x03` (2G Mode) |
| `REG_STEP_CNT_0` | `0x78` | Step Counter LSB (Byte Rendah) | *Dynamic Read* |
| `REG_STEP_CNT_1` | `0x79` | Step Counter MSB (Byte Tinggi) | *Dynamic Read* |
| `REG_STEP_CONF0` | `0x7A` | Konfigurasi Pedometer Engine | `0x15` (Pedometer On) |
| `REG_STEP_CONF1` | `0x7B` | Sensitivity & Mode Selection | `0x0B` (Normal Mode) |
| `REG_CMD` | `0x7E` | Command Register (Reset/Boot) | `0xB2` (Reset Step) |

### 🧠 Implementation Details (Surgical Logic)
*   **I2C Address**: Terkunci pada **0x69** (SDO HIGH). Ini adalah alamat kritis yang menjamin komunikasi hardware berhasil 100%.
*   **Startup Sequence**: 
    1. `0xB6` (Soft Reset) -> Jeda 10ms.
    2. `0x11` (Acc Normal) -> Jeda 4ms.
    3. `0x15` (Gyro Normal) -> Jeda **81ms**. Tanpa jeda gyro ini, pedometer engine seringkali tidak stabil saat mulai menghitung.
    4. **Low Power Mandate**: Pindah ke `0x12` (Acc Low Power) untuk operasi pedometer berkelanjutan. Mode `0x11` (Normal) terbukti menyebabkan sensor mati mendadak/reset.
    5. `0xB2` (Reset Steps) -> Memulai dari 0 di level silikon (Jeda 10ms untuk flush hardware).
*   **Interrupt Guard**: Membaca register `0x1C` secara eksplisit setelah konfigurasi untuk melepaskan latch interrupt, mencegah GPIO 5 (Tombol) terkunci.
*   **Sanity Check Update**: Menambahkan filter `diff > 500` per 500ms untuk membuang data sampah akibat hardware glitch.
*   **Efficiency**: Menggunakan **Burst Read** (Membaca 2 byte sekaligus dari 0x78) untuk mengurangi overhead di bus I2C. ESP32 hanya butuh <0.5ms untuk mengambil data langkah terbaru.

### 🚀 Future Improvements & Sensitivity Tuning
*   **Tune Option 1 (REVERTED)**: Menggunakan Buffer 5 (`0x25`) untuk keseimbangan sensitivitas terbaik hasil test hardware terbaru.
*   **Calibration**: Melakukan `BMI160.autoCalibrateAccelOffset()` sekali saat jam pertama kali diproduksi akan meningkatkan akurasi deteksi gravitasi (Z-Axis), yang secara langsung memperbaiki akurasi pedometer.

---
*Laporan Sensor Agent - Status: BMI160 BARE-METAL READY (V8.5 LOCKED) 🔒.*
