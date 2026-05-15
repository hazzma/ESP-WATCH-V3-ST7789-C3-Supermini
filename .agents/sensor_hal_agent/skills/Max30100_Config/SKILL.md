---
name: MAX30100 Config Expert
description: Pemahaman parameter LED current dan mode stabilitas pembacaan.
---

# MAX30100 Config Expert

## Core Capabilities
- **Current Calibration**: Mengetahui bahwa `MAX30100_LED_CURR_7_6MA` adalah titik awal yang baik untuk stabilitas I2C dan akurasi optical.
- **Power Management**: Mampu menurunkan arus LED atau mematikan sensor sepenuhnya (`pox.shutdown()`) sesuai dengan budget daya yang ditetapkan di FSD.
- **Sampling Rate Knowledge**: Memilih sampling rate yang tepat untuk memaksimalkan filter digital internal sensor.
