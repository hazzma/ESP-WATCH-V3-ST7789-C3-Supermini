# 🛡️ Dokumentasi Display Agent - Zero-Flicker Initiative

Laporan ini merangkum perjuangan teknis dalam menghilangkan kedipan maut (*power-on flicker*) dan artefak gambar rusak pada ESP32-C3 Smartwatch v3.

## 1. Masalah Utama (The Enemy)
*   **Flicker Putih**: Muncul kilatan putih/abu-abu selama sepersekian detik saat bangun dari Deep Sleep.
*   **Artifact VRAM (Gambar Rusak)**: Bagian atas layar sering menampilkan "salju" atau gambar korup sebelum Wallpaper muncul sempurna.
*   **Ghosting Sisa**: Tampilan wallpaper lama yang sempat terlihat sesaat sebelum ke-refresh.

## 2. Analisa Akar Masalah (The Cause)
*   **Hardware Constraint (CS Fixed LOW)**: Pin Chip Select (CS) ditarik fisik ke GND. Ini membuat LCD selalu "mendengarkan" kabel SPI, termasuk sampah data dari bootloader.
*   **SPI Flash Conflict (GPIO 10)**: Pin Backlight (IO10) berbagi jalur dengan internal Flash SPI pada beberapa mode bot ESP32-C3. Aktivitas baca program di awal bot menyebabkan lampu latar berkedip liar.
*   **VRAM Dirty State**: Saat power-on, memori internal ST7789 berisi data acak (sampah) yang harus segera dibersihkan sebelum lampu menyala.

## 3. Taktik Software yang Diterapkan (The Strategy)
Untuk mencapai transisi kelas premium, kita menerapkan strategi **"Sequential Masking & Snap-ON"**:

1.  **Early Backlight Kill**: Pada baris pertama `setup()`, pin 10 langsung dipaksa `LOW` via `digitalWrite` murni (Bukan PWM) untuk meminimalkan durasi kedipan bootloader.
2.  **Insta-Masking (0x28)**: Perintah `DISPLAY OFF` dikirim dalam hitungan mikrodetik setelah `tft.init()`. Ini memutus aliran data VRAM ke panel kaca agar "salju" memori tidak terlihat.
3.  **Double VRAM Scrubbing**: UI Manager menggambar Wallpaper lengkap sebanyak **DUA KALI** berturut-turut saat layar masih terkunci mode gelap. Ini memastikan memori LCD benar-benar "Sapurata" bersih sebelum lampu idup.
4.  **Stable Delay (150ms)**: Memberikan waktu bagi regulator internal LCD untuk stabil sebelum menerima hantaman data SPI kecepatan tinggi.
5.  **Snap-ON Strategy**: Menghapus efek *breathing* (redup ke terang) saat bangun. Lampu langsung ditembak ke target tingkat kecerahan secara instan setelah VRAM siap. Kecepatan ini membantu menutupi noise hardware yang tersisa.
6.  **Elegant Shutdown (Breathing OUT)**: Tetap mempertahankan efek meredup perlahan saat mau tidur untuk menjaga estetika "Jam Mahal".

## 4. Hardware Recommendations (Next Level)
Jika di masa depan ingin mencapai kesempurnaan 100% tanpa bantuan software akrobatik:
*   **Kapasitor Filter**: Tambahkan kapasitor **1uF - 10uF** antara pin Backlight (GPIO 10) dan GND untuk meredam lonjakan listrik liar dari bootloader.
*   **MOSFET Pull-Down**: Gunakan resistor pull-down 10k pada jalur Gate MOSFET backlight.
*   **Independent CS**: Jika memungkinkan pada revisi PCB berikutnya, hubungkan pin CS LCD ke GPIO ESP32-C3 agar bisa dikendalikan sepenuhnya oleh software.

## 5. Status Terkini: LOCKED 🔒
Sistem saat ini sudah sangat stabil dengan transisi yang bersih (Dark -> Snap Wallpaper). Taktik asimetris (Snap-ON / Breathing-OUT) telah teruji sebagai solusi terbaik untuk keterbatasan hardware saat ini.

---
*Laporan selesai by Display Agent - Checkpoint 3 Finalized.*
