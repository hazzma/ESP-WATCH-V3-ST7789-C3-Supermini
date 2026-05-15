---
name: Battery ADC + Thresholds
description: Evaluasi tegangan baterai untuk trigger alert dan sleep paksa.
---

# Battery ADC + Thresholds

## Core Capabilities
- **Threshold Monitoring**: 
    - **Normal**: Vbat > 3.50V.
    - **Warning (Low)**: Vbat ≤ 3.50V (memicu notifikasi/icon merah).
    - **Critical (Shutoff)**: Vbat ≤ 3.40V (memicu pemadaman sistem paksa untuk melindungi umur sel Li-ion).
- **Safe Shutdown**: Mengeksekusi prosedur pre-sleep checklist secara darurat jika baterai mencapai level kritikal.
- **Accurate Evaluation**: Bekerja sama dengan UI Manager untuk mendapatkan data ADC yang sudah difilter/averaged.
