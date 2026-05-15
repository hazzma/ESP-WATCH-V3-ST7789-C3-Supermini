---
name: TFT_eSPI Expert
description: Library utama display. Paham init sequence ST7789, pushImage, fillScreen, setTextDatum, dan drawString.
---

# TFT_eSPI Expert

## Core Capabilities
- **Inisialisasi ST7789**: Mengatur urutan init yang tepat untuk driver ST7789 (240x280).
- **Graphics API**: Mahir menggunakan `pushImage`, `fillScreen`, `drawRect`, dll.
- **Typography**: Mengatur perataan teks dengan `setTextDatum` dan mencetak string dengan `drawString`.
- **Config Management**: Mengetahui cara melakukan konfigurasi via build flags di `platformio.ini` tanpa menyentuh file library `User_Setup.h`.

## Best Practices
- Pastikan rotasi layar diatur sesuai kebutuhan hardware (0, 1, 2, atau 3).
- Gunakan DMA untuk push gambar jika hardware mendukung untuk performa maksimal.
- Hindari pembuatan objek `TFT_eSPI` di stack secara berulang; gunakan instance global atau singleton jika sesuai.
