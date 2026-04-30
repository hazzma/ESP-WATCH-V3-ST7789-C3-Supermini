# 🖼️ Laporan Kesiapan Wallpaper Protocol (v6.4.4)
**Target: ESP32-C3 Supermini Firmware Integration**

## 1. Status Kesiapan Firmware (Watch Side)
**STATUS: 🟢 READY FOR STREAMING**

Firmware saat ini sudah mengimplementasikan protokol **v6.4.4** secara penuh di file `ble_hal.cpp`. Arsitektur BLE sudah dikunci untuk mendukung sinkronisasi tingkat tinggi.

### Jalur Komunikasi BLE:
1.  **UUID_CONTROL (`...9006`)**:
    - **Properties**: `WRITE` | `NOTIFY`
    - **Descriptor**: `BLE2902` (Hadir ✅)
    - **Fungsi**: Handshake Inisiasi (`0x01`) dan ACK Selesai (`0x01/0x15`).
2.  **UUID_WALLPAPER (`...9002`)**:
    - **Properties**: `WRITE` | `NOTIFY`
    - **Descriptor**: `BLE2902` (Hadir ✅)
    - **Fungsi**: Menerima chunk data dan mengirim **ACK Index (`0x06`)**.

---

## 2. Analisa Masalah: "Fail to subscribe to ACK system"
Jika aplikasi mobile lapor gagal subscribe, berikut adalah penyebab teknis di level BLE:

-   **Analisa A (Race Condition)**: App mencoba melakukan `setNotifyValue(true)` SEBELUM service discovery selesai atau sebelum koneksi benar-benar stabil.
-   **Analisa B (Descriptor Presence)**: Jam lo SUDAH punya deskriptor `0x2902` (Client Characteristic Configuration Descriptor) yang wajib ada buat fitur Notify. Jadi secara hardware/firmware, jam lo SUDAH SIAP diajak ngobrol.
-   **Analisa C (MTU Negotiation)**: Jika App meminta subscribe saat sedang negosiasi MTU, modul BLE ESP32 seringkali menolak request tersebut.

---

## 3. Alur Biner yang Diharapkan Jam (Audit v7.3)

| Langkah | Aksi App | Respon Jam | Data Biner Jam |
| :--- | :--- | :--- | :--- |
| **Inisiasi** | Kirim `0x01` + 4 byte Size | `0x01` (Notify via Control) | `[0x01]` |
| **Streaming** | Kirim Chunk Index 0 | `0x06` + Index (Notify via Wallpaper) | `[0x06, 0x00, 0x00]` |
| **Streaming** | Kirim Chunk Index 1 | `0x06` + Index (Notify via Wallpaper) | `[0x06, 0x01, 0x00]` |
| **Selesai** | Kirim `0x02` | `0x01` (Notify via Control) | `[0x01]` |

---

## 4. Arsitektur Memori & Lifecycle (Watch Side)

Untuk menjaga performa **60 FPS** tanpa membuat LittleFS bekerja terlalu keras (IO Bottleneck), jam tangan menggunakan sistem **Dual-Layer Storage**:

### A. Alur Perjalanan Data (Lifecycle)
1.  **HP (Flutter)** -> Mengirim data RGB565 per chunk (v6.4.4).
2.  **LittleFS (`/wallpaper.bin`)** -> Menampung data biner permanen (134,400 byte).
3.  **RAM Cache (`dynamic_wallpaper`)** -> Saat UI butuh gambar, data dari LittleFS ditarik ke RAM SEKALI SAJA.
4.  **Display (ST7789)** -> UI Manager mengambil pointer dari RAM (instan) untuk di-push ke layar.

### B. Mekanisme Hot-Swap (Tanpa Restart)
Begitu pengiriman wallpaper selesai (`Command 0x02` pada Control Char), jam melakukan perintah internal:
`assets_wallpaper_clear_cache();`
Ini akan mereset bendera `wallpaper_cache_valid` menjadi `false`. Pada loop UI berikutnya, jam akan mendeteksi file baru dan meloadingnya ulang ke RAM secara otomatis.

---

## 5. Cuplikan Kode Internal (Reference for App Dev)

### Fungsi Pemuatan (Loader) - assets_wallpaper.cpp
```cpp
const uint16_t* assets_get_wallpaper() {
    if (LittleFS.exists("/wallpaper.bin")) {
        if (!wallpaper_cache_valid) {
            // Alokasi RAM Dinamis (134,400 bytes)
            if (!dynamic_wallpaper) {
                dynamic_wallpaper = (uint16_t*)malloc(240 * 280 * 2);
            }
            if (dynamic_wallpaper) {
                File f = LittleFS.open("/wallpaper.bin", "r");
                if (f) {
                    f.read((uint8_t*)dynamic_wallpaper, 134400);
                    f.close();
                    wallpaper_cache_valid = true;
                    Serial.println("ASSETS: Wallpaper Cached from LittleFS // [READY]");
                }
            }
        }
        if (wallpaper_cache_valid && dynamic_wallpaper) return dynamic_wallpaper;
    }
    return WALLPAPER_DATA; // Fallback ke default jika file hilang
}
```

---

## 6. Rekomendasi buat Developer Mobile App (Flutter/Native)

1.  **Discover Services First**: Pastikan semua service terdeteksi sebelum panggil fungsi `subscribe`.
2.  **MTU Negotiation**: Lakukan `requestMtu(247)` atau `512`. Ini kritis agar chunk index dan data tidak terpotong di tengah.
3.  **ACK Subscription**: App **WAJIB** sukses subscribe ke `UUID_WALLPAPER` sebelum mengirim chunk pertama. Jika gagal subscribe, App tidak akan tahu jika ada paket yang hilang/korup.
4.  **Format Biner**: Kirim data dalam format **Big Endian RGB565**.

---
*Dibuat oleh Master Agent (Lead Architect) - Phase 5/6 Complete.* 🫡🛡️⌚
