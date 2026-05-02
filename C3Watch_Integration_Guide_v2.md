# Firmware Integration Guide: C3Watch Companion App
**Version:** 2.0  
**Changelog:** Added Calibration Protocol (Step Counter + Wake Gesture), Export/Import via JSON, `Send Calibration to Watch` command.

---

## 1. Device Identification
The app filters for devices strictly by their **Advertised Name**.
- **Target Device Name:** `ESP32Watch`
- **BLE Stack Requirement:** Must support GATT Server with stable notifications.

---

## 2. GATT Service & Characteristics Mapping
**Base Service UUID:** `12345678-1234-1234-1234-123456789abc`

| Characteristic | UUID | Properties | Protocol / Format |
| :--- | :--- | :--- | :--- |
| **Time Sync** | `...9001` | Write | 8-byte buffer (see §3.1) |
| **Wallpaper** | `...9002` | Write, Notify | Chunked Binary + ACK (see §3.2) |
| **HR Data** | `...9003` | Notify | 4-byte buffer (see §3.3) |
| **Steps** | `...9004` | Notify | 4-byte buffer (uint32 LE) |
| **Battery** | `...9005` | Notify | 2-byte (percent, charging_flag) |
| **Control** | `...9006` | Write, Notify | Command Byte + Payload |
| **Calibration** ⭐NEW | `...9007` | Write, Notify | Calibration Packet (see §3.5) |

---

## 3. Communication Protocols

### 3.1. Auto Time Sync (App → Watch)
Sent immediately upon connection to characteristic `...9001`.

| Byte | Field | Notes |
|------|-------|-------|
| `[0]` | Year | Offset from 2000 (e.g., 26 = 2026) |
| `[1]` | Month | 1–12 |
| `[2]` | Day | 1–31 |
| `[3]` | Hour | 0–23 |
| `[4]` | Minute | 0–59 |
| `[5]` | Second | 0–59 |
| `[6]` | Weekday | 0=Sun … 6=Sat |
| `[7]` | Checksum | XOR of bytes 0–6 |

**Watch Action:** Update RTC, send `0x01` ACK via Control characteristic.

---

### 3.2. Wallpaper Transfer (App → Watch)
High-speed binary transfer to characteristic `...9002`.

1. **Initiation:** App writes to Control: `[0x01, size_b0, size_b1, size_b2, size_b3]` (uint32 LE total size).  
   → Watch replies `0x01` on Control to confirm ready.

2. **Chunking:** App sends ~263 chunks.

| Bytes | Field |
|-------|-------|
| `[0–1]` | Chunk Index (uint16 LE) |
| `[2–3]` | Total Chunks (uint16 LE) |
| `[4–5]` | Data Length (uint16 LE, max 512) |
| `[6…]` | RGB565 Pixel Data (134,400 bytes total for 240×280) |

   → Watch replies `0x06` ACK or `0x15` NAK per chunk on Wallpaper notify.

3. **Completion:** App writes to Control: `[0x02, crc32_b0, crc32_b1, crc32_b2, crc32_b3]`.  
   → Watch verifies CRC32, replies `0x01` (Success) or `0x02` (Fail) on Control.

---

### 3.3. Sensor Data (Watch → App, Notifications)

**Heart Rate (`...9003`) — 4 Bytes:**

| Byte | Field |
|------|-------|
| `[0]` | BPM (uint8) |
| `[1]` | SpO2 (uint8) |
| `[2–3]` | Minutes since midnight (uint16 LE) |

**Steps (`...9004`) — 4 Bytes:**

| Byte | Field |
|------|-------|
| `[0–3]` | Cumulative Step Count (uint32 LE) |

**Battery (`...9005`) — 2 Bytes:**

| Byte | Field |
|------|-------|
| `[0]` | Percentage (uint8, 0–100) |
| `[1]` | Charging flag (0=No, 1=Yes) |

---

### 3.4. Control Commands (App → Watch, via `...9006`)

| Command | Byte | Payload | Action |
|---------|------|---------|--------|
| Start HR Monitor | `0x03` | — | Watch starts notifying `...9003` continuously |
| Request Steps | `0x04` | — | Watch sends single notify on `...9004` |
| Request Battery | `0x05` | — | Watch sends single notify on `...9005` |
| Reboot | `0x07` | — | Watch restarts immediately |
| Stop HR Monitor | `0x08` | — | Watch stops `...9003` notifications |
| Stop Step Stream | `0x09` | — | Watch stops step stream (if applicable) |
| **Send Calibration** ⭐NEW | `0x0A` | 14-byte calibration packet | Watch applies & saves to NVS (see §3.5) |
| **Request Calibration** ⭐NEW | `0x0B` | — | Watch sends current calibration via `...9007` notify |
| **Reset Calibration** ⭐NEW | `0x0C` | — | Watch resets to factory default calibration |

---

### 3.5. Calibration Protocol ⭐NEW

#### Packet Format (14 Bytes) — bidirectional (`...9007`)

| Byte | Field | Type | Notes |
|------|-------|------|-------|
| `[0]` | Packet Type | uint8 | `0x01`=App→Watch (set), `0x02`=Watch→App (report current) |
| `[1]` | Wake Threshold | uint8 | Any-motion threshold raw value (register format: `val << 2 \| duration_bits`) — see §3.5.1 |
| `[2]` | Wake Duration | uint8 | Any-motion duration samples (0–3 → 1–4 samples) |
| `[3]` | Wake Axis Mask | uint8 | Bitmask: bit0=X, bit1=Y, bit2=Z (e.g., `0x07`=all axes) |
| `[4]` | Screen Timeout | uint8 | Seconds layar menyala setelah wake (1–60s) |
| `[5]` | Step Cal Mode | uint8 | `0x00`=default, `0x01`=manual stride, `0x02`=auto (height/weight) |
| `[6–7]` | Stride Length | uint16 LE | Panjang langkah dalam mm (hanya berlaku jika mode `0x01`) |
| `[8]` | Height cm | uint8 | Tinggi badan user dalam cm (untuk mode `0x02`) |
| `[9]` | Weight kg | uint8 | Berat badan user dalam kg (untuk mode `0x02`) |
| `[10]` | Step Tune Option | uint8 | BMI160 TUNE_OPTION: 1, 2, atau 3 (map ke STEP_CONF preset) |
| `[11]` | Reserved | uint8 | Set `0x00`, reserved untuk fitur berikutnya |
| `[12–13]` | Checksum | uint16 LE | Sum of bytes 0–11, mod 65536 |

#### 3.5.1 Wake Threshold ↔ mg Conversion

```
threshold_mg  = wake_threshold_byte × 3.91
wake_threshold_byte = desired_mg / 3.91

Contoh:
  100mg → byte = 26  (0x1A)
  250mg → byte = 64  (0x40)
  312mg → byte = 80  (0x50)
```

> **Catatan firmware:** Tulis ke register BMI160 sebagai:
> ```c
> writeReg(0x5F, (wake_threshold_byte << 2) | (wake_duration & 0x03));
> ```

#### 3.5.2 Step Calibration Mode

| Mode | Byte | Behaviour |
|------|------|-----------|
| Default | `0x00` | Pakai STEP_CONF preset dari `Step Tune Option` saja, tidak ada koreksi |
| Manual Stride | `0x01` | Jarak = step_count × stride_length_mm. App tampilkan dalam km. |
| Auto (Height/Weight) | `0x02` | Firmware hitung stride estimate: `stride_mm = height_cm × 0.414` (walking), koreksi berdasarkan weight untuk intensitas |

#### 3.5.3 Flow: Send Calibration to Watch

```
App                              Watch
 │                                 │
 │── Control 0x0A ────────────────►│
 │   + 14-byte calibration packet  │
 │                                 │── Validasi checksum
 │                                 │── Tulis ke NVS/EEPROM
 │                                 │── Apply ke BMI160 registers
 │◄── Calibration notify (0x9007) ─│
 │    packet type=0x02 (echo back) │
 │    (konfirmasi nilai tersimpan)  │
```

#### 3.5.4 Flow: Export Calibration dari Watch

```
App                              Watch
 │                                 │
 │── Control 0x0B ────────────────►│  (Request current calibration)
 │                                 │
 │◄── Calibration notify (0x9007) ─│
 │    packet type=0x02             │
 │    (current active values)      │
 │                                 │
 │  App save sebagai .json         │
```

---

## 4. Calibration JSON Export/Import Format

File: `c3watch_calibration_YYYYMMDD.json`

```json
{
  "version": 1,
  "device": "ESP32Watch",
  "exported_at": "2026-05-02T10:30:00",
  "wake_gesture": {
    "threshold_mg": 250,
    "threshold_raw": 64,
    "duration_samples": 3,
    "axis_mask": 7,
    "screen_timeout_sec": 10
  },
  "step_counter": {
    "mode": "auto",
    "stride_length_mm": 720,
    "height_cm": 170,
    "weight_kg": 65,
    "tune_option": 1
  },
  "checksum": 12345
}
```

**Import flow di app:**
1. User pilih file `.json`
2. App validasi `version` dan `device` field
3. App konversi nilai ke 14-byte calibration packet
4. App kirim via `Control 0x0A` ke watch (jika konek), atau simpan lokal untuk dikirim saat konek

---

## 5. Factory Default Calibration Values

Jika watch menerima `Control 0x0C` (Reset Calibration):

| Field | Default Value |
|-------|--------------|
| Wake Threshold | 64 (~250mg) |
| Wake Duration | 3 (4 samples) |
| Wake Axis Mask | 0x07 (X+Y+Z) |
| Screen Timeout | 10 detik |
| Step Cal Mode | 0x00 (default) |
| Stride Length | 700mm |
| Height | 165cm |
| Weight | 60kg |
| Tune Option | 1 |

---

## 6. Implementation Checklist for Firmware Agent

### Existing (v1.0)
- [ ] Set BLE Name to `ESP32Watch`
- [ ] Flash Service UUID `12345678-1234-1234-1234-123456789abc`
- [ ] Implement XOR checksum for Time Sync
- [ ] Implement 134,400-byte buffer for Wallpaper (SPIFFS/LittleFS)
- [ ] Handle 10-second request timeouts

### New (v2.0) ⭐
- [ ] Register Characteristic `...9007` (Write + Notify) untuk Calibration
- [ ] Handle `Control 0x0A` — parse 14-byte calibration packet, validasi checksum uint16
- [ ] Handle `Control 0x0B` — baca NVS, kirim current calibration via `...9007` notify
- [ ] Handle `Control 0x0C` — reset NVS ke factory default, apply ke BMI160
- [ ] Simpan calibration ke **NVS** (bukan RTC_DATA_ATTR) supaya persist hard reset
- [ ] Apply `wake_threshold_raw` + `wake_duration` ke BMI160 register `0x5F` saat init
- [ ] Apply `wake_axis_mask` ke `REG_INT_EN_0` bits [2:0]
- [ ] Apply `screen_timeout_sec` ke display manager
- [ ] Apply `step_tune_option` ke `STEP_CONF_0/1` preset
- [ ] Hitung stride estimate jika `step_cal_mode == 0x02`: `stride_mm = height_cm × 0.414`
- [ ] Echo back calibration packet (type=`0x02`) setelah berhasil disimpan

---

## 7. NVS Key Mapping (ESP-IDF / Arduino Preferences)

```c
// Namespace: "c3watch_cal"
Preferences prefs;
prefs.begin("c3watch_cal", false);

prefs.putUChar("wake_thres",   64);   // threshold raw
prefs.putUChar("wake_dur",      3);   // duration
prefs.putUChar("wake_axis",  0x07);   // axis mask
prefs.putUChar("scr_timeout",  10);   // screen timeout seconds
prefs.putUChar("step_mode",  0x00);   // step cal mode
prefs.putUShort("stride_mm",  700);   // stride length mm
prefs.putUChar("height_cm",   165);   // height
prefs.putUChar("weight_kg",    60);   // weight
prefs.putUChar("tune_opt",      1);   // BMI160 tune option
```
