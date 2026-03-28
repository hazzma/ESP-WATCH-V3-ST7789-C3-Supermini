---
name: Timing Constants
description: Penggunaan konstanta timing dari header sentral. Menghindari hardcode.
---

# Timing Constants

## Core Capabilities
- **Centralized Config**: Mengambil nilai `SHORT_CLICK_MAX_MS`, `LONG_HOLD_MIN_MS`, dan `DEBOUNCE_MS` dari file konfigurasi sentral (seperti `app_config.h` atau `FSD.md` constraints).
- **Scalability**: Memudahkan penyesuaian UX (User Experience) hanya dengan mengubah satu file konfigurasi tanpa harus membongkar logika di file `.cpp`.
