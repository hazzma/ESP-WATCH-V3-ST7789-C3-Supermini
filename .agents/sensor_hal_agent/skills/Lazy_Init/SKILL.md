---
name: Lazy Init Pattern
description: RULE-007 — pox.begin() HANYA saat dibutuhkan, tidak pernah di setup().
---

# Lazy Init Pattern (RULE-007)

## Core Capabilities
- **On-Demand Power**: Menunda konsumsi daya sensor hingga aplikasi benar-benar membutuhkan datanya.
- **State Integration**: Inisialisasi diletakkan di dalam fungsi `..._hal_init()` yang dipanggil oleh State Machine saat memasuki state tertentu (misal: `EXEC_HR`).
- **Resource Recovery**: Mengetahui cara mematikan sensor secara software ketika State Machine berpindah ke state lain atau kembali ke mode idle.
