# Dokumentasi UI/UX Agent - ESP32-C3 Smartwatch
## Version 2.0 (Checkpoint 2 - Premium Navigation)

### 1. Sistem Navigasi "Double Tap" (Timer Setup)
Untuk mempermudah penggunaan satu tangan, mode **Timer Setup** menggunakan deteksi klik cepat (Double Click) di GPIO 5 (Tombol Kanan).

**Logic Deteksi (Internal):**
- **Interval**: Klik kedua harus terjadi dalam waktu **250ms** setelah klik pertama dilepas.
- **Dynamic Mode**: Fitur ini dinonaktifkan secara otomatis saat keluar dari mode Timer agar navigasi menu lain tetap responsif (tanpa jeda 250ms).

**Implementasi Code:**
```cpp
// Panggil di UI Manager saat ganti state
if (target >= STATE_SET_TIMER_H && target <= STATE_SET_TIMER_S) {
    button_hal_set_double_click(true); // Masuk mode Timer: Enable Double Click
} else {
    button_hal_set_double_click(false); // Keluar mode Timer: Fast Response
}
```

### 2. Teknik "Surgical Refresh" (Anti-Flicker)
Menghilangkan kedipan (*flicker*) saat mengatur angka (Timeout/Brightness) dengan hanya memperbarui area angka di dalam sprite, tanpa menggambar ulang seluruh wallpaper.

**Langkah-langkah:**
1. Gunakan `fillRect` pada area angka saja dengan warna box (`BOX_COL`).
2. Tulis ulang angka baru di atas area bersih tersebut.
3. Push sprite ke layar secara lokal.

**Code Snippet:**
```cpp
case STATE_SET_TIMEOUT:
    if (last_state != current_state) {
        build_menu_sprite(menu_spr, ...); // Gambar penuh cuma sekali
    } else {
        // Hanya gambar ulang kotak value (40, 45, dsb)
        menu_spr.fillRect(40, 45, 120, 30, BOX_COL);
    }
    // Render angka baru
    menu_spr.drawString(value, ...); 
    menu_spr.pushSprite(X, Y);
```

### 3. Ghost Protection System (GPS)
Mengatasi artefak visual (sisa gambar hitam/putih) saat berpindah dari mode layar penuh (Timer/Alarm) ke menu normal (Wallpaper).

**Cara Kerja:**
- Setiap state transition dideteksi. 
- Jika state baru adalah mode Menu/Watchface yang membutuhkan latar belakang gambar, panggil `tft.pushImage(0, 0, 240, 280, wallpaper_ptr)` secara eksplisit sebelum merender elemen UI lainnya.

### 4. Layout Timer & Stopwatch Pro
1. **Timer Title**: Selalu di posisi Y:30 (paling atas).
2. **Indicator**: Panah `^^` digunakan untuk menunjukkan kolom mana yang sedang diedit (H/M/S).
3. **Execution Mode**: Menggunakan `TFT_BLACK` solid untuk mengejar **60 FPS** rendering angka detik tanpa beban memori menggambar wallpaper tiap frame.

### 5. Alarm Mode (Epilepsy-Safe)
Flash warna Putih-Merah-Biru setiap **500ms** di kecerahan 100% untuk notifikasi yang sangat terlihat tanpa menyebabkan sakit mata (siklus lambat). Tombol Kiri digunakan untuk menghentikan alarm seketika.
