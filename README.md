# ESP32-C3 Smartwatch v3 (ST7789)

Firmware untuk Smartwatch berbasis **ESP32-C3 Super Mini** dengan layar **ST7789 240x280**. Proyek ini difokuskan pada efisiensi daya (Deep Sleep), performa UI yang halus (Sprite-based), dan fitur kesehatan dasar.

## Fitur Utama
- **Always-On Display (AOD)**: Tampilan redup hemat daya dengan Burst Refresh (80MHz Burst / 10MHz Idle).
- **Smooth UI**: Rendering berbasis Sprite untuk menghindari kedipan (flicker-free).
- **Power Management**: 
    - DFS (Dynamic Frequency Scaling): 160MHz (Render), 80MHz (Idle), 40MHz/10MHz (AOD).
    - Deep Sleep: Konsumsi daya ultra-rendah dengan bangun via tombol GPIO 5.
- **Health Tracking**: Integrasi sensor MAX30100 untuk detak jantung (BPM) dan SpO2.
- **Battery Monitoring**: Pembacaan voltase baterai real-time.

## Pinout (ESP32-C3 Super Mini)
| Perangkat | Komponen | GPIO |
| :--- | :--- | :--- |
| **Layar (SPI)** | MOSI | 6 |
| | SCK | 4 |
| | DC | 2 |
| | RST | 1 |
| | Backlight | 10 |
| **I2C (Shared)** | SDA | 8 |
| | SCL | 9 |
| **Input** | Main Button | 7 |
| | Wake Button | 5 |
| **Battery** | ADC Read | 3 |

## Cara Penggunaan
1.  **Navigasi**: Tekan tombol (GPIO 7) untuk berpindah menu (Watchface -> Heart Rate -> AOD Setting -> System Sleep).
2.  **Heart Rate**: Masuk ke menu Heart Rate, tahan tombol (Long Hold) untuk mulai mengukur.
3.  **AOD**: Aktifkan melalui menu AOD Setting. Jam akan meredup otomatis setelah 1 menit tidak ada aktivitas.
4.  **Deep Sleep**: Masuk ke menu System, tahan tombol (Long Hold) untuk tidur total.
5.  **Wake Up**: Tekan tombol GPIO 5 untuk membangunkan jam dari Deep Sleep.

## Persyaratan Build
- **PlatformIO**
- **Framework**: Arduino
- **Platform**: `espressif32@6.4.0`
- **Library**:
    - `bodmer/TFT_eSPI` (Konfigurasi pin ada di `platformio.ini`)
    - `oxullo/MAX30100lib`

---
*Developed with love for ESP32-C3 Enthusiasts.*
