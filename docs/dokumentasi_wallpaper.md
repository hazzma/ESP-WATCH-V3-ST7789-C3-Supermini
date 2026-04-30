# 🛡️ Smartwatch Wallpaper Streaming Protocol (v6.4.4)
**Project: ESP32-C3 Supermini Watch Integration**

## 1. Overview
Protokol ini menggunakan **LittleFS** sebagai media penyimpanan permanen dan **Dynamic RAM Cache** untuk performa UI 60FPS.

- **Storage Target**: `/wallpaper.bin`
- **Format Image**: RGB565 (16-bit), Big Endian.
- **Resolution**: 240x280 pixels.
- **Total Size**: 134,400 Bytes.
- **BLE MTU**: Optimal di 247 atau 512 bytes.

---

## 2. Handshake Sequence

### Phase A: Initiation (App -> Watch)
- **Characteristic ID**: `UUID_CONTROL` (`...9006`)
- **Command**: `0x01` + `ExpectedSize` (4 bytes, Little Endian).
- **Watch Action**: Membuka file `/wallpaper.bin`, reset hitungan index ke 0, dan boost CPU ke 160MHz.
- **Reply**: `0x01` (Success) via Notification pada `UUID_CONTROL`.

### Phase B: Streaming (App -> Watch)
- **Characteristic ID**: `UUID_WALLPAPER` (`...9002`)
- **Packet Structure**:
  - `[0-1]`: Chunk Index (uint16_t, Little Endian).
  - `[2-3]`: Reserved (0x00, 0x00).
  - `[4-5]`: Payload Length (uint16_t, Little Endian).
  - `[6..]`: Binary Image Data.
- **Watch Verification**: Jam akan mengecek apakah `Chunk Index == nextExpectedChunk`.
- **Unique ACK (0x06)**: Jam akan menjawab via Notification pada `UUID_WALLPAPER`:
  - Format: `[0x06, LowByte(Index), HighByte(Index)]`
  - **IMPLEMENTASI FLUTTER**: App HARUS menunggu ACK dengan index yang sesuai sebelum mengirim potongan berikutnya demi stabilitas (Flash write speed constraint).
- **NAK (0x15)**: Jika urutan salah, jam mengirim `0x15`. App harus resend dari index terakhir yang sukses.

### Phase C: Completion (App -> Watch)
- **Characteristic ID**: `UUID_CONTROL` (`...9006`)
- **Command**: `0x02`.
- **Watch Action**: Menutup file, flushing cache, dan mengosongkan RAM buffer untuk reload gambar baru.
- **Reply**: `0x01` (Success) atau `0x15` (Failure - Jika ukuran file tidak pas 134,400 byte).

---

## 3. Flutter Implementation Tips (For Flutter Agent)

### A. MTU Negotiation
Gunakan `requestMtu(247)` atau `512` segera setelah koneksi berhasil. Semakin besar MTU, semakin cepat pengiriman gambar.

### B. Throttle & Sync
Jangan menggunakan `writeWithoutResponse` secara membabi buta tanpa mengecek ACK. LittleFS pada ESP32 memiliki kecepatan tulis terbatas. Rekomendasi: kirim per 240-500 byte, tunggu notifikasi `0x06`.

### C. Image Conversion
Pastikan gambar dikonversi ke **RGB565** sebelum dikirim. Byte order harus sesuai dengan ekspektasi driver ST7789 (biasanya Big Endian swap).

---

## 4. Potential Bugs & Warning
1. **Memory Exhaustion**: Wallpaper menggunakan 134KB RAM. Jika App lo mengaktifkan fitur BLE lain yang memakan RAM banyak secara bersamaan, ESP32 bisa **Reboot (Panic)**.
2. **Flash Wear**: Jangan melakukan sinkronisasi wallpaper terlalu sering (e.g., tiap menit) karena akan memperpendek umur Flash chip.
3. **Ghosting Image**: Jika pengiriman gagal (NAK), file `/wallpaper.bin` mungkin korup. Jam akan otomatis balik ke wallpaper default (`PROGMEM`) sampai sinkronisasi sukses dilakukan ulang.

---
*Created by Master Agent (Lead Architect) - Phase 5/6 Complete.* 🫡🛡️⌚
