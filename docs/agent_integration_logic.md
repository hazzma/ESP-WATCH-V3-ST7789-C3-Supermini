# 📜 Mandat Agen: Logic Integrator (The Manager)
*Versi 1.0 — Fokus: State & Navigation Logic v5.5*

## 1. Tanggung Jawab Utama
Agen ini adalah **Arsitek Alur**. Dia bertugas mengatur kapan sebuah menu muncul, bagaimana transisinya, dan apa yang terjadi saat tombol ditekan.

## 2. Area Kerja (File Ownership)
*   `include/system_states.h` (State Definitions)
*   `src/ui_manager.cpp` (Update & Transition Loop)
*   `src/ui_states.h` (Internal UI logic)

## 3. Protokol Integrasi (Mandatory)
Setiap kali menambahkan fitur baru (misal: BLE/Wallpaper Update), agen ini **WAJIB**:
1.  **State Creation**: Menambahkan enum state baru di `system_states.h`.
2.  **Navigation Interlock**: Menghubungkan state baru tersebut ke sistem navigasi (Tombol Kanan/Kiri).
3.  **Loading Controller**: Jika fitur membutuhkan waktu tunggu (seperti download data), Agen ini wajib memicu state `STATE_WAITING` atau loading screen.
4.  **No Ghosting**: Mastiin pembersihan VRAM (`tft.pushImage`) dilakukan saat pindah dari menu full-black ke wallpaper.

## 4. Flag Awareness (Surgical)
*   Agen Logic harus mengirimkan sinyal `ui_is_high_load = true` sebelum memulai animasi besar guna memblokir tugas latar belakang yang berat.

---
*Laporan selesai by General Agent - Lead Logic Oversight.*
