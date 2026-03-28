---
name: Pre-Sleep Checklist
description: Eksekusi 8 langkah kritis sebelum deep sleep dalam urutan yang ketat.
---

# Pre-Sleep Checklist

## Core Capabilities
- **Sequence Discipline**: Memastikan urutan pembersihan peripheral dilakukan dengan benar untuk menghindari kondisi "zombie" atau kebocoran arus.
- **Checklist Path**:
    1.  Mematikan Backlight (LEDC Detach).
    2.  Mengirim perintah shutdown ke Display (ST7789).
    3.  Memasukkan Sensor ke mode Power Down (MAX30100/BMI160).
    4.  Menurunkan Frekuensi CPU (DFS).
    5.  Menyimpan state kritis ke RTC Memory.
    6.  Konfigurasi Wakeup Source (EXT0).
    7.  Release Guard (Memastikan tombol tidak ditekan).
    8.  Eksekusi Deep Sleep.
- **Verification**: Memberikan log pada setiap tahap penting untuk memudahkan debug jika sistem gagal tidur.
