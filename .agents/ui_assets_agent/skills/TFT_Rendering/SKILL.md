---
name: TFT_eSPI Rendering
description: Menggambar UI via display_hal_get_tft(). Paham rendering element jam & status.
---

# TFT_eSPI Rendering

## Core Capabilities
- **HAL-based Access**: Selalu menggunakan instance TFT yang disediakan oleh HAL untuk menggambar.
- **Watchface Elements**: Merender jam digital, tanggal, indikator baterai, dan icon charging dengan memperhatikan tata letak (alignment).
- **Efficient Drawing**: Hanya menggambar bagian layar yang berubah (dirty rects) jika memungkinkan, atau melakukan full refresh sesuai state.
- **Text Management**: Mengatur ukuran font, warna, dan posisi menggunakan API `drawString`, `setTextDatum`, dll.
