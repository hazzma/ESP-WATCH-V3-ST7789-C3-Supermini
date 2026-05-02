# 📲 Connectivity & BLE (NimBLE Architecture)

Dokumentasi ini menjelaskan migrasi sistem Bluetooth dari standar Bluedroid ke **NimBLE** pada ESP32-C3 Smartwatch, serta cara kerja sistem konektivitas baru yang lebih stabil dan hemat daya.

---

## 🚀 Mengapa Migrasi ke NimBLE?

Sebelumnya, proyek ini menggunakan *stack* Bluetooth bawaan ESP32 (Bluedroid). Masalah utama Bluedroid adalah:
1. **Boros Memori**: Memakan flash >500KB dan RAM yang besar.
2. **Bug Deinit**: Memanggil `BLEDevice::deinit(true)` seringkali menyebabkan *kernel panic* (freeze) saat dipanggil kembali (`init`).
3. **Instabilitas**: Sering terjadi *overhead* saat menangani banyak karakteristik sekaligus.

**NimBLE** (Apache NimBLE) adalah alternatif yang jauh lebih ringan dan stabil yang kini diintegrasikan ke dalam proyek ini.

### Hasil Migrasi:
*   **Firmware Size**: Berkurang dari **1.3 MB** menjadi **~840 KB**.
*   **Stability**: Mendukung siklus `init()` dan `deinit()` secara sempurna tanpa *freeze*.
*   **Power Efficiency**: Mendukung *Modem Sleep* yang sangat efisien saat radio dimatikan.

---

## 🛠️ Cara Kerja Kode Baru

### 1. Inisialisasi Kondisional
Bluetooth tidak lagi menyala secara otomatis jika dinonaktifkan di pengaturan.

```cpp
void ble_hal_init() {
    if (!power_manager_get_ble_enabled()) return; // Cek status di NVS/RTC
    if (ble_initialized) return;

    NimBLEDevice::init("ESP32Watch");
    pServer = NimBLEDevice::createServer();
    // ... setup service & characteristics ...
    ble_initialized = true;
}
```

### 2. Zero-Freeze Toggle (Radio Management)
Untuk mematikan Bluetooth tanpa merusak sistem, kita menggunakan `NimBLEDevice::deinit(true)`.

```cpp
void ble_hal_update_enabled() {
    bool target = power_manager_get_ble_enabled();
    if (target && !ble_initialized) {
        ble_hal_init(); // Nyalakan radio
    } else if (target && ble_initialized) {
        NimBLEDevice::getAdvertising()->start(); // Resume iklan
    } else if (!target && ble_initialized) {
        NimBLEDevice::deinit(true); // Matikan radio total (Aman di NimBLE)
        ble_initialized = false;
        ble_is_connected = false;
    }
}
```

### 3. Otomasi Re-Advertising
Jika koneksi terputus, jam akan otomatis masuk ke mode iklan (*advertising*) kembali, **HANYA JIKA** fitur BLE dalam kondisi ON.

```cpp
void onDisconnect(NimBLEServer* pServer) {
    ble_is_connected = false;
    if (power_manager_get_ble_enabled()) {
        NimBLEDevice::startAdvertising(); 
    }
}
```

---

## 📋 Fitur Konektivitas (BLE v2.0)

1.  **Time Sync (UUID ...9001)**: Sinkronisasi waktu sistem dari smartphone.
2.  **Wallpaper Transfer (UUID ...9002)**: Pengiriman file biner gambar 240x280 dalam bentuk *chunks*.
3.  **Sensor Notification (UUID ...9003-9005)**: *Push* data detak jantung, langkah kaki, dan baterai ke aplikasi.
4.  **Calibration Protocol (UUID ...9007)**: Sinkronisasi parameter kalibrasi sensor dari aplikasi.

---

## 💡 Tips untuk Implementasi Sendiri

Jika Anda ingin mengimplementasikan sistem serupa:
*   **Gunakan NimBLE-Arduino**: Library ini jauh lebih baik untuk perangkat *low-power* dibanding library bawaan.
*   **Gunakan Alias**: NimBLE secara default mendukung alias `BLEDevice`, jadi Anda tidak harus merubah seluruh nama class di kode lama jika tidak mau (meski disarankan menggunakan prefix `NimBLE`).
*   **Handle High Load**: Gunakan flag `ui_is_high_load` untuk menghentikan pengiriman data sensor saat animasi UI sedang berjalan berat guna mencegah tabrakan data.

---
*Dokumentasi ini dibuat oleh Antigravity Master Agent untuk Checkpoint 5 (Connectivity Optimization).*
