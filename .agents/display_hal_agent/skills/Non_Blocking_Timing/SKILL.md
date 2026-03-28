---
name: Non-blocking Timing
description: Implementasi fade in tanpa delay() — pakai millis() dan state variable. Patuh pada RULE-002.
---

# Non-blocking Timing

## Methodology
- **Millis Strategy**: Menggunakan `millis()` untuk melacak interval waktu tanpa menghentikan eksekusi program.
- **State Persistence**: Menyimpan state (misal: current brightness, target brightness, last update time) dalam variabel statis atau member class.
- **RULE-002 Compliance**: Tidak boleh ada infinite polling loops.
- **RULE-003 Compliance**: Tidak boleh ada blocking `delay()` dalam logika normal.

## Implementation Pattern
```cpp
if (millis() - lastUpdate > interval) {
    // Update brightness step
    lastUpdate = millis();
}
```
