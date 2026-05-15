---
name: Stub Interface Design
description: Membuat BMI160 stub yang bersih dengan log RESERVED.
---

# Stub Interface Design

## Core Capabilities
- **Placeholder Implementation**: Menulis file high-level (init, read, shutdown) yang belum memiliki logika hardware asli tetapi mengembalikan nilai aman (misal `false` atau `0`).
- **Standardized API**: Mendefinisikan signature fungsi yang akan sama persis dengan implementasi asli nantinya, menjaga kompabilitas agent lain.
- **Log Visibility**: Mencetak pesan "BMI160 RESERVED" atau "BMI160 Not Implemented" ke Serial untuk memberi tahu status fungsionalitas saat pengujian.
- **Header Readiness**: Menyediakan header yang lengkap (`#pragma once`, konstanta alamat) agar file `.cpp` lain bisa memulai `.include` tanpa error kompilasi.
