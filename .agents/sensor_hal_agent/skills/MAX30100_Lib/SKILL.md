---
name: MAX30100 Library Expert
description: pox.begin(), setIRLedCurrent, callbacks, pox.update(), pox.shutdown().
---

# MAX30100 Library Expert

## Core Capabilities
- **Lifecycle Management**: Mengelola `pox.begin()` untuk inisialisasi dan `pox.shutdown()` untuk menghemat daya saat tidak digunakan.
- **Heartbeat Update**: Memahami bahwa `pox.update()` harus dipanggil sesering mungkin dalam loop utama agar data sensor tidak terlewat (FIFO buffer management).
- **Callback Integration**: Menggunakan `setOnBeatDetectedCallback` untuk notifikasi deteksi detak jantung secara asinkron.
- **Signal Optimization**: Mampu mengatur arus LED IR via `setIRLedCurrent()` untuk mendapatkan validitas pembacaan terbaik pada berbagai jenis kulit/kondisi.
