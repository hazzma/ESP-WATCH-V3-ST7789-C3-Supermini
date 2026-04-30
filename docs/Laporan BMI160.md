# 📑 Laporan Analisis Kegagalan Pedometer (BMI160)
**Versi Analisis: 1.0 (Audit v7.1.1)**
**Status: CRITICAL - NO DATA FLOW**

## 1. Ringkasan Masalah
Langkah kaki (Step Counter) pada jam tangan berhenti berfungsi (Stuck at 0) setelah penerapan optimasi hemat daya dan peningkatan sensitivitas pada versi 7.1. Padahal pada versi 6.x, sensor mampu membaca langkah (meskipun kurang sensitif).

---

## 2. Temuan Teknis (Jantung Masalah)

### 🔴 A. Register 0x7B (STEP_CONF[1]) - "Tuning Bit Sabotage"
- **Kondisi v6.0**: Mengirim `0x0B` (`0b0000 1011`). 
  - Bit 3 = 1 (Enabled).
  - Bits 0-1 = `11` (Tuning/Filtering standard BOSCH).
- **Kondisi v7.1**: Mengirim `0x08` (`0b0000 1000`).
  - Bit 3 = 1 (Enabled).
  - Bits 0-1 = `00` (Filtering dihapus total).
- **Analisa**: Algoritma pedometer BMI160 di level silikon membutuhkan tuning filter minimum agar bisa membedakan getaran langkah dengan noise statis. Dengan memberikan `0x08`, kita mematikan filter tersebut, membuat sensor menganggap semua gerakan adalah gangguan listrik (noise).

### 🔴 B. Hilangnya "Gyro Startup Sync"
- **Fakta**: Fungsi `writeReg(REG_CMD, 0x15)` (Gyro Normal) dihapus di v7.1 untuk menghemat ~700uA.
- **Dampak**: Dokumentasi internal (`dokumentasi_sensoragent.md` poin 43) secara spesifik menyebutkan bahwa **Pedometer Engine butuh kestabilan unit tenaga (PMU) & Gyro selama 81ms** di awal inisialisasi untuk setup gravitasi (Zero-G Offset). Tanpa ini, mesin penghitung langkah tidak pernah "Bangun" dari kondisi Sleep/Reset.

### 🔴 C. UI Logic - "The 0-Reset Trap"
- **Lokasi**: `ui_manager.cpp` baris 627-628.
- **Kondisi**: Variabel `uint32_t steps = 0` dideklarasikan SECARA LOKAL di dalam loop rendering.
- **Masalah**: 
  1. Jika jam lo sibuk ( `ui_is_high_load == true` ), pembacaan sensor dilewati dan `steps` akan selamanya lapor 0 ke layar.
  2. Adanya timer `step_warmup_timer > 1000ms` memaksa tampilan lari ke angka 0 setiap kali user pindah menu atau ada perubahan menit di layar (karena timer di-reset pada baris 623).

---

## 3. Rencana Operasi Penyelamatan (Proposed Action)

1.  **Re-Enable Gyro Hybrid**: Nyalain Gyro selama 80ms pas `init()`, biarkan mesin pedometer sinkron, baru boleh di-suspend setelah masuk loop utama.
2.  **Restore 0x7B**: Gunakan kombinasi mode `SENSITIVE (0x2D)` dengan bit filter `0x0B` atau biarkan bit tuning tetap aktif.
3.  **Global UI Steps**: Pindahkan variabel `steps` ke status global atau update akumulator secara asynchronous tanpa penghalang timer 1 detik yang ketat.

---
*Dibuat oleh Master Agent (Lead Architect) - Science over Code!* 🫡🛡️⌚
