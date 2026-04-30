# BMI160 — Firmware Reference Cheatsheet
> Bare-metal ESP32-C3 | Bosch Sensortec BST-BMI160-DS000-09

---

## Ringkasan Singkat

BMI160 adalah IMU 6-axis (3-axis akselerometer + 3-axis giroskop) 16-bit dengan FIFO 1 KB, dua pin interrupt, dan antarmuka SPI/I2C.  
Semua sensor **default suspend** setelah power-up — kamu harus eksplisit aktifkan via register `CMD (0x7E)`.

---

## 1. Identifikasi Chip

| Register | Addr | Mode | Nilai Default | Keterangan |
|----------|------|------|---------------|------------|
| `CHIP_ID` | `0x00` | R | `0xD1` | Selalu cek ini dulu setelah init |

```c
// Verifikasi chip
uint8_t id = bmi160_read_reg(0x00);
assert(id == 0xD1); // kalau bukan, ada masalah wiring/SPI
```

---

## 2. Power Mode (PMU)

### Cara ubah power mode → tulis ke `CMD (0x7E)`

| Perintah | Opcode | Keterangan |
|----------|--------|------------|
| Accel → Normal | `0x11` | Aktif penuh, ~3.2ms startup |
| Accel → Low Power | `0x12` | Duty-cycling, hemat daya |
| Accel → Suspend | `0x10` | Mati, tidak ada sampling |
| Gyro → Normal | `0x15` | Aktif penuh, ~55ms startup dari suspend |
| Gyro → Fast Start-Up | `0x17` | Standby cepat, transisi ke normal ≤10ms |
| Gyro → Suspend | `0x14` | Mati |
| Mag → Normal | `0x19` | Aktifkan antarmuka magnetometer |
| Mag → Suspend | `0x18` | Matikan antarmuka magnetometer |

```c
// Contoh: aktifkan accel + gyro normal mode
bmi160_write_reg(0x7E, 0x11); // accel normal
delay_ms(4);                  // tunggu startup
bmi160_write_reg(0x7E, 0x15); // gyro normal
delay_ms(80);                 // gyro butuh ~55ms (max 80ms)
```

### Status Power Mode Saat Ini

| Register | Addr | Mode |
|----------|------|------|
| `PMU_STATUS` | `0x03` | R |

| Bit | Field | Nilai |
|-----|-------|-------|
| `[5:4]` | `acc_pmu_status` | `00`=Suspend, `01`=Normal, `10`=Low Power |
| `[3:2]` | `gyr_pmu_status` | `00`=Suspend, `01`=Normal, `11`=Fast Start-Up |
| `[1:0]` | `mag_pmu_status` | `00`=Suspend, `01`=Normal, `10`=Low Power |

### Konsumsi Daya (Tipikal)

| Mode Accel | Mode Gyro | Arus (µA) |
|------------|-----------|-----------|
| Suspend | Suspend | 3 |
| Normal | Suspend | 180 |
| Suspend | Normal | 850 |
| Normal | Normal | **925** |
| Suspend | Fast Start-Up | 500 |

---

## 3. Konfigurasi Akselerometer

### `ACC_CONF (0x40)` — ODR, Bandwidth, Undersampling

| Bit | Field | Keterangan |
|-----|-------|------------|
| `[7]` | `acc_us` | `0`=Normal, `1`=Undersampling (low power) |
| `[6:4]` | `acc_bwp` | Bandwidth (filter cutoff) |
| `[3:0]` | `acc_odr` | Output Data Rate |

#### Pilihan ODR (`acc_odr`)

| Nilai | ODR (Hz) |
|-------|----------|
| `0x01` | 25/32 ≈ 0.78 |
| `0x02` | 25/16 ≈ 1.56 |
| `0x03` | 25/8 ≈ 3.125 |
| `0x04` | 25/4 ≈ 6.25 |
| `0x05` | 25/2 = 12.5 |
| `0x06` | 25 |
| `0x07` | 50 |
| `0x08` | **100** ← default |
| `0x09` | 200 |
| `0x0A` | 400 |
| `0x0B` | 800 |
| `0x0C` | **1600** |

#### Pilihan Bandwidth (`acc_bwp`, saat `acc_us=0`)

| Nilai | Mode | -3dB Cutoff |
|-------|------|-------------|
| `0x00` | OSR4 Normal | ODR/8 |
| `0x01` | OSR2 Normal | ODR/4 |
| `0x02` | Normal | **ODR/2** ← default (0x28 = ODR 100Hz, BWP Normal) |
| `0x03`–`0x07` | CIC/Reserved | — |

> **Default register**: `ACC_CONF = 0x28` → ODR 100Hz, BWP Normal, acc_us=0

### `ACC_RANGE (0x41)` — Rentang G

| Nilai | Rentang | Resolusi |
|-------|---------|----------|
| `0x03` | ±2g | 0.061 mg/LSB |
| `0x05` | ±4g | 0.122 mg/LSB |
| `0x08` | ±8g | 0.244 mg/LSB |
| `0x0C` | ±16g | 0.488 mg/LSB |

> Semua nilai lain → fallback ke ±2g

---

## 4. Konfigurasi Giroskop

### `GYR_CONF (0x42)` — ODR & Bandwidth

| Bit | Field | Keterangan |
|-----|-------|------------|
| `[5:4]` | `gyr_bwp` | Bandwidth coefficient |
| `[3:0]` | `gyr_odr` | Output Data Rate |

#### Pilihan ODR (`gyr_odr`)

| Nilai | ODR (Hz) |
|-------|----------|
| `0x06` | 25 |
| `0x07` | 50 |
| `0x08` | **100** |
| `0x09` | 200 |
| `0x0A` | 400 |
| `0x0B` | 800 |
| `0x0C` | 1600 |
| `0x0D` | 3200 |

> **Default**: `GYR_CONF = 0x28` → ODR 100Hz

### `GYR_RANGE (0x43)` — Rentang °/s

| Nilai | Rentang | Resolusi |
|-------|---------|----------|
| `0x00` | **±2000°/s** | 16.4 LSB/°/s (61.0 m°/s/LSB) |
| `0x01` | ±1000°/s | 32.8 LSB/°/s |
| `0x02` | ±500°/s | 65.6 LSB/°/s |
| `0x03` | ±250°/s | 131.2 LSB/°/s |
| `0x04` | ±125°/s | 262.4 LSB/°/s |

---

## 5. Pembacaan Data Sensor

### Register Data (`0x04–0x17`) — 20 byte, read-only

| Addr | Konten |
|------|--------|
| `0x04–0x05` | MAG_X (LSB, MSB) |
| `0x06–0x07` | MAG_Y |
| `0x08–0x09` | MAG_Z |
| `0x0A–0x0B` | RHALL (kompensasi magnetometer) |
| `0x0C–0x0D` | GYR_X (LSB, MSB) |
| `0x0E–0x0F` | GYR_Y |
| `0x10–0x11` | GYR_Z |
| `0x12–0x13` | ACC_X (LSB, MSB) |
| `0x14–0x15` | ACC_Y |
| `0x16–0x17` | ACC_Z |

> Format: **16-bit two's complement**, little-endian (LSB dulu).

```c
// Burst read accel saja (0x12–0x17 = 6 byte)
uint8_t buf[6];
bmi160_burst_read(0x12, buf, 6);

int16_t ax = (int16_t)(buf[1] << 8 | buf[0]);
int16_t ay = (int16_t)(buf[3] << 8 | buf[2]);
int16_t az = (int16_t)(buf[5] << 8 | buf[4]);

// Konversi ke g (range ±8g, resolusi 0.244 mg/LSB)
float ax_g = ax * 0.000244f;
```

### Sensor Time (`0x18–0x1A`) — 24-bit counter, 39 µs/tick

```c
uint8_t t[3];
bmi160_burst_read(0x18, t, 3);
uint32_t sensortime = (t[2] << 16) | (t[1] << 8) | t[0];
float time_ms = sensortime * 0.039f;
```

### Temperature (`0x20–0x21`)

| Nilai | Suhu |
|-------|------|
| `0x7FFF` | 87°C |
| `0x0000` | 23°C |
| `0x8001` | -41°C |
| `0x8000` | Invalid |

> Resolusi: 1/512 K/LSB. Valid hanya saat gyro dalam normal mode.

---

## 6. Status Register

### `STATUS (0x1B)` — Data Ready & Status

| Bit | Field | Keterangan |
|-----|-------|------------|
| `[7]` | `drdy_acc` | Accel data baru tersedia |
| `[6]` | `drdy_gyr` | Gyro data baru tersedia |
| `[5]` | `drdy_mag` | Mag data baru tersedia |
| `[4]` | `nvm_rdy` | NVM siap (`1`=ready) |
| `[3]` | `foc_rdy` | Fast Offset Compensation selesai |
| `[1]` | `gyr_self_test_ok` | Self-test gyro lulus |

### `ERR_REG (0x02)` — Error Flags

| Bit | Field | Keterangan |
|-----|-------|------------|
| `[7]` | `drop_cmd_err` | CMD register penuh, perintah di-drop |
| `[4:1]` | `err_code` | Kode error (0=ok, lihat datasheet) |
| `[0]` | `fatal_err` | Chip error parah (butuh POR untuk clear) |

> Flags reset otomatis saat dibaca (kecuali `fatal_err`).

---

## 7. FIFO

### Konfigurasi FIFO

#### `FIFO_CONFIG[0] (0x46)` — Watermark Level

| Bit | Field | Keterangan |
|-----|-------|------------|
| `[7:0]` | `fifo_water_mark` | Level watermark (×4 byte). Interrupt saat terlampaui. |

#### `FIFO_CONFIG[1] (0x47)` — Enable & Mode

| Bit | Field | Keterangan |
|-----|-------|------------|
| `[7]` | `fifo_gyr_en` | `1` = simpan data gyro ke FIFO |
| `[6]` | `fifo_acc_en` | `1` = simpan data accel ke FIFO |
| `[5]` | `fifo_mag_en` | `1` = simpan data mag ke FIFO |
| `[4]` | `fifo_header_en` | `1` = header mode, `0` = headerless |
| `[3]` | `fifo_tag_int1_en` | Tag frame saat INT1 aktif |
| `[2]` | `fifo_tag_int2_en` | Tag frame saat INT2 aktif |
| `[1]` | `fifo_time_en` | Sertakan sensortime frame di akhir FIFO |

#### `FIFO_DOWNS (0x45)` — Downsampling

| Bit | Field | Keterangan |
|-----|-------|------------|
| `[7]` | `acc_fifo_filt_data` | `0`=pre-filtered, `1`=filtered |
| `[6:4]` | `acc_fifo_downs` | Ratio: 2^val |
| `[3]` | `gyr_fifo_filt_data` | `0`=pre-filtered, `1`=filtered |
| `[2:0]` | `gyr_fifo_downs` | Ratio: 2^val |

### Membaca FIFO

| Register | Addr | Keterangan |
|----------|------|------------|
| `FIFO_LENGTH[0:1]` | `0x22–0x23` | Jumlah byte valid di FIFO (max 1024) |
| `FIFO_DATA` | `0x24` | Burst read dari sini (alamat tidak auto-increment) |

```c
// Baca FIFO
uint16_t len;
bmi160_burst_read(0x22, (uint8_t*)&len, 2);
len &= 0x07FF; // 11-bit

uint8_t fifo_buf[1024];
bmi160_burst_read(0x24, fifo_buf, len);
// parse frames setelah ini
```

> Magic number `0x80` di header = frame tidak valid / FIFO habis.

---

## 8. Interrupt Engine

### Enable Interrupt: `INT_EN[0–2] (0x50–0x52)`

#### `INT_EN[0] (0x50)`

| Bit | Field | Trigger |
|-----|-------|---------|
| `[7]` | `int_flat_en` | Posisi flat |
| `[6]` | `int_orient_en` | Orientasi berubah |
| `[5]` | `int_s_tap_en` | Single tap |
| `[4]` | `int_d_tap_en` | Double tap |
| `[2]` | `int_anymo_z_en` | Any-motion Z |
| `[1]` | `int_anymo_y_en` | Any-motion Y |
| `[0]` | `int_anymo_x_en` | Any-motion X |

#### `INT_EN[1] (0x51)`

| Bit | Field | Trigger |
|-----|-------|---------|
| `[6]` | `int_fwm_en` | FIFO watermark |
| `[5]` | `int_ffull_en` | FIFO full |
| `[4]` | `int_drdy_en` | Data ready |
| `[3]` | `int_low_en` | Low-g |
| `[2]` | `int_highz_en` | High-g Z |
| `[1]` | `int_highy_en` | High-g Y |
| `[0]` | `int_highx_en` | High-g X |

#### `INT_EN[2] (0x52)`

| Bit | Field | Trigger |
|-----|-------|---------|
| `[3]` | `int_step_det_en` | Step detector |
| `[2]` | `int_nomoz_en` | No-motion Z |
| `[1]` | `int_nomoy_en` | No-motion Y |
| `[0]` | `int_nomox_en` | No-motion X |

### Konfigurasi Pin Interrupt: `INT_OUT_CTRL (0x53)`

| Bit | Field | Keterangan |
|-----|-------|------------|
| `[7]` | `int2_output_en` | Enable output INT2 |
| `[6]` | `int2_od` | `0`=Push-pull, `1`=Open-drain |
| `[5]` | `int2_lvl` | `0`=Active low, `1`=Active high |
| `[4]` | `int2_edge_ctrl` | `0`=Level, `1`=Edge triggered |
| `[3]` | `int1_output_en` | Enable output INT1 |
| `[2]` | `int1_od` | `0`=Push-pull, `1`=Open-drain |
| `[1]` | `int1_lvl` | `0`=Active low, `1`=Active high |
| `[0]` | `int1_edge_ctrl` | `0`=Level, `1`=Edge triggered |

### Latch Mode: `INT_LATCH (0x54)`

| Bit | Field | Keterangan |
|-----|-------|------------|
| `[5]` | `int2_input_en` | Enable input INT2 |
| `[4]` | `int1_input_en` | Enable input INT1 |
| `[3:0]` | `int_latch` | Mode latch interrupt |

| `int_latch` | Mode |
|-------------|------|
| `0x0` | Non-latched |
| `0x1`–`0xE` | Temporal (312.5µs – 2.56s) |
| `0xF` | **Latched** (tetap sampai dibaca) |

### Mapping Interrupt ke Pin: `INT_MAP[0–2] (0x55–0x57)`

| Interrupt | INT_MAP[0] bit → INT1 | INT_MAP[2] bit → INT2 |
|-----------|----------------------|-----------------------|
| Flat | `[7]` | `[7]` |
| Orientation | `[6]` | `[6]` |
| Single Tap | `[5]` | `[5]` |
| Double Tap | `[4]` | `[4]` |
| No-motion | `[3]` | `[3]` |
| Any-motion | `[2]` | `[2]` |
| High-g | `[1]` | `[1]` |
| Low-g / Step | `[0]` | `[0]` |

| Interrupt | INT_MAP[1] `[7:4]` → INT1 | INT_MAP[1] `[3:0]` → INT2 |
|-----------|--------------------------|--------------------------|
| Data Ready | `[7]` | `[3]` |
| FIFO Watermark | `[6]` | `[2]` |
| FIFO Full | `[5]` | `[1]` |
| PMU Trigger | `[4]` | `[0]` |

---

## 9. Threshold Interrupt

### High-G & Low-G: `INT_LOWHIGH (0x5A–0x5E)`

| Sub-reg | Field | Keterangan |
|---------|-------|------------|
| `[0]` | `int_low_dur` | Delay low-g: `(val+1) × 2.5ms`, range 2.5–640ms |
| `[1]` | `int_low_th` | Threshold low-g: `val × 7.81mg` |
| `[2]` | `int_low_hy` / `int_high_hy` | Histeresis |
| `[3]` | `int_high_dur` | Delay high-g: `(val+1) × 2.5ms` |
| `[4]` | `int_high_th` | Threshold high-g: `val × 7.81mg` (range ±2g) |

### Any-Motion & No-Motion: `INT_MOTION (0x5F–0x62)`

| Sub-reg | Field | Keterangan |
|---------|-------|------------|
| `[0]` | `int_anym_dur` | Jumlah sampel konsekutif di atas threshold |
| `[0]` | `int_slo_no_mot_dur` | Durasi no-motion (hingga 430s) |
| `[1]` | `int_anym_th` | Threshold any-motion: `val × 3.91mg` (±2g) |
| `[2]` | `int_slo_no_mot_th` | Threshold no-motion |
| `[3]` | `int_no_mot_sel` | `1`=no-motion, `0`=slow-motion |
| `[3]` | `int_sig_mot_sel` | `1`=significant motion, `0`=anymotion |

### Tap: `INT_TAP (0x63–0x64)`

| Field | Keterangan |
|-------|------------|
| `int_tap_quiet` | `0`=30ms, `1`=20ms quiet time |
| `int_tap_shock` | `0`=50ms, `1`=75ms shock time |
| `int_tap_dur` | Window double-tap: 50–700ms |
| `int_tap_th` | Threshold tap: `val × 62.5mg` (±2g) |

---

## 10. Offset Compensation (Kalibrasi)

### Fast Offset Compensation (FOC)

1. Posisikan sensor di orientasi yang diketahui (misal flat, Z menghadap atas = +1g)
2. Set target di `FOC_CONF (0x69)`:

| Bit | Field | Keterangan |
|-----|-------|------------|
| `[6]` | `foc_gyr_en` | Aktifkan FOC gyro (target 0 dps) |
| `[5:4]` | `foc_acc_x` | Target X: `00`=off, `01`=+1g, `10`=-1g, `11`=0g |
| `[3:2]` | `foc_acc_y` | Target Y |
| `[1:0]` | `foc_acc_z` | Target Z |

3. Trigger: `bmi160_write_reg(0x7E, 0x03)` → `start_foc`
4. Tunggu `foc_rdy` bit `[3]` di `STATUS (0x1B)` = `1` (max 250ms)

```c
// Contoh: FOC untuk sensor flat (Z=+1g)
bmi160_write_reg(0x69, 0b01110011); // gyr_en + acc_z=+1g, xy=0g
bmi160_write_reg(0x7E, 0x03);       // trigger FOC
while (!(bmi160_read_reg(0x1B) & (1<<3))); // tunggu foc_rdy

// Enable offset di register OFFSET[6] (0x77)
uint8_t off6 = bmi160_read_reg(0x77);
bmi160_write_reg(0x77, off6 | 0xC0); // set acc_off_en + gyr_off_en
```

### Manual Offset: `OFFSET (0x71–0x77)`

| Addr | Field | Resolusi |
|------|-------|----------|
| `0x71` | `off_acc_x` | 3.9 mg/LSB (8-bit signed) |
| `0x72` | `off_acc_y` | 3.9 mg/LSB |
| `0x73` | `off_acc_z` | 3.9 mg/LSB |
| `0x74` | `off_gyr_x[7:0]` | 0.061 °/s/LSB (10-bit signed) |
| `0x75` | `off_gyr_y[7:0]` | 0.061 °/s/LSB |
| `0x76` | `off_gyr_z[7:0]` | 0.061 °/s/LSB |
| `0x77` | MSB gyro offset + enable flags | |

---

## 11. Pedometer / Step Counter

### Konfigurasi: `STEP_CONF (0x7A–0x7B)`

| Preset Mode | `STEP_CONF[0]` | `STEP_CONF[1]` | Keterangan |
|-------------|----------------|----------------|------------|
| Normal | `0x15` | `0x03` | Balance false-positive & false-negative |
| Sensitive | `0x2D` | `0x00` | Sedikit false-negative (untuk orang kurus) |
| Robust | `0x1D` | `0x07` | Sedikit false-positive |

```c
// Setup pedometer normal mode + enable counter
bmi160_write_reg(0x7A, 0x15);
bmi160_write_reg(0x7B, 0x03 | (1<<3)); // bit3 = step_cnt_en
```

### Baca Step Count: `STEP_CNT (0x78–0x79)` — 16-bit, read-only

```c
uint8_t s[2];
bmi160_burst_read(0x78, s, 2);
uint16_t steps = (s[1] << 8) | s[0];
```

---

## 12. CMD Register — Perintah Penting

| Opcode | Nama | Keterangan |
|--------|------|------------|
| `0x03` | `start_foc` | Mulai Fast Offset Compensation |
| `0x10` | `acc_suspend` | Accel → Suspend |
| `0x11` | `acc_normal` | Accel → Normal |
| `0x12` | `acc_lowpower` | Accel → Low Power |
| `0x14` | `gyr_suspend` | Gyro → Suspend |
| `0x15` | `gyr_normal` | Gyro → Normal |
| `0x17` | `gyr_faststartup` | Gyro → Fast Start-Up |
| `0x18` | `mag_suspend` | Mag interface → Suspend |
| `0x19` | `mag_normal` | Mag interface → Normal |
| `0xA0` | `prog_nvm` | Tulis offset ke NVM (max 14x!) |
| `0xB0` | `fifo_flush` | Kosongkan FIFO (config tetap) |
| `0xB1` | `int_reset` | Reset semua interrupt |
| `0xB2` | `step_cnt_clr` | Reset step counter |
| `0xB6` | `softreset` | Full reset + reboot |

---

## 13. Interface — SPI (Bare-Metal ESP32-C3)

### Seleksi Protokol
- **Default setelah power-up**: I2C  
- **Switch ke SPI**: berikan 1 rising edge pada CSB, lalu lakukan dummy read ke `0x7F`

```c
// Init SPI: dummy read 0x7F untuk lock-in SPI mode
gpio_set_level(BMI160_CS, 0); // CSB low
spi_transfer(0x7F | 0x80);    // read bit[7]=1, addr=0x7F
spi_transfer(0x00);
gpio_set_level(BMI160_CS, 1); // CSB high
delay_us(10);
```

### Protokol SPI

| Parameter | Nilai |
|-----------|-------|
| Mode | CPOL=0 CPHA=0 **atau** CPOL=1 CPHA=1 (auto-detect) |
| Max Clock | 10 MHz (VDDIO ≥ 1.71V) |
| Frame | 16-bit per byte (8-bit addr + 8-bit data) |
| Bit7 pada byte addr | `0`=Write, `1`=Read |

```c
// Write register
void bmi160_write_reg(uint8_t reg, uint8_t val) {
    gpio_set_level(CS, 0);
    spi_transfer(reg & 0x7F); // bit7=0 → write
    spi_transfer(val);
    gpio_set_level(CS, 1);
    delay_us(2);
}

// Read register  
uint8_t bmi160_read_reg(uint8_t reg) {
    gpio_set_level(CS, 0);
    spi_transfer(reg | 0x80); // bit7=1 → read
    uint8_t val = spi_transfer(0x00);
    gpio_set_level(CS, 1);
    return val;
}

// Burst read (alamat auto-increment)
void bmi160_burst_read(uint8_t reg, uint8_t *buf, uint16_t len) {
    gpio_set_level(CS, 0);
    spi_transfer(reg | 0x80);
    for (uint16_t i = 0; i < len; i++)
        buf[i] = spi_transfer(0x00);
    gpio_set_level(CS, 1);
}
```

### I2C Address (jika pakai I2C)

| SDO Pin | I2C Address |
|---------|-------------|
| GND | `0x68` |
| VDDIO | `0x69` |

---

## 14. NVM — Non-Volatile Memory

> **PERHATIAN**: NVM hanya bisa ditulis **maksimal 14 kali** seumur chip!

Gunakan untuk menyimpan offset kalibrasi secara permanen:

```c
// Prosedur tulis NVM
// 1. Pastikan accel dalam normal mode
// 2. Unlock NVM
bmi160_write_reg(0x6A, 0x02); // CONF: set nvm_prog_en

// 3. Trigger write
bmi160_write_reg(0x7E, 0xA0); // prog_nvm

// 4. Tunggu nvm_rdy (bit[4] STATUS = 1)
while (!(bmi160_read_reg(0x1B) & (1<<4)));
```

---

## 15. Urutan Inisialisasi Minimal (Bare-Metal)

```c
void bmi160_init(void) {
    // 1. Dummy read untuk aktifkan SPI mode
    bmi160_read_reg(0x7F);
    delay_ms(1);

    // 2. Soft reset
    bmi160_write_reg(0x7E, 0xB6);
    delay_ms(10); // tunggu reboot

    // 3. Verifikasi chip ID
    uint8_t id = bmi160_read_reg(0x00);
    if (id != 0xD1) { /* handle error */ }

    // 4. Set akselerometer: ODR 100Hz, range ±8g, Normal mode
    bmi160_write_reg(0x40, 0x28); // ACC_CONF: ODR 100Hz, BWP normal
    bmi160_write_reg(0x41, 0x08); // ACC_RANGE: ±8g
    bmi160_write_reg(0x7E, 0x11); // CMD: accel normal
    delay_ms(5);

    // 5. Set giroskop: ODR 100Hz, range ±2000°/s, Normal mode
    bmi160_write_reg(0x42, 0x28); // GYR_CONF: ODR 100Hz
    bmi160_write_reg(0x43, 0x00); // GYR_RANGE: ±2000°/s
    bmi160_write_reg(0x7E, 0x15); // CMD: gyro normal
    delay_ms(80); // gyro butuh waktu startup lebih lama

    // 6. (Opsional) Enable data ready interrupt di INT1
    bmi160_write_reg(0x50, 0x00); // INT_EN[0]: semua off
    bmi160_write_reg(0x51, 0x10); // INT_EN[1]: drdy_en
    bmi160_write_reg(0x53, 0x0A); // INT_OUT_CTRL: INT1 push-pull, active high
    bmi160_write_reg(0x56, 0x80); // INT_MAP[1]: drdy → INT1
}
```

---

## 16. Ringkasan Register Map Penting

| Addr | Nama | R/W | Fungsi |
|------|------|-----|--------|
| `0x00` | CHIP_ID | R | Identifikasi chip (= 0xD1) |
| `0x02` | ERR_REG | R | Flag error |
| `0x03` | PMU_STATUS | R | Status power mode saat ini |
| `0x04–0x17` | DATA | R | Data 6-axis + mag (20 byte) |
| `0x18–0x1A` | SENSORTIME | R | Counter 24-bit, 39µs/tick |
| `0x1B` | STATUS | R | Data ready, NVM ready, FOC done |
| `0x1C–0x1F` | INT_STATUS | R | Flag interrupt aktif |
| `0x20–0x21` | TEMPERATURE | R | Suhu sensor (gyro harus normal) |
| `0x22–0x23` | FIFO_LENGTH | R | Byte valid di FIFO |
| `0x24` | FIFO_DATA | R | Burst read FIFO |
| `0x40` | ACC_CONF | RW | ODR & bandwidth akselerometer |
| `0x41` | ACC_RANGE | RW | Rentang g akselerometer |
| `0x42` | GYR_CONF | RW | ODR & bandwidth giroskop |
| `0x43` | GYR_RANGE | RW | Rentang °/s giroskop |
| `0x44` | MAG_CONF | RW | ODR magnetometer |
| `0x45` | FIFO_DOWNS | RW | Downsampling FIFO |
| `0x46–0x47` | FIFO_CONFIG | RW | Konfigurasi FIFO |
| `0x4B–0x4F` | MAG_IF | RW | Interface magnetometer eksternal |
| `0x50–0x52` | INT_EN | RW | Enable interrupt |
| `0x53` | INT_OUT_CTRL | RW | Konfigurasi pin interrupt |
| `0x54` | INT_LATCH | RW | Mode latch & input interrupt |
| `0x55–0x57` | INT_MAP | RW | Mapping interrupt ke pin |
| `0x58–0x59` | INT_DATA | RW | Sumber data interrupt |
| `0x5A–0x5E` | INT_LOWHIGH | RW | Threshold high-g / low-g |
| `0x5F–0x62` | INT_MOTION | RW | Konfigurasi anymotion / nomotion |
| `0x63–0x64` | INT_TAP | RW | Konfigurasi tap detection |
| `0x65–0x66` | INT_ORIENT | RW | Konfigurasi orientation detection |
| `0x67–0x68` | INT_FLAT | RW | Konfigurasi flat detection |
| `0x69` | FOC_CONF | RW | Konfigurasi Fast Offset Compensation |
| `0x6A` | CONF | RW | NVM unlock |
| `0x6B` | IF_CONF | RW | Mode interface (SPI 3/4W, secondary) |
| `0x6C` | PMU_TRIGGER | RW | Auto power mode gyro |
| `0x6D` | SELF_TEST | RW | Self-test trigger |
| `0x70` | NV_CONF | RW | SPI enable permanen (NVM-backed) |
| `0x71–0x77` | OFFSET | RW | Nilai offset kalibrasi |
| `0x78–0x79` | STEP_CNT | R | Jumlah langkah |
| `0x7A–0x7B` | STEP_CONF | RW | Sensitivitas pedometer |
| `0x7E` | CMD | W | Command register |

---

*Referensi: Bosch Sensortec BMI160 Datasheet BST-BMI160-DS000-09 Rev 1.0, November 2020*
