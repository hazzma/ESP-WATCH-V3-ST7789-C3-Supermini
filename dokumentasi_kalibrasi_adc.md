# 🛡️ Battery ADC Calibration Log (v6.5.1)
**Primary Reference Instrument: Sanwa Professional Lab Multimeter**

## 1. Sanwa Lab Handshake (FINAL)
Gue pake metode **Linear Interpolation (2-Point)** buat nge-bypass ketidakteraturan kurva ADC pada ESP32-C3 Supermini.

| Metric | Anchor Point 1 (Low) | Anchor Point 2 (High / Full) |
| :--- | :--- | :--- |
| **GPIO3 (raw_mv Read)** | 1964mV | 2061mV |
| **GPIO3 (Physical Probe)**| 1.929V | 2.024V |
| **VBAT (Sanwa Meter)** | 3.90V | 4.10V |
| **Ideal Ratio (Divider)** | 2.02x | 2.02x (Verified Precise) |

### 🔬 Deviation Analysis:
- **ADC Offset**: Internal ADC ngebaca **~36mV lebih tinggi** (1964 vs 1929) dibanding probe fisik. Rumus interpolasi otomatis meniadakan (nullify) offset ini.
- **Linearity**: Sangat stabil di angka 2.02x. Deviasi antar titik sangat minim dibanding instrumen sebelumnya.

---

## 2. Active Formula (power_manager.cpp)
```cpp
// [CALIB 2-POINT] Interpolation (Sanwa Lab Verified Final - v6.5.1)
// Point 1: 1964mV -> 3.90V | Point 2: 2061mV -> 4.10V
float voltage = 3.90f + ((float)raw_mv - 1964.0f) * 0.0020618f;
```

---

## 3. Battery Visibility Curve (ui_manager.cpp)
- **Charging Spike Protection**: Sanwa mendeteksi lonjakan **+270mV (0.27V)** saat dicolok charger. Sistem menggunakan kompensasi **-0.25V** untuk mencegah kenaikan persentase palsu.
- **Charger Detection**: Heuristic `Vbat > 4.16V` (uncompensated) tetap digunakan untuk mendeteksi kabel dicolok.

- **>= 4.10V**: 100% (Sanwa Stable Full Point)
- **> 3.90V**: 80% - 99%
- **> 3.65V**: 40% - 79% (Nominal Voltage Range)
- **<= 3.35V**: 0% (Safety Hardware Cut-off)

---

## 4. Maintenance Notes
- **2026-04-02**: Migrasi total ke **Sanwa Lab Standard**. Aneng didegradasi ke instrumen sekunder.
- **Status**: **ULTRA-PRECISE & CALIBRATED**.

---
*Created by Master Agent (Lead Architect) - Science First!* 🫡🛡️⌚
