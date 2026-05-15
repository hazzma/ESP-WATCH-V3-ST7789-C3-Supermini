---
name: ST7789 Driver Quirks
description: Tau offset, rotation, dan RGB order ST7789 240x280.
---

# ST7789 Driver Quirks

## Common Issues
- **Offset**: Layar 240x280 kadang membutuhkan offset memori (misal: x=0, y=20 atau sebaliknya) agar gambar tidak terpotong.
- **Rotation**: Memahami perbedaan orientasi 0 dan 2 (portrait/upside-down).
- **Color Order**: Menangani masalah warna terbalik (RGB vs BGR) melalui konfigurasi pengisian bit warna.
- **Inversion**: Mengaktifkan atau menonaktifkan "Display Inversion" (INVON/INVOFF) sesuai kebutuhan panel spesifik.
