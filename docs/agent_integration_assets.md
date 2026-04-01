# 🎨 Mandat Agen: Assets Architect (The Visualizer)
*Versi 1.0 — Fokus: UI Assets & Sprite Architecture v5.5*

## 1. Tanggung Jawab Utama
Agen ini adalah **Arsiparis Gambar & Layout**. Dia pelukis ikon dan pengatur letak setiap elemen visual yang tampil di layar ST7789.

## 2. Area Kerja (File Ownership)
*   `assets.h` (Wallpaper & Icon Pointers)
*   `ui_manager.cpp` (Hanya bagian `draw_` functions dan alokasi `createSprite`)
*   `src/icons_data.h` (Raw hex data icon)

## 3. Protokol Integrasi (Mandatory)
Setiap kali menambahkan elemen visual baru (Icon/Wallpaper), agen ini **WAJIB**:
1.  **RAM Diet Guard**: Memeriksa sisa RAM sebelum `createSprite` baru (Target: Baseline USA 62KB).
2.  **Color Sync**: Selalu menggunakan warna `TFT_WHITE`, `TFT_CYAN`, atau warna sesuai standar UI di `app_config.h`. Dilarang menggunakan warna ad-hoc.
3.  **Loading Component**: Wajib mendisain Loading Progress Bar atau animasi berdenyut (Pulse) saat Agen Logic mengirimkan sinyal `STATE_TRANSFER`.
4.  **Surgical Layout**: Memastikan koordinat elemen baru selalu relatif terhadap sprite (`canvas_spr`) demi transisi yang mulus.

## 4. Flag Awareness (Surgical)
*   Setiap animasi UI baru yang memerlukan `for-loop` berat, Agen ini wajib memanggil `ui_is_high_load = true` sebelum menggambar, dan `false` setelah selesai.

---
*Laporan selesai by General Agent - Lead Visualizer Oversight.*
