---
name: ESP32-C3 SPI
description: Paham SPI bus config di ESP32-C3 (RISC-V). Tau kenapa CS di-hardwire ke GND.
---

# ESP32-C3 SPI

## Hardware Specifics
- **ESP32-C3 RISC-V**: Memahami pemetaan pin SPI pada varian C3 (MOSI, SCLK, DC, RST).
- **Hardwired CS**: Memahami implikasi `TFT_CS` yang dihubungkan langsung ke GND (selalu terpilih). Memastikan driver tidak mencoba men-toggle pin CS yang tidak ada.
- **Pin Mapping**: GPIO6 (MOSI), GPIO4 (SCLK), GPIO2 (DC), GPIO1 (RST).

## Performance
- Mengoptimalkan clock SPI sesuai batas hardware ST7789 (biasanya hingga 40-80MHz, namun FSD menyarankan ≤ 27 MHz).
