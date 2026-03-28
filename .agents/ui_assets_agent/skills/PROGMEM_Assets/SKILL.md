---
name: PROGMEM Assets
description: Format RGB565 array di memori flash. Getter pattern yang benar.
---

# PROGMEM Assets

## Core Capabilities
- **Flash Storage**: Menggunakan keyword `PROGMEM` untuk menyimpan array gambar besar agar tidak memakan RAM.
- **RGB565 Format**: Mengelola data gambar dalam format 16-bit (Red 5, Green 6, Blue 5) yang kompatibel dengan ST7789.
- **Resource Headers**: Menyusun file `assets_wallpaper.h` dan `assets_icons.h` dengan deklarasi `extern` yang rapi.
- **Asset Size Management**: Menyediakan ukuran lebar/tinggi asset sebagai konstanta atau dalam struct bersama data pixel.
