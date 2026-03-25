---
name: Header Design
description: Menulis .h yang bersih: pragma once, forward declaration, minimal include.
---

# Header Design

## Guidelines
- **Clean Interface**: Menyediakan API yang mudah dipahami oleh agent lain tanpa perlu tahu detail implementasinya.
- **Guard**: Selalu gunakan `#pragma once`.
- **Minimal Includes**: Hanya include apa yang benar-benar dibutuhkan di file header. Gunakan *forward declarations* jika memungkinkan.
- **Encapsulation**: Sembunyikan detail implementasi internal (seperti instance `TFT_eSPI`) dari interface publik jika memungkinkan.
- **Documentation**: Berikan komentar pada setiap fungsi publik untuk menjelaskan parameter dan return value.
