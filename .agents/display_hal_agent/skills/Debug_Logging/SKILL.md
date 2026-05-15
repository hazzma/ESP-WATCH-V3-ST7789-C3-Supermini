---
name: Debug Logging (RULE-009)
description: Semua Serial.print debug ditandai // [DEBUG]. Paham relevansi info debug.
---

# Debug Logging (RULE-009)

## Standard
- **Tagging**: Setiap baris output debug harus menyertakan komentar `// [DEBUG]` untuk kemudahan filter.
- **Selective Logging**: Hanya mencetak informasi yang benar-benar berguna untuk diagnosa hardware atau alur eksekusi (misal: "Display initialized", "Backlight set to 255").
- **Verbosity**: Menghindari spamming serial monitor yang dapat mempengaruhi timing sistem.
- **Conditional Compilation**: (Opsional) Menggunakan macro untuk mematikan semua log debug pada build produksi.
