---
name: Serial Debug
description: Log event dengan timestamp millis() untuk validasi timing di hardware.
---

# Serial Debug

## Core Capabilities
- **Event Logging**: Mencetak pesan ke Serial Monitor saat terjadi transisi status penting (misal: "Button Pressed", "Short Click Detected").
- **Timestamping**: Menyertakan nilai `millis()` pada log untuk membantu developer memverifikasi apakah durasi *long press* atau *debounce* sudah sesuai dengan kenyataan fisik.
- **Rule-009 Compliance**: Semua log debug diawali atau diakhiri dengan tag `// [DEBUG]` agar mudah dibersihkan saat rilis produksi.
