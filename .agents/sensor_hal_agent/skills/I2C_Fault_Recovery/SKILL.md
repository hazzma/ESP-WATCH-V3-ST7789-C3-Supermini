---
name: I2C Fault Recovery
description: Retry logic dan bus-lock recovery (bit-banging clock pulses).
---

# I2C Fault Recovery

## Core Capabilities
- **Retry Logic**: Implementasi percobaan ulang (misal 3x) dengan delay non-blocking (atau minimal) saat inisialisasi gagal.
- **Bus-lock Recovery**: Teknik untuk membebaskan bus I2C yang "nyangkut" (SDA tertahan LOW oleh slave):
    1. Mengubah pin SCL menjadi output GPIO manual.
    2. Memberikan 9 pulsa clock untuk merestat state machine slave.
    3. Re-inisialisasi peripheral I2C hardware.
- **Error Propagation**: Melaporkan kegagalan permanen ke sistem untuk ditangani oleh Power Manager atau State Machine.
