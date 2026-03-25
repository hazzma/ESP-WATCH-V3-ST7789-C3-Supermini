# Laporan Pengembangan Display HAL & Backlight System

Dibuat oleh: **Display HAL Agent**  
Target: ESP32-C3 Watch (GPIO 10 - PIN_LCD_BL)  
Status: ✅ **Sync with Hardware Works**

---

## 1. Konfigurasi Hardware (PWM)

Untuk mengontrol tingkat kecerahan layar tanpa membuat layar berkedip (*flicker*), saya menggunakan modul **LEDC** bawaan ESP32-C3.

### Parameter PWM:
- **Frequency**: 5000 Hz (Tinggi agar tidak terlihat getar oleh mata manusia).
- **Resolution**: 8-bit (0-255).
- **Pin**: GPIO 10.

```cpp
// Konfigurasi awal di display_hal.cpp
ledcSetup(BL_CHANNEL, 5000, 8); // 5kHz, 8-bit
ledcAttachPin(10, BL_CHANNEL);
```

---

## 2. Fitur "Memory" (Persistence)

Masalah umum pada jam tangan adalah settingan brightness yang balik ke default setelah dimatikan. Saya menggunakan atribut **RTC RAM** milik ESP32-C3 agar nilai brightness bertahan meskipun jam masuk ke mode **Deep Sleep**.

```cpp
// Di ui_manager.cpp
static RTC_DATA_ATTR int brightness_ui_val = 127; // Tersimpan di memori RTC
```

Saat jam bangun dari tidur, `ui_manager_init()` langsung memanggil nilai ini untuk memulihkan kecerahan terakhir.

---

## 3. Sinkronisasi UI & Hardware

Setiap kali pengguna menekan tombol Kiri atau Kanan di menu pengaturan, dua hal terjadi secara simultan:
1. **Visual Update**: Bar hijau di layar bertambah/berkurang.
2. **Hardware Update**: Nilai PWM dikirim langsung ke driver layar.

```cpp
// Snippet Logic di ui_manager_update()
if (bt == BTN_RIGHT_CLICK) {
    brightness_ui_val += 15; 
    if (brightness_ui_val > 255) brightness_ui_val = 255;
    
    // Perintah langsung ke hardware:
    display_hal_backlight_set(brightness_ui_val); 
}
```

---

## 4. Efek Fade-In Smooth

Untuk memberikan kesan premium, saat jam tangan baru dinyalakan, layar tidak langsung "jreng" terang, melainkan melakukan **Fade-In** halus dari gelap ke tingkat terang yang diinginkan.

```cpp
void ui_manager_init() {
    // ... init lainnya ...
    display_hal_backlight_fade_in(brightness_ui_val, 500); // 500ms fade
}
```

---

*Status: Terkoneksi & Persisten.*
