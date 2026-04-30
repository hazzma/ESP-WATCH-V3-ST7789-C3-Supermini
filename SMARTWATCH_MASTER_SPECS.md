# ⌚ ESP32-C3 Smartwatch V3 - Master Specifications & Features

Dokumen ini berisi rangkuman teknis lengkap mengenai hardware, software, dan fitur yang ada pada project ESP-WATCH-V3-ST7789. Gunakan dokumen ini sebagai basis brainstorming atau input untuk AI.

---

## 🚀 1. Hardware Specifications
| Komponen | Spesifikasi | Keterangan |
| :--- | :--- | :--- |
| **MCU** | ESP32-C3 RISC-V | 160MHz, WiFi + BLE 5.0 |
| **Layar** | ST7789 IPS LCD | 1.28", 240x280 (Logic 240x240) |
| **Motion Sensor**| BMI160 (6-Axis) | Accel & Gyro, High-Precision Pedometer |
| **Biometric** | MAX30100 | Heart Rate & SpO2 Oximeter |
| **Penyimpanan** | 4MB Flash | LittleFS untuk Wallpaper & Data |
| **Power** | Li-Po 3.7V | Terintegrasi Charging (GPIO 0) & ADC (GPIO 3) |

---

## 🛠️ 2. Core Architecture (Software)
- **Framework**: Arduino ESP32 v2.0.11 (Core 6.4.0).
- **UI Engine**: `TFT_eSPI` dengan sistem **Unified Shared Architecture (USA)**. Seluruh layar dirender via Sprite untuk menghilangkan *flicker*.
- **I2C Protocol**: Bare-metal murni dengan **Repeated Start** support. Dioptimasi untuk menangani bus collision antara BMI160 dan MAX30100.
- **Persistence**: Menggunakan **RTC Slow Memory** (`RTC_DATA_ATTR`) agar data langkah dan status jam tidak hilang saat mati daya atau tidur.

---

## ✨ 3. Feature Set
### 🕒 Watchface & Time
- **Dual Mode**: Analog & Digital (Switchable).
- **Time Sync**: Sinkronisasi waktu presisi via BLE (tm structure).
- **Midnight Auto-Reset**: Langkah kaki otomatis kembali ke 0 setiap ganti hari (berdasarkan perbandingan `tm_yday`).

### 🏃 Pedometer (Langkah Kaki)
- **24/7 Counting**: Sensor tetap menghitung meski jam dalam kondisi *Deep Sleep*.
- **Wrist-Optimized**: Tuning sensitivitas khusus pergelangan tangan (Normal Mode, Buffer 5-langkah).
- **PCB Flip Correction**: Koreksi sumbu Z perangkat lunak karena sensor dipasang terbalik pada PCB.
- **Sanity Filter**: Proteksi "Anti-Pecicilan" (mengabaikan lonjakan langkah >500 dalam 500ms).

### 💓 Health Monitoring
- **Real-time Heart Rate**: Deteksi detak jantung (BPM) instan.
- **SpO2 Oximeter**: Pengukuran kadar oksigen darah.
- **Burst Polling**: Pengambilan data sensor 5x per siklus untuk mencegah luapan FIFO saat UI sibuk.
- **Turbo Rendering**: Mematikan proses wallpaper saat mengukur untuk fokus daya CPU pada sensor.

### 🔋 Power Management & Wake
- **Deep Sleep**: Konsumsi daya sangat rendah (<100uA).
- **Wake-on-Motion**: Layar menyala otomatis saat tangan diputar atau diguncang (~250mg threshold).
- **Wake-on-Button**: Bangun via tombol fisik.
- **Dynamic Frequency**: Otomatis 80MHz (Hemat) saat standby, 160MHz (Turbo) saat sinkronisasi data.

### 📲 Connectivity (BLE)
- **Wallpaper Transfer**: Upload gambar background custom via Bluetooth (Chunked Binary Transfer).
- **Sensor Streaming**: Mengirim data langkah dan baterai ke aplikasi smartphone.
- **Control Interface**: Remote control untuk navigasi menu jam via BLE.

---

## 🔌 4. Pin Mapping (Pinout)
| Fungsi | Pin (GPIO) | Keterangan |
| :--- | :--- | :--- |
| **LCD MOSI** | 6 | SPI Data |
| **LCD SCLK** | 4 | SPI Clock |
| **LCD DC / RST** | 2 / 1 | Data Command / Reset |
| **LCD Backlight**| 10 | PWM Dimming |
| **I2C SDA / SCL** | 8 / 9 | Shared Sensor Bus |
| **Left Button** | 7 | Navigation |
| **Right Button** | 5 | **SHARED** dengan BMI160 INT1 |
| **Battery ADC** | 3 | Monitoring Tegangan |
| **Charge Stat** | 0 | LOW = Charging |

---

## 🛡️ 5. Special Logic Implementations
- **Shared Pin Synergy (GPIO 5)**: Menggunakan mode *Open-Drain* dan *Mapping Suspension* agar BMI160 dan Tombol tidak saling ganggu meski berbagi satu kabel.
- **Elegant Backlight**: Sistem *Fading In/Out* non-blocking menggunakan task background agar transisi layar terasa premium.
- **Asset Cache**: Sistem proteksi LittleFS agar wallpaper tidak rusak saat transfer terputus di tengah jalan.

---

## 🧭 6. UI Navigation & Interactions
Sistem navigasi menggunakan sistem **Circular Slide** (Geser Melingkar).

### 📑 Menu Order (Urutan Halaman)
Navigasi antar menu menggunakan tombol Klik (Kanan: Maju, Kiri: Mundur):
1.  **Watchface** (Home / Tampilan Utama)
2.  **Heart Rate** (Menu detak jantung)
3.  **Screen Timeout** (Menu durasi layar nyala)
4.  **Always On Display (AOD)** (Menu toggle AOD)
5.  **Timer** (Menu pengatur waktu mundur)
6.  **Stopwatch** (Menu pencatat waktu)
7.  **Brightness** (Menu kecerahan layar)
8.  **Sync Mode** (Menu pairing BLE)
9.  **Steps Dash** (Tampilan detail langkah kaki)
*(Kembali ke Watchface)*

### 🎮 Button Mapping (Fungsi Tombol)
| Aksi | Tombol Kanan (Right) | Tombol Kiri (Left) |
| :--- | :--- | :--- |
| **Klik (Click)** | Menu Selanjutnya / Tambah Nilai (+) | Menu Sebelumnya / Kurang Nilai (-) |
| **Tahan (Hold)** | **Select / Enter**: Masuk ke fitur, Konfirmasi, atau Start Timer | **Exit / Power**: Keluar ke menu utama atau masuk ke *Deep Sleep* |
| **Klik 2x (Double)**| Ganti Field (Jam/Menit/Detik pada Timer) | - |

---

## 🧠 7. Brainstorming & Roadmap (Future Implementation)
Rencana pengembangan struktur menu dan fitur baru.

### 🗺️ Proposed UI Tree (Struktur Menu Baru)
```text
Watchface (Home)
    │
AOD ON/OFF
    │
Heart Rate
    │
Timer
    │
Stopwatch
    │
Settings  ← Satu pintu masuk utama
    │
Brightness
    │
Steps Dash 
(Kembali ke Watchface)
```

### ⚙️ Settings Sub-Menu Detail
```text
Settings
    ├── Brightness (Slider +/-)
    ├── AOD Config
    │     ├── Toggle: Never Off vs Timed
    │     ├── AOD Brightness Level
    │     └── AOD Duration (Hanya aktif jika mode Timed dipilih)
    ├── Raise to Wake (Raise2Wake)
    │     ├── Toggle ON/OFF
    │     └── Sensitivity Config (Rotate/Lift)
    ├── BLE / Connectivity
    │     ├── Toggle BLE ON/OFF
    │     └── Pairing / Sync Mode
    ├── Screen Timeout (Global)
    └── Exit (via Hold Kiri)
```

### 🚀 Upcoming Features
- **Fall Detector**: Menggunakan algoritma g-force pada BMI160 untuk mendeteksi jatuh ekstrem.
- **Improved Settings**: Mengkonsolidasi menu-menu kecil ke dalam satu pintu masuk "Settings" agar navigasi utama lebih ringkas.

---
*Generated by Antigravity Master Agent - Smartwatch V3 Project Status.*
