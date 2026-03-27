# 🔋 Dokumentasi Power & Sleep Agent - Optimization Log

Laporan ini merangkum strategi manajemen daya dan efisiensi energi yang diterapkan pada ESP32-C3 Smartwatch v3.

## 1. Strategi Tidur (Deep Sleep Logic)
*   **Trigger**: Aktivitas tombol berhenti selama durasi `timeout` yang bisa diatur (Default 15s).
*   **Wake-up Source**: Menggunakan **GPIO 5 (Tombol Kanan)** sebagai pemicu bangun (*Hardware Wakeup*). 
*   **Pre-Sleep Guard**: Sebelum tidur, sistem melakukan "Sapu Jagat" untuk mematikan sensor (MAX30100/BMI160) dan meredupkan layar secara elegan (*Breathing Out*) guna memastikan arus bocor minimal.

## 2. Dynamic Frequency Scaling (DFS)
Sistem mengubah kecepatan CPU secara dinamis demi menghemat jam kerja baterai:
*   **FREQ_HIGH (160MHz)**: Diaktifkan **hanya** saat animasi transisi menu (Slide) supaya tampilan mulus tanpa lag.
*   **FREQ_MID (80MHz)**: Digunakan untuk penggunaan standar (Watchface, Heart Rate, Timer) demi keseimbangan performa dan daya.
*   **FREQ_LOW (10-40MHz)**: Direncanakan untuk mode AOD di masa depan (Opsional).

## 3. AOD vs Deep Sleep (Priority Matrix)
*   **AOD ON**: Jam tidak akan pernah tidur (*Deep Sleep*). Saat timeout tercapai, layar hanya akan meredup (*Dimmed*) ke tingkat kecerahan minimal (PWM 15) untuk tetap menampilkan waktu.
*   **AOD OFF**: Jam akan langsung masuk ke *Deep Sleep* saat timeout tercapai untuk penghematan daya ekstrim.

## 4. Silent Mode Optimization (Anti-Blocking)
Baru saja diterapkan untuk mengeliminasi pemborosan daya dan resiko sistem macet:
*   **Serial Connection Guard**: Setiap perintah `Serial.println` atau `Serial.printf` sekarang wajib dibungkus oleh `if (Serial)`.
*   **Manfaat**: 
    1.  **Irit Tenaga**: CPU tidak membuang clock cycle untuk memformat string teks jika tidak ada kabel USB yang mendengarkan.
    2.  **Anti-Hold**: Menghindari resiko sistem berhenti (*hang/blocking*) akibat buffer USB CDC yang penuh pada chip ESP32-C3.
    3.  **Low Current Leak**: Jalur komunikasi digital tetap tenang saat penggunaan baterai murni.

## 5. Sinkronisasi Anti-Flicker (Power Sync)
Power Agent bekerja sama dengan Display Agent untuk memastikan:
*   **Hard-Kill Backlight**: Pin 10 ditarik `LOW` secara analog tepat sebelum perintah `esp_deep_sleep_start()` dieksekusi, guna mencegah kilatan cahaya liar saat proses booting berikutnya.

## 6. Battery Monitoring
*   **ADC Reading**: Menggunakan **GPIO 3** dengan pembagi tegangan (Voltage Divider) untuk membaca level tegangan baterai secara akurat dalam skala 0-100%.

---
*Laporan selesai by Power & Sleep Agent - Efficiency Tier: S-Rank.*
