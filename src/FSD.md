# ESP32-C3 Smartwatch
## Functional Specification Document — v2.0 (Living Document)

> **Cara baca dokumen ini:**
> Setiap section punya status tag:
> - `[SPEC]` — belum diimplementasi, ini target
> - `[VERIFIED ✓]` — sudah confirmed works di hardware, interface LOCKED
> - `[RESERVED]` — direncanakan tapi tidak diimplementasi di v1, jangan disentuh agent
> - `[DEBUG]` — aktif saat development, di-comment saat final build

---

## 0. Firmware Architecture

### 0.1 Platform & Compiler `[SPEC]`

| Key | Value |
| :--- | :--- |
| Framework | Arduino via PlatformIO |
| Platform | `espressif32@6.4.0` — **WAJIB versi ini. Jangan upgrade.** |
| MCU | ESP32-C3 Super Mini — Single Core RISC-V, No PSRAM |
| Display Library | TFT_eSPI (Bodmer) `^2.5.31` |
| Sensor HR | MAX30100lib (oxullo) `^1.2.1` |
| Sensor IMU | Bmi160 (bolderflight) `^3.0.1` |
| Architecture | Bare-metal discipline wrapped in Arduino |

### 0.2 Golden Rules `[SPEC]`

> Agent wajib cite rule ID kalau ada keputusan implementasi yang berkaitan.

| ID | Rule | Berlaku di |
| :--- | :--- | :--- |
| RULE-001 | Deep sleep adalah default state. Target < 50µA | `power_manager` |
| RULE-002 | `delay()` DILARANG kecuali di I2C retry block (maks 3×50ms). Semua timer pakai `millis()` | Semua file |
| RULE-003 | DFS: 160MHz saat render, 80MHz saat idle display on, 40MHz saat AOD | `power_manager`, `ui_manager` |
| RULE-004 | Sensor wajib di-shutdown via I2C command sebelum `esp_deep_sleep_start()` | `power_manager` |
| RULE-005 | GPIO 10 di-set LOW dan LEDC di-detach sebelum sleep | `power_manager` |
| RULE-006 | Wake source hanya EXT0 GPIO 5. Tidak ada gyro/accelerometer wake | `power_manager` |
| RULE-007 | MAX30100 lazy init — `pox.begin()` hanya dipanggil saat masuk state `EXEC_HR` | `max30100_hal` |
| RULE-008 | `Wire.begin(8, 9)` dipanggil **tepat satu kali** di `setup()` saja, tidak di tempat lain | `main.cpp` |
| RULE-009 | Debug Serial log aktif saat development. Saat user bilang "final", semua `Serial.print` debug di-comment — bukan dihapus | Semua file |
| RULE-010 | BMI160 dan fitur-fiturnya (step counter, fall detection) adalah RESERVED. Stub interface saja, jangan implementasi | `bmi160_hal` |
| RULE-011 | Asset gambar (wallpaper, ikon) DILARANG ditulis langsung di `ui_manager` atau `main.cpp`. Semua array RGB565 wajib ada di `lib/assets/` dengan `PROGMEM`. UI layer hanya boleh `#include` dan memanggil fungsi getter dari asset file | `assets_*`, `ui_manager` |

### 0.3 Debug vs Final Build Policy `[SPEC]`

Semua baris debug ditandai dengan komentar `// [DEBUG]` di akhir baris:

```cpp
Serial.println("[BTN] Short click detected");  // [DEBUG]
Serial.print("[HR] BPM: ");                     // [DEBUG]
Serial.println(pox.getHeartRate());             // [DEBUG]
```

Saat user minta "final build", agent cukup comment semua baris bertanda `// [DEBUG]`.
**Jangan hapus** — biar bisa di-uncomment lagi kalau perlu debug ulang.

---

## 1. Hardware & Pin Mapping

### 1.1 Pinout `[VERIFIED ✓]`

> Pinout ini sudah dikonfirmasi benar di hardware. Tidak boleh diubah oleh agent.

| Interface | Signal | GPIO | Dir | Catatan |
| :--- | :--- | :--- | :--- | :--- |
| SPI (TFT) | MOSI | 6 | Out | |
| SPI (TFT) | SCK | 4 | Out | |
| SPI (TFT) | DC | 2 | Out | |
| SPI (TFT) | RST | 1 | Out | |
| SPI (TFT) | CS | GND | — | Hardwired, `-1` in code |
| SPI (TFT) | BL | 10 | Out | MOSFET + LEDC PWM |
| I2C | SDA | 8 | I/O | Shared bus. **Strapping pin — lihat warning** |
| I2C | SCL | 9 | Out | Shared bus |
| Input | BTN_L | 7 | In | Nav Left (Prev Menu) |
| Input | BTN_R | 5 | In | Nav Right (Next), Select, Wake-up |

> ⚠️ **BOOT WARNING:** GPIO 0 dan GPIO 8 adalah strapping pins. Pastikan tidak ditarik LOW saat boot atau ESP32-C3 masuk Download Mode.

### 1.2 Battery ADC `[SPEC]`

- Voltage divider: 220kΩ (Vbat → ADC) + 220kΩ (ADC → GND)
- Multiplier: **2.0×**
- Formula: `Vbat = (analogRead(3) / 4095.0) * 3.3 * 2.0`

---

## 2. PlatformIO Configuration `[SPEC]`

```ini
[env:esp32c3]
platform = espressif32@6.4.0
board = esp32-c3-devkitm-1
framework = arduino

monitor_speed = 115200
monitor_filters = esp32_exception_decoder, time

build_flags =
    -D CORE_DEBUG_LEVEL=3
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1
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
    oxullo/MAX30100lib @ ^1.2.1
    bolderflight/Bmi160 @ ^3.0.1
```

---

## 3. Directory Structure `[SPEC]`

```
ESP32-C3-Smartwatch/
├── platformio.ini
├── include/
│   ├── config_pins.h        # Semua #define GPIO — satu-satunya sumber pin
│   ├── system_states.h      # Enum AppState, ButtonEvent
│   └── app_config.h         # Konstanta: timeout, threshold, freq
├── src/
│   ├── main.cpp             # setup() + loop() tipis ONLY
│   ├── power_manager.cpp
│   ├── power_manager.h
│   ├── ui_manager.cpp
│   └── ui_manager.h
├── lib/
│   ├── drivers/
│   │   ├── display_hal.cpp
│   │   ├── display_hal.h
│   │   ├── button_hal.cpp
│   │   ├── button_hal.h
│   │   ├── max30100_hal.cpp
│   │   ├── max30100_hal.h
│   │   ├── bmi160_hal.cpp        # RESERVED — stub only
│   │   └── bmi160_hal.h          # RESERVED — stub only
│   └── assets/
│       ├── assets_wallpaper.cpp  # PROGMEM RGB565 array — watchface background
│       ├── assets_wallpaper.h    # Getter: const uint16_t* assets_get_wallpaper()
│       ├── assets_icons.cpp      # PROGMEM RGB565 arrays — charging bolt, batt icon
│       └── assets_icons.h        # Getter per ikon
└── docs/
    └── FSD_v2.0.md
```

---

## 4. Development Steps & Status

> Ini adalah urutan build yang wajib diikuti. Agent hanya boleh mengerjakan step yang statusnya `[CURRENT]`. Step yang `[LOCKED]` tidak boleh dimodifikasi.

| Step | Domain | Files | Status |
| :--- | :--- | :--- | :--- |
| 1 | Display HAL | `display_hal.cpp/h` | `[VERIFIED ✓]` |
| 2 | Button HAL | `button_hal.cpp/h` | `[VERIFIED ✓]` |
| 3 | Sensor HAL | `max30100_hal.cpp/h`, `bmi160_hal.cpp/h` | `[VERIFIED ✓]` |
| 4 | UI + State Machine + Assets | `ui_manager.cpp/h` | `[VERIFIED ✓]` |
| 5 | Power & Sleep | `power_manager.cpp/h` | `[VERIFIED ✓]` |

---

## 5. Locked Interfaces

> Section ini diisi secara bertahap setelah tiap step `[VERIFIED ✓]`.
> Semua interface di sini adalah kontrak — agent tidak boleh mengubah signature yang sudah locked.

### 5.1 LOCKED: display_hal (Step 1 — Verified on Hardware)
```cpp
// DO NOT MODIFY — downstream agents depend on this
void display_hal_init();
void display_hal_backlight_set(uint8_t brightness);
void display_hal_backlight_fade_in(uint8_t target, int duration_ms);
void display_hal_backlight_fade_out();
TFT_eSPI& display_hal_get_tft();
// Notes: LEDC channel 0 reserved for backlight.
//        All draw calls must use display_hal_get_tft().
//        Never call tft. directly outside display_hal.
```

### 5.2 LOCKED: button_hal (Step 2 — Verified on Hardware)
```cpp
typedef enum { BTN_NONE, BTN_SHORT_CLICK, BTN_LONG_HOLD } ButtonEvent;
void button_hal_init();
ButtonEvent button_hal_read();  // non-blocking, call every loop
// Notes: All events fire on RELEASE only.
//        Release guard active — BTN_NONE while GPIO7 is LOW.
```

### 5.3 LOCKED: sensor_hal (Step 3 — Verified on Hardware)
```cpp
bool    max30100_hal_init();
void    max30100_hal_update();
void    max30100_hal_shutdown();
float   max30100_hal_get_bpm();
uint8_t max30100_hal_get_spo2();
bool    bmi160_hal_init();      // STUB — RULE-010
void    bmi160_hal_shutdown();  // STUB — RULE-010
// Notes: Wire.begin() is in main.cpp, not here.
//        max30100_hal_init() is lazy — EXEC_HR state only.
```

### 5.4 LOCKED: ui_manager (Step 4 — Verified on Hardware)
```cpp
void ui_manager_init();
void ui_manager_update();  // call every loop
// Notes: State machine owns sensor lifecycle in EXEC_HR.
//        Auto-AOD timer (60s) resets on every button event.
//        AOD state persists in RTC_DATA_ATTR.
```

### 5.5 LOCKED: assets (Step 4 — Verified on Hardware)
```cpp
const uint16_t* assets_get_wallpaper();
uint16_t assets_get_wallpaper_width();   // 240
uint16_t assets_get_wallpaper_height();  // 280
const uint16_t* assets_get_icon_charging();
const uint16_t* assets_get_icon_battery();
uint16_t assets_get_icon_width();   // 16
uint16_t assets_get_icon_height();  // 16
// Notes: Replace placeholder arrays in .cpp with real RGB565 data anytime.
//        No other file changes needed when swapping images.
```

### 5.6 LOCKED: power_manager (Step 5 — Verified on Hardware)
```cpp
void power_manager_init();
void power_manager_set_freq(int mhz);
void power_manager_enter_deep_sleep();
float power_manager_read_battery_voltage();
// Notes: Pre-sleep follows 8-step mandatory order.
//        Wake source: GPIO 5 (Pull-up, Ground to Wake).
```

---

## 6. Step 1 — Display HAL `[CURRENT]`

### 6.1 Tanggung Jawab

`display_hal` adalah satu-satunya layer yang boleh menyentuh TFT_eSPI dan LEDC backlight.
Tidak ada file lain yang boleh memanggil `tft.` secara langsung — semua lewat fungsi dari `display_hal`.

### 6.2 Interface yang Harus Diimplementasi

```cpp
// Init — dipanggil sekali di setup()
void display_hal_init();

// Backlight control via LEDC PWM (channel 0, 5000Hz, 8-bit, GPIO 10)
void display_hal_backlight_set(uint8_t brightness);   // 0=off, 255=full
void display_hal_backlight_fade_in(uint8_t target, int duration_ms);  // non-blocking
void display_hal_backlight_fade_out();  // synchronous — boleh blocking, hanya dipanggil pre-sleep

// Akses ke TFT object — semua render pakai ini
TFT_eSPI& display_hal_get_tft();
```

### 6.3 Success Criteria (Test di Hardware)

Agent wajib cantumkan test ini di output. User akan konfirmasi satu per satu:

- [ ] Layar menyala, tidak blank putih, tidak blank hitam permanen
- [ ] Background bisa di-fill warna solid (merah / hijau / biru)
- [ ] Teks `"DISPLAY OK"` muncul di tengah layar
- [ ] Backlight fade in dari 0 ke full dalam ~500ms — terlihat smooth
- [ ] `display_hal_backlight_set(0)` matiin backlight, set 255 nyalain lagi
- [ ] Tidak ada crash / reboot loop setelah init

### 6.4 Debug Output yang Diharapkan

```
[DISPLAY] Init started...          // [DEBUG]
[DISPLAY] TFT initialized OK       // [DEBUG]
[DISPLAY] LEDC channel 0 ready     // [DEBUG]
[DISPLAY] Fade in complete         // [DEBUG]
```

### 6.5 Feedback Template (Diisi Agent Setelah Selesai)

Agent wajib output section ini setelah menulis code:

```
=== DISPLAY HAL FEEDBACK ===
Status: DONE / BLOCKED

Files produced:
- lib/drivers/display_hal.h
- lib/drivers/display_hal.cpp

Interface exported:
- [list semua fungsi yang diimplementasi]

Assumptions made:
- [list asumsi yang dibuat agent]

Known limitations:
- [list hal yang belum dihandle]

Ready for user test: YES / NO
Blocker (if NO): [penjelasan]
=============================
```

---

## 7. Step 2 — Button HAL `[PENDING]`

> Section ini akan didetailkan setelah Step 1 `[VERIFIED ✓]`.

### 7.1 Ringkasan

- BTN_L (GPIO 7), BTN_R (GPIO 5), active LOW, `INPUT_PULLUP`
- Single click L: Geser menu ke kiri (Prev)
- Single click R: Geser menu ke kanan (Next)
- Long hold R: Select / Execute action (≥ 800ms)
- Release guard: event di-fire setelah tombol dilepas

---

## 8. Step 3 — Sensor HAL `[PENDING]`

> Section ini akan didetailkan setelah Step 2 `[VERIFIED ✓]`.

### 8.1 MAX30100 — Prinsip Utama

- **RULE-007 berlaku penuh:** `pox.begin()` tidak boleh ada di `setup()`
- `Wire.begin(8, 9)` sudah dipanggil di `main.cpp` — sensor tidak perlu memanggil lagi (RULE-008)
- `pox.update()` wajib dipanggil tiap loop selama state `EXEC_HR` aktif

### 8.2 Verified Working Pattern (dari hardware test)

Pattern ini sudah dikonfirmasi works di hardware ini:

```cpp
// Di setup() — HANYA ini, tidak ada pox.begin()
Wire.begin(8, 9);

// Di max30100_hal_init() — dipanggil HANYA saat masuk EXEC_HR
bool max30100_hal_init() {
    if (!pox.begin()) return false;
    pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
    pox.setOnBeatDetectedCallback(onBeatDetected);
    return true;
}

void max30100_hal_update() {
    pox.update(); // Dipanggil tiap loop, tanpa delay
}

void max30100_hal_shutdown() {
    pox.shutdown();
}
```

### 8.3 HR Display Policy

- Hasil HR ditampilkan **live di layar selama mode HR aktif** (update tiap ~1 detik)
- Serial debug log aktif selama development (RULE-009)
- Timeout mode HR: 15 detik → kembali ke watchface otomatis

### 8.4 BMI160 — RESERVED `[RESERVED]`

- I2C address: `0x68`
- Fitur yang direncanakan (future update): step counter, fall detection
- **Yang harus dilakukan agent:** buat stub file saja — `bmi160_hal_init()` dan `bmi160_hal_shutdown()` return `false` dan print `"[BMI160] RESERVED"` // [DEBUG]
- **Jangan implementasi** fitur apapun sampai ada instruksi eksplisit

---

## 9. Step 4 — UI & State Machine `[VERIFIED ✓]`

> Section ini akan didetailkan setelah Step 3 `[VERIFIED ✓]`.

### 9.1 State Machine Overview

```
[ WATCHFACE ] <─── BTN_L / BTN_R ───> [ MENU_HR ] <─── BTN_L / BTN_R ───> [ MENU_AOD ] 
      │                                   │                                   │
      │                                BTN_R_HOLD                          BTN_R_HOLD
      │                                   ▼                                   ▼
[ DEEP_SLEEP ] <─── BTN_R_HOLD ─── [ MENU_SLEEP ]                     [ TOGGLE AOD_STATUS ]
```

### 9.2 Watchface Elements

Elemen yang ditampilkan di watchface:

| Elemen | Posisi | Catatan |
| :--- | :--- | :--- |
| Jam HH:MM | Center, besar | Font utama |
| Tanggal | Bawah jam | Format: DD/MM/YYYY |
| Battery % | Pojok kanan atas | |
| Charging icon | Sebelah battery % | Tampil hanya saat `CHRG_PIN = LOW` |

### 9.3 Asset Management `[SPEC]`

> RULE-011 berlaku penuh di section ini.

#### Cara Kerja

UI Manager Agent **tidak boleh** menulis array pixel langsung. Alurnya:

1. Lu convert gambar ke RGB565 array pakai tool eksternal (misalnya [image2cpp](https://javl.github.io/image2cpp/) atau script Python)
2. Hasil array lu paste ke `assets_wallpaper.cpp` / `assets_icons.cpp`
3. UI Manager tinggal `#include` dan panggil getter — dia tidak perlu tau isi arraynya

#### Format File Asset

**`assets_wallpaper.h`** — hanya expose getter, tidak ada array di sini:
```cpp
#pragma once
#include <pgmspace.h>
#include <stdint.h>

// Watchface background — 240x280, RGB565, PROGMEM
const uint16_t* assets_get_wallpaper();
uint16_t assets_get_wallpaper_width();
uint16_t assets_get_wallpaper_height();
```

**`assets_wallpaper.cpp`** — array ada di sini, dalam PROGMEM:
```cpp
#include "assets_wallpaper.h"

static const uint16_t WALLPAPER_DATA[] PROGMEM = {
    // Hasil convert RGB565 — paste array di sini
    0x0000, 0x0000, // ... dst
};

const uint16_t* assets_get_wallpaper()        { return WALLPAPER_DATA; }
uint16_t        assets_get_wallpaper_width()  { return 240; }
uint16_t        assets_get_wallpaper_height() { return 280; }
```

**`assets_icons.h`** — getter per ikon:
```cpp
#pragma once
#include <pgmspace.h>
#include <stdint.h>

// Charging bolt icon — 16x16, RGB565, PROGMEM
const uint16_t* assets_get_icon_charging();

// Battery icon — 16x16, RGB565, PROGMEM
const uint16_t* assets_get_icon_battery();

uint16_t assets_get_icon_width();   // semua ikon 16x16
uint16_t assets_get_icon_height();
```

#### Cara UI Manager Pakai Asset

```cpp
// Di ui_manager.cpp — render wallpaper
TFT_eSPI& tft = display_hal_get_tft();
tft.pushImage(0, 0,
    assets_get_wallpaper_width(),
    assets_get_wallpaper_height(),
    assets_get_wallpaper()
);

// Render charging icon di pojok kanan atas
tft.pushImage(220, 4,
    assets_get_icon_width(),
    assets_get_icon_height(),
    assets_get_icon_charging()
);
```

#### Placeholder Policy

Saat Step 4 dikerjakan, kalau lu belum punya file gambar yang sudah di-convert:
- Agent bikin `assets_wallpaper.cpp` dengan array placeholder `{0x0000}` (satu pixel hitam)
- Agent bikin `assets_icons.cpp` dengan array placeholder `{0xFFFF}` (satu pixel putih)
- Dimensi getter tetap return nilai asli (240×280 dan 16×16)
- Begitu lu siap dengan array asli, tinggal ganti isi array — **tidak ada file lain yang perlu diubah**

#### Konversi RGB565 (Referensi)

Tool yang bisa dipakai:
- **Online:** [javl.github.io/image2cpp](https://javl.github.io/image2cpp/) — pilih format "Arduino", color format "RGB565", little-endian
- **Python:** `pip install pillow`, konversi manual pixel per pixel
- Output harus berupa array `uint16_t` dalam format little-endian RGB565

### 9.4 AOD Policy
- Saat AOD aktif: background hitam, jam putih, brightness 10%, CPU 40MHz
- AOD nyala otomatis saat charging
- AOD bisa di-toggle dari `MENU_AOD`
- AOD state disimpan — persistent across wake cycles (RTC memory)

---

## 10. Step 5 — Power & Sleep `[VERIFIED ✓]`

> Section ini akan didetailkan setelah## 10. Step 5 — Power & Sleep `[CURRENT]`

### 10.1 Pre-Sleep Checklist (Urutan Wajib)

1. Spin-wait sampai GPIO 5 = HIGH (release guard) — RULE-006
2. max30100_hal_shutdown() — RULE-004
3. bmi160_hal_shutdown() — RULE-004
4. display_hal_backlight_fade_out() — RULE-005
5. digitalWrite(10, LOW) — RULE-005
6. ledcDetach(10) — RULE-005
7. esp_deep_sleep_enable_gpio_wakeup(1ULL << 5, ESP_GPIO_WAKEUP_GPIO_LOW) — RULE-006
8. esp_deep_sleep_start() — RULE-001

### 10.2 DFS Frequency Table

| Kondisi | CPU Freq |
| :--- | :--- |
| Boot / Rendering | 160 MHz |
| Display on, idle | 80 MHz |
| AOD aktif | 40 MHz |
| Deep sleep | — |

### 10.3 Battery Thresholds

| Level | Voltage | Aksi |
| :--- | :--- | :--- |
| Warning | ≤ 3.50V | Nonaktifkan AOD, CPU → 40MHz, brightness max 50%, tampilkan ikon |
| Critical | ≤ 3.40V | Tampilkan "LOW BATTERY", matikan sensor, masuk deep sleep |

---

## 11. Fault Handling `[SPEC]`

- **I2C tidak respond:** retry maks 3×, delay 50ms antar retry. Jika gagal, disable fitur sensor, kembali ke watchface
- **I2C bus lock (SDA stuck LOW):** toggle GPIO 8 sebagai output, generate 9 clock pulse di GPIO 9, reconfigure sebagai I2C, retry
- **ADC nilai aneh:** abaikan, pakai nilai valid terakhir
- **Crash > 3× dalam 1 menit:** masuk safe mode — sensor off, display watchface only
- **Watchdog:** aktif di production build

---

## 12. Memory Policy `[SPEC]`

- `malloc` / `new` dilarang di loop utama
- Semua buffer: static allocation
- Asset gambar: `PROGMEM`
- RAM target: maks 70% SRAM
- Flash target: maks 80%

---

## Appendix A — config_pins.h Template

```cpp
#pragma once

// SPI — TFT
#define PIN_TFT_MOSI  6
#define PIN_TFT_SCK   4
#define PIN_TFT_DC    2
#define PIN_TFT_RST   1
#define PIN_TFT_CS    -1
#define PIN_TFT_BL    10

// I2C — Shared bus
#define PIN_SDA       8
#define PIN_SCL       9

// Input
#define PIN_BTN       7

// Power
#define PIN_BATT_ADC  3
#define PIN_CHRG      0
```

## Appendix B — app_config.h Template

```cpp
#pragma once

// Timeouts
#define AUTO_SLEEP_MS       30000   // 30 detik
#define HR_MEASURE_MS       15000   // 15 detik
#define DEBOUNCE_MS         20
#define SHORT_CLICK_MAX_MS  400
#define LONG_HOLD_MIN_MS    1500

// Battery
#define BATT_WARN_V         3.50f
#define BATT_CRIT_V         3.40f

// DFS
#define FREQ_HIGH           160
#define FREQ_NORMAL         80
#define FREQ_LOW            40

// Backlight
#define BL_FULL             255
#define BL_AOD              25      // ~10%
#define BL_LOW_BATT_MAX     127     // ~50%
#define BL_FADE_MS          400
```

---

*FSD v2.1 — Living Document. Diupdate setiap step verified di hardware.*
*Step selesai → section Locked Interfaces diisi → status diubah → lanjut step berikutnya.*

---
*-- END OF DOCUMENT --*