# Laporan Pengembangan UI/UX Eksperimental

Dibuat oleh: **Antigravity UI/UX Agent**  
Target: ESP32-C3 Watch (ST7789)  
Revisi Terakhir: **v8 — "State Sync & HR Exit"**

---

## 1. Ringkasan Versi

| Versi | Topik | Status |
|-------|-------|--------|
| v6 | Chroma Key transparency | Digantikan (pink artefak) |
| v7 | Wallpaper Injection + Pre-render Slide | Digantikan (bug watchface shadow) |
| v8 | **Full State Sync + HR Exit + No Ghosting** | ✅ **AKTIF - CHECKPOINT 1** |

---

## 2. Perbaikan & Fitur Baru (v8)

### 2.1 Fix: Watchface Ghosting
Ditemukan bug di mana angka jam dari Home masih membekas saat masuk ke menu.  
**Solusi:** Menambahkan deteksi state sebelumnya. Jika transisi berasal dari `STATE_WATCHFACE` atau `STATE_EXEC_HR` (yang berlatar hitam), sistem dipaksa melakukan *Full Wallpaper Push* (240x280) untuk membersihkan layar secara total.

### 2.2 Fix: Menu Refresh Loop
Sebelumnya menu me-refresh setiap 2000ms secara paksa.  
**Solusi:** Menyinkronkan variabel `last_min_val` setiap kali Top Clock digambar di menu. Sekarang menu benar-benar statis dan hanya akan menggambar ulang tepat saat menit pada jam berganti.

### 2.3 Fix: Logic Selection & Toggles
Menambahkan flag `logical_change` untuk memastikan render dipicu saat ada perubahan nilai (seperti toggle AOD atau adj. brightness) tanpa harus merubah state utama.

### 2.4 Fitur: HR Measure Exit
Sekarang mode "Measuring Heart Rate" bisa dihentikan dengan menahan tombol kanan selama **0.8 detik**, memberikan kontrol penuh kepada pengguna untuk kembali ke menu tanpa menunggu sensor selesai.

---

## 3. Detail Navigasi (Checkpoint 1)

| Input | Aksi di Home | Aksi di Menu | Aksi di Transisi |
|-------|--------------|--------------|-------------------|
| **Tombol Kiri (Klik)** | Masuk ke Brightness | Menu Sebelumnya | Pindah Cepat |
| **Tombol Kanan (Klik)** | Masuk ke HR | Menu Selanjutnya | Pindah Cepat |
| **Tombol Kanan (Hold 0.8s)** | - | **Select / Toggle** | Keluar / Masuk |

---

## 4. Arsitektur Teknis

- **Wallpaper Injection**: Setiap sprite digambar di atas potongan wallpaper asli secara memori (tidak ada Chroma Key).
- **Dual-Buffer Brightness**: Progress bar brightness digambar di dalam sprite sebelum di-push ke layar untuk menjamin **Zero Flicker**.
- **CPU Boost**: Frekuensi dilonjakkan ke **160MHz** hanya saat animasi geser berlangsung.

---

*Status: Checkpoint 1 - Navigasi Dual-Button & Smooth UI Sempurna.*
