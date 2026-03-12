# ESP32-C3 Smartwatch

## Functional Specification Document (FSD) - Core V1.1 (Ultra Low Power)

---

## 0. Firmware Architecture Guideline

### 0.1 Framework & Compiler Policy

*   **Framework:** Arduino (via PlatformIO)
*   **Platform Version:** `platform = espressif32@6.4.0` (MANDATORY. Newer/older versions cause TFT_eSPI panics and RISC-V bootloops).
*   **Target MCU:** ESP32-C3 Super Mini (Single Core RISC-V, No PSRAM).
*   **UI Graphics:** TFT_eSPI (Bodmer).
*   **Architecture:** Bare-metal discipline wrapped in Arduino.

---

### 0.2 Power Discipline Rules (The "Golden" Rules)

*   **RULE-001 (Deep Sleep):** Deep sleep SHALL be the default system state. TARGET < 50µA.
*   **RULE-002 (No Blocking):** Dilarang keras menggunakan delay() untuk state handling. Gunakan non-blocking timer (millis()).
*   **RULE-003 (Dynamic Frequency Scaling - DFS):** CPU harus berjalan di 160MHz saat render UI/Booting. Saat layar menyala tapi idle (menunggu input tombol), CPU turun ke 80MHz.
*   **RULE-004 (Sensor Isolation):** Sensor I2C (BMI160 & MAX30100) WAJIB di-set ke mode Power-Down/Sleep terdalam via command I2C sebelum MCU mengeksekusi esp_deep_sleep_start().
*   **RULE-005 (Backlight Leakage Prevention):** Pin Backlight (GPIO 10) WAJIB di-set LOW dan PWM timer di-detach sebelum sleep untuk mencegah leakage current pada MOSFET.
*   **RULE-006 (No Gyro Wake):** Fitur Lift/Wrist to Wake ditiadakan demi efisiensi baterai maksimal. MCU murni mengandalkan EXT0 Wake dari Push Button.

---

## 1. Hardware Specification & Pin Mapping

⚠️ **CRITICAL BOOT WARNING:**
GPIO 0 (CHRG) dan GPIO 8 (SDA) adalah Strapping Pins.
Pastikan IC Charger tidak menarik GPIO 0 ke LOW saat proses booting, atau ESP32-C3 akan masuk ke Download Mode (layar blank, seolah-olah mati/brick).

---

### 1.1 Pinout Configuration (Verified)

| Interface | Signal | GPIO | Dir | Notes & Constraints |
| :--- | :--- | :--- | :--- | :--- |
| SPI (TFT) | MOSI | GPIO 6 | Out | SPI Data (TFT_MOSI) |
| SPI (TFT) | SCK | GPIO 4 | Out | SPI Clock (TFT_SCLK) |
| SPI (TFT) | DC | GPIO 2 | Out | Data/Command Selection |
| SPI (TFT) | RST | GPIO 1 | Out | Hardware Reset |
| SPI (TFT) | CS | GND | N/A | Hardwired to GND (-1 in code) |
| SPI (TFT) | BL | GPIO 10 | Out | Backlight via MOSFET (PWM Capable) |
| I2C (Bus) | SDA | GPIO 8 | I/O | Shared (BMI160 + MAX30100). Strapping pin. |
| I2C (Bus) | SCL | GPIO 9 | Out | Shared Bus |
| Input | BTN | GPIO 7 | In | Single Push Button (EXT0 Wake Source) |
| Power | B_READ | GPIO 3 | In | Battery ADC (Voltage Divider) |
| Power | CHRG | GPIO 0 | In | Charging Status (WARNING: Boot Strap!) |

---

## 2. User Interface & Button Logic

Smartwatch ini hanya memiliki 1 Push Button (GPIO 7). Logika interaksi harus ditangani dengan state machine yang solid dan debounce yang sempurna.

---

### 2.1 Global Button Timing

| Action | System State | Duration | Result / Function |
| :--- | :--- | :--- | :--- |
| Wake Press | Deep Sleep | Any | Membangunkan MCU dari Deep Sleep ke WATCHFACE. |
| Short Click | Active/Menu | < 400 ms (Released) | Geser menu ke item berikutnya (Next Menu). |
| Long Hold | Active/Menu | ≥ 1500 ms (Released) | Eksekusi/Select/Enter Menu yang sedang disorot. Di-trigger SETELAH tombol dilepas. |
| Release Guard| Transition | N/A | MCU dilarang mengeksekusi aksi selanjutnya atau masuk Deep Sleep jika tombol masih ditahan. |

---

### 2.2 UI State Machine & Menu Flow

[ DEEP_SLEEP ]
      │ (Press Button GPIO 7)
      ▼
[ WATCHFACE ] ──(Auto Sleep 30s)──► [ DEEP_SLEEP ]
      │
      │ (Short Click / Release < 400ms)
      ▼
[ MENU: HEART RATE ] ──(Release Held ≥ 1.5s)──► [ EXECUTE HR SENSOR ]
      │                                          │ (Short Click or 15s Timeout)
      │                                          ▼
      │                                     [ WATCHFACE ]
      │ (Short Click / Release < 400ms)
      ▼
[ MENU: SETTINGS / AOD ] ──(Release Held ≥ 1.5s)──► [ TOGGLE ALWAYS ON ]
      │
      │ (Short Click / Release < 400ms)
      ▼
[ MENU: FORCE SLEEP ] ──(Release Held ≥ 1.5s)──► [ DEEP_SLEEP ]
      │
      └─(Short Click)──► [ Kembali ke WATCHFACE ]

---

### 2.3 Display Timeout & AOD Policy
*   **Auto-Sleep**: Default 30 detik.
*   **AOD (Always On Display)**:
    *   Jika `isCharging` aktif, AOD menyala otomatis (*default*).
    *   Pengguna bisa mematikan AOD via menu SETTINGS (Long Hold).
    *   Saat AOD Aktif: Background Hitam, Jam Putih Stacked, Brightness 10% (PWM), CPU 40MHz.
*   **Deep Sleep**: Jika AOD mati (Off) dan timer habis, MCU masuk ke Deep Sleep.

### 2.4 Charging Behavior
*   **Charging Icon**: Ikon Petir (Bolt) 16x16 pixel (RGB565 Biner) ditampilkan di sebelah indikator baterai saat `CHRG_PIN` bernilai LOW.
*   **AOD Default**: Secara default jam tidak akan mati (*Always On*) saat sedang dicas untuk memudahkan pemantauan waktu.

---

## 3. Battery & Power Monitoring (V1.1)

*   **ADC Voltage Divider**: Menggunakan resistor **220k Ω** (Vbat to ADC) dan **220k Ω** (ADC to GND). Rasio 1:1 (2.0x multiplier).
*   **Wakeup Reliability (ESP32-C3)**:
    *   GPIO 7 (Active LOW) digunakan sebagai pemicu bangun.
    *   `RTC_PERIPH` power domain dipaksa tetap aktif selama sleep (`ESP_PD_OPTION_ON`) untuk memastikan sirkuit deteksi interrupt digital pada pin digital tetap mendapatkan daya.
    *   Resistor pull-up eksternal (fisik) wajib ada untuk stabilitas.

---

## 4. Tombol Input (User Interaction)

*   **Mode**: Active LOW (Pin GPIO 7 terhubung ke Ground saat ditekan).
*   **Internal Pull-up**: WAJIB diaktifkan (`INPUT_PULLUP`).
*   **Wakeup Source**: GPIO 7 dengan trigger level `LOW`.
*   **Timing**: (Lihat Tabel 2.1).

---

## 5. Asset Management (Wallpapers)

*   **ASSET-001:** Biner gambar (Watchface background, Ikon UI) TIDAK BOLEH ditulis mentah di main.cpp.
*   **ASSET-002:** Buat file khusus `lib/assets/assets_wallpaper.h` atau `.cpp` untuk menampung data array.
*   **ASSET-003:** Biner gambar wajib menggunakan macro PROGMEM agar disimpan di Flash Memory, bukan menguras RAM. Format yang digunakan harus RGB565 array yang kompatibel dengan fungsi pushImage dari TFT_eSPI.

---

## 5. PlatformIO Configuration (MANDATORY)

File `platformio.ini` WAJIB dikonfigurasi menggunakan build flags untuk TFT_eSPI, bukan dengan mengedit User_Setup.h di dalam library. Ini mempermudah debugging dan mencegah library ter-reset saat update.

```ini
[env:esp32c3]
platform = espressif32@6.4.0
board = esp32-c3-devkitm-1
framework = arduino

; Serial Monitor setup for debugging
monitor_speed = 115200
monitor_filters = esp32_exception_decoder, time

build_flags =
    ; CORE & DEBUGGING FLAGS
    -D CORE_DEBUG_LEVEL=3       ; Info level for Serial output
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1 ; Allow Serial Monitor via Native USB
    
    ; TFT_eSPI CONFIGURATION (Dynamic Injection, DO NOT edit User_Setup.h)
    -D USER_SETUP_LOADED=1
    -D ST7789_DRIVER=1
    -D TFT_WIDTH=240
    -D TFT_HEIGHT=280
    -D TFT_RGB_ORDER=TFT_RGB
    -D TFT_MISO=-1
    -D TFT_MOSI=6
    -D TFT_SCLK=4
    -D TFT_CS=-1
    -D TFT_DC=2
    -D TFT_RST=1
    -D TFT_BL=10
    -D LOAD_GLCD=1
    -D LOAD_FONT2=1
    -D SPI_FREQUENCY=40000000

lib_deps =
    bodmer/TFT_eSPI @ ^2.5.31
```

---

## 6. Target Directory Structure

AI Developer SHALL format the project exactly like this:

```
ESP WATCH V3 ST7789 C3 Supermini/
│
├── platformio.ini
│
├── include/
│   ├── config_pins.h       # Pinout definitions only
│   ├── system_states.h     # Enums for UI and Power states
│
├── src/
│   ├── main.cpp            # Setup and State Machine loop ONLY
│   ├── power_manager.cpp   # Deep Sleep, DFS (set_cpu_freq_mhz), Wake logic
│   ├── ui_manager.cpp      # Menu rendering and Button logic routing
│
├── lib/
│   ├── drivers/
│   │   ├── button_hal.cpp  # Debounce, Short Click (<400ms), Long Hold (≥1500ms)
│   │   ├── display_hal.cpp # TFT_eSPI wrappers and PWM Backlight fade logic
│   │   ├── sensors_hal.cpp # I2C, BMI160, MAX30100 init and I2C sleep commands
│   │
│   └── assets/
│       ├── assets_wallpaper.cpp # PROGMEM RGB565 arrays
│       └── assets_icons.cpp
│
└── docs/
    └── FSD.md
```

---

## 7. Development Stepping (For AI / Developer)

AI Assistant harus mengerjakan fitur dengan urutan prioritas berikut agar pondasi power management stabil sebelum UI dibangun:

1.  **Step 1: Button & Boot Core**
    Implementasi `button_hal.cpp` (Short click vs 1.5s Hold detection) dan Release Guard. Print log ke Serial. Pastikan GPIO 7 berfungsi sebagai input yang stabil.

2.  **Step 2: Power Management**
    Implementasi `power_manager.cpp` (Fungsi masuk Deep Sleep via `esp_deep_sleep_start()`, bangun via EXT0 di GPIO 7, dan uji coba DFS).

3.  **Step 3: Display Driver**
    Implementasi `display_hal.cpp` (Render UI dasar + hardware PWM Backlight Fade in/out di GPIO 10 menggunakan ledc).

4.  **Step 4: UI & State Machine**
    Gabungkan UI logic (`ui_manager.cpp`) dengan State Machine di `main.cpp` (Perpindahan menu dengan timer Auto-Sleep 30s).

5.  **Step 5: Sensors**
    Implementasi pembacaan Sensor via I2C (`sensors_hal.cpp`) dan integrasikan dengan menu Heart Rate. Pastikan sensor mati sempurna sebelum deep sleep.

---

## 8. Low Battery Strategy

*   Low Battery Threshold: 3.50V (Warning Level).
*   Critical Battery Threshold: 3.40V (Force Deep Sleep).
*   Saat mencapai Warning Level:
    *   Nonaktifkan AOD.
    *   Turunkan CPU ke 40MHz.
    *   Batasi brightness maksimal 50%.
    *   Tampilkan ikon Low Battery.
*   Saat mencapai Critical Level:
    *   Tampilkan notifikasi "LOW BATTERY - SLEEPING".
    *   Matikan sensor.
    *   Masuk Deep Sleep otomatis.
*   Sistem hanya boleh bangun kembali jika tombol ditekan atau charger terdeteksi.

---

## 9. Fault Handling & Recovery Policy

*   Jika I2C device tidak merespon:
    *   Lakukan re-init bus maksimal 3 kali.
    *   Jika gagal, disable fitur sensor dan kembali ke WATCHFACE.
*   Jika ADC membaca nilai tidak masuk akal:
    *   Abaikan pembacaan dan gunakan nilai terakhir yang valid.
*   Jika terjadi I2C bus lock:
    *   Reconfigure pin sebagai GPIO output dan generate clock pulse manual sebelum re-init Wire.
*   Watchdog Timer wajib diaktifkan pada production build.
*   Jika sistem mengalami crash berulang (lebih dari 3 kali dalam 1 menit):
    *   Masuk ke Safe Mode (tanpa sensor, tanpa BLE).

---

## 10. Memory & Resource Policy

*   Dilarang menggunakan dynamic allocation (malloc/new) pada loop utama.
*   Semua buffer display dan sensor harus static allocation.
*   Asset gambar wajib menggunakan PROGMEM.
*   Target penggunaan RAM maksimum 70% dari total SRAM.
*   Target penggunaan Flash maksimum 80%.
*   Serial logging level harus diturunkan pada production build.

---

-- END OF DOCUMENT --
