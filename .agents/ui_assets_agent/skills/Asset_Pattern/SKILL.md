---
name: RULE-011 Asset Pattern
description: Pemisahan asset dari logic. Penggunaan getter untuk gambar.
---

# RULE-011 Asset Pattern

## Core Capabilities
- **Decoupled Graphics**: Memastikan `ui_manager.cpp` tidak mengandung data array pixel (hex values).
- **Getter Architecture**: Memanggil fungsi seperti `assets_get_wallpaper()` atau `assets_get_icon_heart()` untuk mendapatkan pointer ke data gambar di PROGMEM.
- **Informative Placeholders**: Jika asset asli belum siap, menyediakan placeholder yang informatif dan terdokumentasi dengan baik (bukan sekedar array kosong).
