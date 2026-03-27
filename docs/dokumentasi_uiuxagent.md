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

---

## Version 4.6 (Surgical Precision - EDAN Edition) 🔥
**Update Terbaru: 27 Maret 2026**

### 1. Mandat Kerja "Bedah Saraf" (Surgical Focus)
Berdasarkan instruksi langsung dari USER, setiap perubahan kode wajib dilakukan secara **Surgical** (Targeted). Dilarang keras melakukan perombakan total (rewrite) pada file yang sudah stabil. Fokus hanya pada baris kode yang bermasalah untuk meminimalkan resiko crash dan beban kerja sistem.

### 2. Animasi Transisi "Window Parallax" (Anti-Tarik Wallpaper)
Mengatasi efek wallpaper yang ikut bergeser (ketarik) saat melakukan navigasi menu dari/ke Dashboard Steps. 

**Logika Implementasi (Precision Sync):**
Setiap frame pada animasi geser menghitung ulang koordinat sumber wallpaper (`bx`) secara dinamis berdasarkan posisi sprite di layar. Hal ini membuat wallpaper di dalam kotak transisi selalu sinkron dengan wallpaper statis di belakangnya, menciptakan efek "Jendela Transparan".

**Code Snippet (Sync Logic):**
```cpp
// From (Outgoing) - Sinkronisasi koordinat sumber wallpaper dengan posisi gerak
int from_x = MENU_BX - offset;
build_menu_sprite(menu_spr, from, from_x, MENU_BY);
menu_spr.pushToSprite(&combo, from_x, 0);

// To (Incoming) - Sinkronisasi koordinat sumber wallpaper dengan posisi gerak
int to_x = MENU_BX + 240 - offset;
build_menu_sprite(menu_spr, to, to_x, MENU_BY);
menu_spr.pushToSprite(&combo, to_x, 0);
```

### 3. Restorasi Data Dashboard Activity (Steps)
Menampilkan kembali metrik penting yang sempat hilang di bagian bawah ring dashboard langkah:
- **KCAL (Kalori)**: Dihitung secara estimasi berdasar jumlah langkah.
- **KM (Jarak)**: Konversi langkah ke kilometer (0.75m per langkah estimasi).

**Update Visual:**
Menambahkan preview judul "ACTIVITY" dan bayangan ring statis ke dalam `build_menu_sprite` untuk Dashboard Steps, sehingga saat transisi tidak lagi muncul kotak hitam/kosong melainkan preview dashboard yang sedang meluncur masuk.

### 4. Status Stabilitas: SUPER OPTIMAL (EDAN) ✅
Sistem saat ini berada dalam kondisi performa terbaik dengan beban CPU rendah dan visual transisi sekelas premium. 
- **SPI Speed**: Locked 80MHz.
- **I2C Status**: Selective Polling (On-Demand) Berhasil.
- **Button Status**: Polling Mode Stable.
