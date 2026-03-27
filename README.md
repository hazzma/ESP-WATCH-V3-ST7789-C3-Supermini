# ⌚ ESP32-C3 Smartwatch v4 (ST7789) - Checkpoint 4 (HEALTH & POWER)

Proyek smartwatch berbasis **ESP32-C3 Super Mini** dengan layar ST7789 (240x280) menggunakan desain UI/UX modern berbasis **Glassmorphism**.

---

## ⚡ Fitur Utama & Milestones

### 🩺 Checkpoint 4: Health & Power Intelligence (LATEST)
Peningkatan drastis pada akurasi pembacaan sensor dan manajemen daya:
*   **Precision Pedometer**: Integrasi driver **BMI160 Bare-Metal** (SDO High 0x69) dengan sinkronisasi startup 81ms untuk akurasi langkah maksimal.
*   **Sanwa-Calibrated Battery**: Monitoring tegangan baterai menggunakan `analogReadMilliVolts` dengan pengali presisi **1.418f** (dikalibrasi manual dengan multimeter Sanwa).
*   **Charging Awareness**: Ikon petir dinamis (Amber) dan tema baterai Cyan yang muncul otomatis saat pengisian daya terdeteksi.
*   **Auto-AOD on Charge**: Layar otomatis masuk ke mode *Always-On* saat terhubung ke charger untuk kemudahan monitoring.
*   **Midnight Auto-Reset**: Logika otomatis untuk me-reset hitungan langkah harian tepat pukul 00:00.

### 🛡️ Checkpoint 3: Zero-Flicker Initiative
Berhasil mengeliminasi kedipan maut (*power-on flicker*) dan artefak "gambar rusak" pada sistem dengan **CS Hard-wired to GND**:

### 🎨 Checkpoint 2: UI/UX & Navigation Finalization
*   **Ghost Protection System (GPS)**: Algoritma brute-force redraw otomatis saat pindah dari mode hitam (Timer/Stopwatch/HR) ke menu utama untuk mencegah bayangan.
*   **Precision Mode (Timer Setup)**: Otomatis mendeteksi Double-Click tombol kanan hanya saat dalam menu setup timer (H/M/S switching).
*   **Fast Navigation**: Latensi 0ms pada menu standar untuk navigasi yang gesit.
*   **AOD Anti-Sleep**: Jam tidak akan masuk deep sleep jika fitur Always-On Display diaktifkan.

### ✨ Fitur Dasar (Checkpoint 1)
*   **Glassmorphism UI**: Tampilan transparan dengan sudut membulat.
*   **Slide Animation**: Transisi antar menu yang mulus.
*   **Sensor Integration**: Heart Rate (MAX30100) real-time measurement.

---

## 🛠️ Pinout (ESP32-C3 Super Mini)

| Sensor/Komponen | Pin ESP32-C3 | Detail |
|-----------------|--------------|--------|
| **SPI MOSI**    | GPIO 6       | Data ST7789 |
| **SPI SCK**     | GPIO 4       | Clock ST7789 |
| **TFT DC**      | GPIO 2       | Data/Command |
| **TFT RST**     | GPIO 1       | Reset Hardware |
| **TFT BL**      | GPIO 10      | Backlight (PWM + Analog Guard) |
| **TFT CS**      | **GND**      | **HARD-WIRED TO GND** (Software Masked Required) |
| **I2C SDA**     | GPIO 8       | MAX30100 & BMI160 |
| **I2C SCL**     | GPIO 9       | MAX30100 & BMI160 |
| **Tombol Kiri** | GPIO 7       | Navigasi Prev / Manual Sleep |
| **Tombol Kanan**| GPIO 5       | Navigasi Next / Wake / Double Click |
| **Battery ADC** | GPIO 3       | Monitoring Tegangan Baterai |

---

## 🚀 Instalasi & Development

Proyek ini menggunakan **PlatformIO** (VS Code).

1.  Clone repo ini.
2.  Buka di VS Code.
3.  Klik **Upload** di sidebar PlatformIO.

---

## 📖 Dokumentasi Teknis
Untuk detail teknis per-agen, silakan baca:
*   [docs/dokumentasi_uiuxagent.md](docs/dokumentasi_uiuxagent.md) - Manajemen UI, GPS, & Navigation.
*   [docs/dokumentasi_displayagent.md](docs/dokumentasi_displayagent.md) - Strategi Anti-Flicker & Hardware Sync.
*   [src/FSD.md](src/FSD.md) - Spesifikasi Fungsi & Checkpoint Status.

---
*Dibuat dengan dedikasi tinggi oleh Antigravity AI Agent.*
