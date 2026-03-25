# FSD (Functional Specification Document) — ESP32-C3 Smartwatch v2.2

Dokumen spesifikasi fungsional untuk arsitektur firmware jam tangan ESP32-C3.

---

## 1. Spesifikasi Hardware

| Komponen | Pin | Detail |
|----------|-----|--------|
| **MCU** | ESP32-C3 | Super Mini (RISC-V) |
| **Display** | ST7789 (SPI) | 240x280 (MOSI: 6, SCK: 4, DC: 2, RST: 1) |
| **Backlight** | GPIO 10 | PWM (Dimming supported) |
| **I2C Bus** | Shared | SDA: 8, SCL: 9 |
| **Sensor HR** | MAX30100 | Heart Rate & SpO2 |
| **Input L** | **GPIO 7** | Pull-up (GND when pressed) |
| **Input R** | **GPIO 5** | Pull-up & Wake-up source |
| **Power** | ADC 3 | Battery Voltage monitoring |

---

## 2. Navigasi & Input (Dual Button System)

Jam tangan menggunakan sistem navigasi dua tombol yang disederhanakan:

### 2.1 Tombol Kiri (GPIO 7)
- **Klik**: Kembali ke menu sebelumnya / Kurangi nilai (Brightness).

### 2.2 Tombol Kanan (GPIO 5 / Wake Button)
- **Klik**: Lanjut ke menu berikutnya / Tambah nilai (Brightness).
- **Hold (0.8s)**: Masuk (Select) / Toggle / Keluar dari menu atau fitur aktif.
- **Wake**: Membangunkan jam dari Deep Sleep.

---

## 3. State Machine & Fitur

1.  **WATCHFACE**: 
    - Menampilkan Jam (HH:MM), Tanggal, dan Baterai.
    - Menampilkan FPS (Opsional).
    - Status AOD (Always-On Display) indikator.
2.  **MENU_HR → EXEC_HR**: 
    - Hold (0.8s) di menu HR untuk mulai mengukur.
    - Tampilan hitam (Power Saving) saat pengukuran berlangsung.
    - Hold (0.8s) saat pengukuran untuk mematikan sensor dan kembali ke menu.
3.  **MENU_AOD**: 
    - Hold (0.8s) untuk mengaktifkan/mematikan transisi ke mode AOD otomatis (setelah 10 detik idle).
4.  **MENU_SLEEP**: 
    - Hold (0.8s) untuk mematikan jam (Deep Sleep mode). Wake-up source: GPIO 5.
5.  **MENU_BRIGHTNESS → SET_BRIGHTNESS**: 
    - Hold (0.8s) untuk masuk ke mode pengaturan.
    - Klik Kanan/Kiri untuk nambah/kurang brightness.
    - Hold (0.8s) untuk simpan dan keluar.

---

## 4. UI/UX & Rendering (Experimental v8)

- **Wallpaper Injection**: Wallpaper tetap terlihat di belakang menu pop-up (Glassmorphism).
- **Anti-Flicker Double Buffering**: Semua perubahan konten menu diproses di memori (Sprite) sebelum dikirim ke LCD untuk eliminasi kedip.
- **iPhone-Style Top Clock**: Jam mini (static) di tengah-atas semua menu.
- **Slide Transition**: Efek animasi geser (Slide) saat navigasi menu dengan akselerasi CPU 160MHz.

---

*Status: Disahkan v2.2 — Checkpoint 1 Integration.*