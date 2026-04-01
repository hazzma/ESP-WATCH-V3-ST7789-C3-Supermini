# Firmware Integration Guide: C3Watch Companion App

This document provides the exact technical specifications for a firmware agent (e.g., ESP32-C3 developer) to integrate with this Flutter application.

## 1. Device Identification
The app filters for devices strictly by their **Advertised Name**.
- **Target Device Name:** `ESP32Watch`
- **BLE Stack Requirement:** Must support GATT Server with stable notifications.

## 2. GATT Service & Characteristics Mapping
The application expects the following UUID structure. All characteristics must be under the **same Service UUID**.

**Base Service UUID:** `12345678-1234-1234-1234-123456789abc`

| Characteristic | UUID | Properties | Protocol / Format |
| :--- | :--- | :--- | :--- |
| **Time Sync** | `...9001` | Write | 8-byte buffer (see Section 3.1) |
| **Wallpaper** | `...9002` | Write, Notify | Chunked Binary + ACK (see Section 3.2) |
| **HR Data** | `...9003` | Notify | 4-byte buffer (see Section 3.3) |
| **Steps** | `...9004` | Notify | 4-byte buffer (uint32 LE steps) |
| **Battery** | `...9005` | Notify | 2-byte (percent, charging_flag) |
| **Control** | `...9006` | Write, Notify | Command Byte + Payload |

---

## 3. Communication Protocols

### 3.1. Auto Time Sync (Incoming to Watch)
Sent by the app immediately upon connection to characteristic `...9001`.
- **Packet Size:** 8 Bytes
- **Structure:**
  - `[0]`: Year (offset from 2000, e.g., 26 for 2026)
  - `[1]`: Month (1-12)
  - `[2]`: Day (1-31)
  - `[3]`: Hour (0-23)
  - `[4]`: Minute (0-59)
  - `[5]`: Second (0-59)
  - `[6]`: Weekday (0=Sun, 1=Mon ... 6=Sat)
  - `[7]`: Checksum (`XOR` of bytes 0-6)
- **Watch Action:** Update RTC and send `0x01` ACK via `Control` characteristic if required.

### 3.2. Wallpaper Transfer (Incoming to Watch)
High-speed binary transfer to characteristic `...9002`.
1. **Initiation:** App writes to `Control` characteristic: `[0x01, size_byte0, size_byte1, size_byte2, size_byte3]` (4-byte LE total size).
   - **Watch Action:** Reply `0x01` on `Control` to start.
2. **Chunking:** App sends ~263 chunks.
   - **Chunk Structure (Header 6 Bytes + Payload):**
     - `[0-1]`: Chunk Index (uint16 LE)
     - `[2-3]`: Total Chunks (uint16 LE)
     - `[4-5]`: Data Length (uint16 LE, max 512)
     - `[6..]`: RGB565 Pixel Data (Total 134,400 bytes for 240x280)
   - **Watch Action:** For every chunk, reply `0x06` (ACK) or `0x15` (NAK/Retry) on `Wallpaper` notify characteristic.
3. **Completion:** App writes to `Control`: `[0x02, crc32_b0, crc32_b1, crc32_b2, crc32_b3]`.
   - **Watch Action:** Verify CRC32. Reply `0x01` (Success) or `0x02` (Fail) on `Control`.

### 3.3. Sensor Data (Outgoing to App)
Watch must send **Notifications** on these characteristics when requested or periodically.

- **Heart Rate (`...9003`):** 4 Bytes
  - `[0]`: BPM (uint8)
  - `[1]`: SpO2 (uint8)
  - `[2-3]`: Minutes since midnight (uint16 LE)
- **Steps (`...9004`):** 4 Bytes
  - `[0-3]`: Cumulative Step Count (uint32 LE)
- **Battery (`...9005`):** 2 Bytes
  - `[0]`: Percentage (uint8, 0-100)
  - `[1]`: Charging (0 = No, 1 = Yes)

### 3.4. Control Commands (Requesting States)
The app will send 1-byte commands to `Control` characteristic `...9006`:
- `0x03`: Request HR/SpO2 Reading
- `0x04`: Request Step Count
- `0x05`: Request Battery Status
- `0x07`: Reboot Order (Watch should restart immediately)

---

## 4. Implementation Checklist for Firmware Agent
- [ ] Set BLE Name to `ESP32Watch`.
- [ ] Flash the Service UUID `12345678-1234-1234-1234-123456789abc`.
- [ ] Implement `XOR` checksum verification for Time Sync.
- [ ] Implement a buffer to receive 134,400 bytes for Wallpaper (SPIFFS/LittleFS/Flash recommended).
- [ ] Handle 10-second request timeouts gracefully.
