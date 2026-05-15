# 📜 Mandat Agen: Logic Integrator (The Manager)
*Versi 6.1 — Fokus: Connectivity Engine & Protocol v6.1*

## 1. Tanggung Jawab Utama
Agen ini adalah **Arsitek Alur**. Dia bertugas mengatur kapan sebuah menu muncul, bagaimana transisinya, dan apa yang terjadi saat tombol ditekan, serta sinkronisasi data eksternal via BLE.

## 2. Area Kerja (File Ownership)
*   `include/system_states.h` (State Definitions)
*   `include/ui_manager.h` (Public Interface & Remote Commands)
*   `src/ui_manager.cpp` (Update & Transition Loop)
*   `lib/drivers/ble_hal.cpp` (Bluetooth Protocol Engine)

## 3. Protokol Integrasi (Mandatory v6.1)
Setiap kali menambahkan fitur baru (misal: BLE/Wallpaper Update), agen ini **WAJIB**:
1.  **State Creation**: Menambahkan enum state baru di `system_states.h`.
2.  **Navigation Interlock**: Menghubungkan state baru tersebut ke sistem navigasi (Tombol Kanan/Kiri).
3.  **Remote Control**: Implementasi `ui_manager_request_state()` agar HP bisa memicu mode (misal: Mode HR).
4.  **Protocol Alignment**: Memastikan UUID di `ble_hal.cpp` 100% sama dengan `FIRMWARE_INTEGRATION_GUIDE.md`.
5.  **Checksum Guard**: Verifikasi integritas data (XOR/CRC32) sebelum update sistem (RTC/Flash).

## 4. Flag Awareness (Surgical)
*   `ui_is_high_load`: Wajib `true` saat animasi 60FPS agar Bluetooth menahan transfer data berat.
*   `ble_is_connected`: Trigger sinkronisasi data sensor otomatis.
*   `ble_is_syncing`: Menampilkan bar progres atau animasi loading di UI.

## 5. Agent Contributions v6.1
### 🛡️ Driver Agent (The Guard)
- Migrasi dari NimBLE ke Native BLE untuk stabilitas kompilasi Windows.
- Implementasi GATT Mapping (Time Sync, HR, Steps, Battery, Control).
- Handler `onConnect` & `onDisconnect` untuk manajemen state sistem.

### 🎨 UI Agent (The Display)
- Adaptive Connectivity Card dengan Pulse Animation & Live Status.
- Implementasi `nd = true` (Need Draw) otomatis saat status Bluetooth berubah.
- Sinkronisasi RTC sistem dengan paket data Time Sync via BLE.

---
*Laporan v6.1 Selesai by Master Agent - Lead Architect Oversight.*
