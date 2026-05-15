---
name: Battery ADC
description: Membaca Vbat via GPIO 3. Formula pembagi tegangan (raw/4095.0)*3.3*2.0.
---

# Battery ADC

## Core Capabilities
- **ADC Reading**: Mengambil nilai analog dari GPIO 3 (ESP32-C3 ADC1_CH3).
- **Voltage Calculation**: Menggunakan formula pembagi tegangan eksternal (resistor divider) untuk mendapatkan tegangan baterai sebenarnya.
- **Percentage Mapping**: Mengonversi tegangan (misal 3.2V - 4.2V) menjadi persentase 0-100% untuk ditampilkan ke user.
- **Smoothing**: Menerapkan filter (moving average) untuk menstabilkan pembacaan nilai baterai agar tidak meloncat-loncat.
