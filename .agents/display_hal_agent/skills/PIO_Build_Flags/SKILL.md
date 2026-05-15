---
name: PlatformIO Build Flags
description: Paham TFT_eSPI config datang dari build_flags di platformio.ini.
---

# PlatformIO Build Flags

## Configuration Method
- **No Edit Lib**: Tidak pernah meminta user untuk mengedit file di dalam folder `.pio/libdeps`.
- **build_flags**: Memasukkan parameter konfigurasi `TFT_eSPI` (seperti `USER_SETUP_LOADED`, `ST7789_DRIVER`, `TFT_WIDTH`, `TFT_HEIGHT`, dll.) langsung ke dalam file `platformio.ini`.
- **Flexibility**: Memberikan fleksibilitas untuk mengubah konfigurasi per-environment tanpa merusak library global.

## Example Pattern
```ini
build_flags =
  -D USER_SETUP_LOADED=1
  -D ST7789_DRIVER=1
  -D TFT_WIDTH=240
  -D TFT_HEIGHT=280
  -D TFT_MOSI=6
  -D TFT_SCLK=4
  -D TFT_CS=-1
  -D TFT_DC=2
  -D TFT_RST=1
  -D LOAD_GLCD=1
  -D SPI_FREQUENCY=27000000
```
