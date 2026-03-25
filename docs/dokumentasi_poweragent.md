# Laporan Pengembangan Power Management & Auto-Sleep

Dibuat oleh: **Power Agent**  
Target: ESP32-C3 Power Domains  
Fitur: ✅ **Quick Kill-Switch** & ✅ **Smart Auto-Sleep**

---

## 1. Quick Kill-Switch (Hotkey Sleep)

Untuk memudahkan pengguna mematikan jam tanpa harus masuk ke menu, saya mengimplementasikan **Hotkey Shutdown** pada tombol kiri.

- **Input**: **GPIO 7** (Tombol Kiri).
- **Aksi**: Hold selama **0.8 detik**.
- **Hasil**: Jam langsung mematikan semua sensor, meredupkan layar, dan masuk ke **Deep Sleep**.
- **Context**: Bisa dilakukan dari layar mana saja (Watchface/Menu).

```cpp
// Logic di ui_manager.cpp
else if (bt == BTN_LEFT_HOLD) {
    power_manager_enter_deep_sleep();
}
```

---

## 2. Smart Auto-Sleep System

Jam tangan sekarang memiliki timer otomatis yang mendeteksi ketidakaktifan pengguna (*idle*).

- **Default Timeout**: 15 Detik (Bisa diubah lewat `power_manager_set_auto_sleep_timeout`).
- **Logic Cabang**:
    1. **AOD ON**: Jika pengaturan AOD menyala, jam hanya akan **meredupkan layar** (Dimmed mode) ke kecerahan minimal (15%) agar tetap bisa melihat jam.
    2. **AOD OFF**: Jika pengaturan AOD mati, jam akan langsung masuk ke **Deep Sleep** (Mati total) untuk menghemat baterai secara maksimal.

```cpp
// Algoritma Penentu di ui_manager_update()
uint32_t timeout_ms = power_manager_get_auto_sleep_timeout() * 1000;
if (now - last_activity_time > timeout_ms) {
    if (aod_allowed) is_dimmed_aod = true; // Cuma redup
    else power_manager_enter_deep_sleep(); // Mati total
}
```

---

## 3. Integrasi Versi Berikutnya (UI Setup)

Bagi **Agent UI**, berikut adalah *endpoint* yang bisa digunakan untuk membuat halaman pengaturan waktu tidur:

- **Getter**: `power_manager_get_auto_sleep_timeout()` (mengembalikan nilai `uint32_t` dalam detik).
- **Setter**: `power_manager_set_auto_sleep_timeout(uint32_t seconds)`.

> ⚠️ Nilai ini disimpan di **RTC RAM**, sehingga tidak akan hilang meskipun jam dimatikan.

---

*Status: Hemat Daya & Responsif.*
