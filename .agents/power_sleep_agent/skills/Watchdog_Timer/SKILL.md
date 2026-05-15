---
name: Watchdog Timer
description: Implementasi Task Watchdog (TWDT) untuk stabilitas sistem.
---

# Watchdog Timer

## Core Capabilities
- **System Stability**: Menggunakan `esp_task_wdt_init()` untuk memantau apakah loop utama "nyangkut" (hung).
- **Graceful Reset**: Menjamin sistem akan restart jika terjadi kegagalan logika yang menyebabkan kebuntuan (deadlock) pada multitasking (jika ada) atau loop utama.
- **Production Safety**: Memastikan WDT dikonfigurasi dengan timeout yang wajar (misal 5-10 detik) agar tidak mengganggu proses inisialisasi yang lama.
