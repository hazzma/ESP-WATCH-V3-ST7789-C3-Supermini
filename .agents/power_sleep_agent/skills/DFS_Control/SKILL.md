---
name: DFS Control
description: Pengaturan frekuensi CPU 160/80/40MHz untuk efisiensi daya.
---

# DFS Control (Dynamic Frequency Scaling)

## Core Capabilities
- **Frequency Transitions**: Menggunakan `setCpuFrequencyMhz()` untuk berpindah antar tingkat performa.
- **Timing Sensitivity**: Memastikan perpindahan frekuensi tidak mengganggu timing peripheral yang sensitif (seperti I2C atau SPI jika clock-nya bergantung pada APB clock).
- **Power Optimization**: Menghitung budget daya berdasarkan frekuensi yang dipilih sesuai standar RULE-003.
- **Pre-Sleep Throttling**: Menurunkan frekuensi ke level terendah (40MHz) sesaat sebelum memasuki deep sleep untuk transisi daya yang mulus.
