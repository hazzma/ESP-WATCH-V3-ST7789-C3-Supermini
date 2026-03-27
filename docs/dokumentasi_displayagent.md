# 🛡️ Dokumentasi Display Agent - Zero-Flicker Initiative
*Status: S-Rank Finalized (v3.8)*

Laporan ini merangkum perjuangan teknis dalam menghilangkan kedipan maut (*power-on flicker*) dan artefak gambar rusak pada ESP32-C3 Smartwatch v3.

## 1. Masalah Utama yang Diatasi
*   **Flash Putih (The White Ghost)**: Muncul kilatan putih saat bangun dari Deep Sleep akibat VRAM default ST7789.
*   **SPI Tearing**: Efek bagian atas layar bergeser duluan dibanding bawah saat navigasi.
*   **Glitched Startup**: Muncul salju/noise di layar saat transisi bootloader.

## 2. Strategi "Nuclear Masking" (v3.8 Update) 🚀
Untuk mencapai transisi kelas premium tanpa kilatan, kita menerapkan strategi berlapis:

1.  **Hardware Pulldown Insurance**: Memaksa GPIO 10 tetap LOW secara elektrik.
2.  **Post-INIT Re-Force**: Menangkap glitch otomatis dari library.
3.  **Cold Black Scrubbing**: Membersihkan VRAM warna hitam murni dalam waktu <13ms.
4.  **Snap-ON to Breathing**: Transisi halus (~150ms).

### 🛠️ Reference Code (Surgical Logic)
Berikut adalah potongan kode kritis untuk menjaga stabilitas display di masa depan:

**A. Proteksi Inisialisasi (`display_hal.cpp`):**
```cpp
void display_hal_init() {
    pinMode(BL_PIN, OUTPUT);
    digitalWrite(BL_PIN, LOW); 
    gpio_set_pull_mode((gpio_num_t)BL_PIN, GPIO_PULLDOWN_ONLY); // Mencegah Floating

    tft.init();
    digitalWrite(BL_PIN, LOW); // Matikan paksa jika library sempat menyalakannya
    tft.writecommand(0x28);    // DISPOFF - Jangan tampilkan sampah VRAM
    tft.fillScreen(TFT_BLACK); // Cold Scrubbing
}
```

**B. Sinkronisasi Wakeup (`ui_manager.cpp`):**
```cpp
void ui_manager_init() {
    display_hal_display_off(); // Keep Mask
    tft.fillScreen(TFT_BLACK); // Power Scrub
    
    render_current_state();    // Gambar Wallpaper 
    display_hal_display_on();  // Buka Mask
    display_hal_backlight_fade_in(val, 150); // Premium Entry
    
    last_rendered_state = current_state; // CRITICAL: Cegah kedipan ganda di loop
}
```

## 3. High-Speed SPI (80MHz)
*   **Manfaat**: Menghilangkan tearing vertikal karena pengiriman data 3x lebih cepat dibanding standar 27MHz.
*   **Power Consumption**: Sangat efisien karena bus SPI hanya aktif sebentar lalu kembali idle (*Race-to-Sleep*).

## 4. Status Terkini: LOCKED 🔒
Sistem saat ini sudah sangat stabil dengan transisi yang bersih (Deep Black -> Wallpaper Slide). Taktik asimetris (Fast Breathing IN / Elegant Fade OUT) telah teruji sebagai solusi terbaik untuk keterbatasan hardware ST7789 tanpa pin CS.

---
*Laporan selesai by Display Agent - Checkpoint 4 (Premium Grade).*
