---
name: Clean Enum Design
description: ButtonEvent { NONE, SHORT_CLICK, LONG_HOLD } — interface yang caller-friendly.
---

# Clean Enum Design

## Core Capabilities
- **Abstracted Events**: Menyediakan `enum` sebagai output utama fungsi HAL sehingga logika tingkat atas tidak perlu berurusan dengan milidetik atau level tegangan.
- **Minimalist API**: Memastikan fungsi `getButtonEvent()` atau sejenisnya hanya mengembalikan satu event per waktu untuk diproses oleh state machine utama.
- **Readability**: Menggunakan penamaan yang jelas dan deskriptif untuk setiap nilai dalam `enum`.
