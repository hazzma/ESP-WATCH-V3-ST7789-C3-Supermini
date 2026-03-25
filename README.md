# ⌚ ESP32-C3 Smartwatch v3 (ST7789)

Proyek smartwatch berbasis **ESP32-C3 Super Mini** dengan layar ST7789 (240x280) menggunakan desain UI/UX modern berbasis **Glassmorphism**.

---

## ✨ Fitur Utama (Checkpoint 1)

*   **Modern UI/UX (Glassmorphism)**: Tampilan menu transparan di atas wallpaper dengan sudut membulat (Rounded Design).
*   **Dual-Button Navigation**: 
    - **GPIO 7 (Kiri)**: Geser ke kiri / Kurangi nilai.
    - **GPIO 5 (Kanan/Wake)**: Geser ke kanan / Masuk (Hold 0.8s) / Bangunkan jam.
*   **Slide Animation**: Transisi menu geser yang mulus dengan akselerasi CPU 160MHz.
*   **iPhone-Style Top Clock**: Jam kecil statis yang nangkring di atas semua menu (tanpa kedip).
*   **Sensor Heart Rate (MAX30100)**: Pengukuran BPM dan SpO2 secara real-time.
*   **Always-On Display (AOD)**: Mode hemat energi (Dimmed) yang tetap menampilkan jam besar.
*   **Power Manager**: Deep sleep hemat daya dengan bangun instan lewat tombol fisik.

---

## 🛠️ Pinout (ESP32-C3 Super Mini)

| Sensor/Komponen | Pin ESP32-C3 | Detail |
|-----------------|--------------|--------|
| **SPI MOSI** | GPIO 6 | Data ST7789 |
| **SPI SCK** | GPIO 4 | Clock ST7789 |
| **TFT DC / CS** | GPIO 2 / 20 | Kontrol Layar |
| **TFT RST** | GPIO 1 | Reset Layar |
| **I2C SDA** | GPIO 8 | Shared Sensor (MAX30100) |
| **I2C SCL** | GPIO 9 | Shared Sensor (MAX30100) |
| **Tombol Kiri** | GPIO 7 | Navigasi Prev |
| **Tombol Kanan** | GPIO 5 | Navigasi Next / Wake |
| **Battery ADC** | GPIO 3 | Monitoring Tegangan Baterai |

---

## 🚀 Instalasi & Development

Proyek ini dibangun menggunakan **PlatformIO** (VS Code).

1.  Clone repo ini.
2.  Buka di VS Code dengan plugin PlatformIO.
3.  Klik **Upload** (PlatformIO: Upload).
4.  Pastikan library `TFT_eSPI` dikonfigurasi pinda-pinnya sesuai tabel di atas.

---

## 📝 Catatan UI/UX (Eksperimental)

UI jam ini menggunakan teknik **Wallpaper Injection** dan **Double Buffering** (Sprite) untuk mengeliminasi kedip (*flickering*) pada layar SPI murah, memberikan pengalaman visual yang setara dengan jam tangan flagship.

*Dokumentasi lengkap pengembangan bisa dilihat di [docs/dokumentasi_uiuxagent.md](docs/dokumentasi_uiuxagent.md).*

---

*Dibuat oleh Antigravity AI Agent.*
