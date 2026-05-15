---
name: DFS Integration (RULE-003)
description: Mengatur frekuensi CPU (power_manager_set_freq) berdasarkan beban kerja.
---

# DFS Integration (RULE-003)

## Core Capabilities
- **Adaptive Frequency**: 
    - **160 MHz**: Digunakan saat proses rendering intensif (Watchface update, UI transitions).
    - **80 MHz**: Digunakan saat aplikasi dalam keadaan idle atau standby (background polling).
    - **40 MHz**: Digunakan saat mode Always-On Display (AOD) untuk penghematan daya ekstrim.
- **Timing Efficiency**: Memanggil perubahan frekuensi tepat sebelum dan sesudah operasi berat untuk meminimalkan durasi konsumsi daya tinggi.
