#include "ble_hal.h"
#include <NimBLEDevice.h>
#include <time.h>
#include <sys/time.h>
#include <LittleFS.h>
#include "ui_manager.h"
#include "power_manager.h"
#include "assets_wallpaper.h" // [GUARD] Cache Control
#include "calibration_manager.h"

extern bool ble_is_syncing;
extern bool ble_is_connected;
extern bool ui_is_high_load;
extern AppState current_state;

#define SERVICE_UUID           "12345678-1234-1234-1234-123456789abc"
#define UUID_TIME_SYNC         "12345678-1234-1234-1234-123456789001"
#define UUID_WALLPAPER         "12345678-1234-1234-1234-123456789002" // [STEP 5]
#define UUID_HR_DATA           "12345678-1234-1234-1234-123456789003"
#define UUID_STEPS             "12345678-1234-1234-1234-123456789004"
#define UUID_BATTERY           "12345678-1234-1234-1234-123456789005"
#define UUID_CONTROL           "12345678-1234-1234-1234-123456789006"
#define UUID_CALIBRATION       "12345678-1234-1234-1234-123456789007"

static NimBLEServer* pServer = nullptr;
static NimBLECharacteristic* pControlChar = nullptr;
static NimBLECharacteristic* pHrChar = nullptr;
static NimBLECharacteristic* pStepsChar = nullptr;
static NimBLECharacteristic* pBatChar = nullptr;
static NimBLECharacteristic* pWallChar = nullptr;
static NimBLECharacteristic* pCalChar = nullptr;

static File wallFile;
static uint32_t expectedSize = 0;
static uint32_t currentTotalSize = 0;
static uint16_t nextExpectedChunk = 0; // [SYNC GUARD]
static bool ble_initialized = false;

class MyServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
        ble_is_connected = true;
        if (Serial) Serial.println("BLE: Connected // [READY]");
    }
    void onDisconnect(NimBLEServer* pServer) {
        ble_is_connected = false;
        ble_is_syncing = false;
        if (wallFile) wallFile.close();
        if (power_manager_get_ble_enabled()) {
            BLEDevice::startAdvertising(); // Resume advertising when disconnected
        }
        power_manager_set_freq(FREQ_MID);
        if (Serial) Serial.println("BLE: Disconnected! // [STATE]");
    }
};

class TimeSyncCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() == 8) {
            uint8_t* data = (uint8_t*)value.data();
            uint8_t checksum = 0;
            for (int i = 0; i < 7; i++) checksum ^= data[i];
            if (checksum == data[7]) {
                struct tm t;
                t.tm_year = data[0] + 100;
                t.tm_mon  = data[1] - 1;
                t.tm_mday = data[2];
                t.tm_hour = data[3];
                t.tm_min  = data[4];
                t.tm_sec  = data[5];
                t.tm_isdst = -1;
                time_t timeSinceEpoch = mktime(&t);
                struct timeval tv = { .tv_sec = timeSinceEpoch, .tv_usec = 0 };
                settimeofday(&tv, NULL);
                uint8_t ack = 0x01;
                pControlChar->setValue(&ack, 1);
                pControlChar->notify();
            }
        }
    }
};

class WallpaperCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic) {
        if (!wallFile) return;
        std::string value = pCharacteristic->getValue();
        uint8_t* data = (uint8_t*)value.data();
        
        // Protocol [3.2] Chunking: [0-1] Index, [2-3] Total, [4-5] Len, [6..] Payload
        if (value.length() >= 6) {
            uint16_t chunkIdx = data[0] | (data[1] << 8);
            uint16_t len = data[4] | (data[5] << 8);
            
            if (chunkIdx == nextExpectedChunk) {
                if (len > 0) {
                    wallFile.write(&data[6], len);
                    currentTotalSize += len;
                    nextExpectedChunk++;
                    
                    // Reply UNIQUE ACK 0x06 + Index [low, high]
                    uint8_t reply[3];
                    reply[0] = 0x06;
                    reply[1] = chunkIdx & 0xFF;
                    reply[2] = (chunkIdx >> 8) & 0xFF;
                    
                    pWallChar->setValue(reply, 3);
                    pWallChar->notify();
                    
                    if (chunkIdx % 25 == 0 && Serial) Serial.printf("BLE: Chunk %d OK // [ACK SENT]\n", chunkIdx);
                }
            } else {
                // Reply NAK 0x15 (Out of order)
                uint8_t nak = 0x15;
                pWallChar->setValue(&nak, 1);
                pWallChar->notify();
                if (Serial) Serial.printf("BLE: Index Mismatch! Got %d, Expected %d // [NAK]\n", chunkIdx, nextExpectedChunk);
            }
        }
    }
};

class ControlCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            uint8_t cmd = value[0];
            if (cmd == 0x01 && value.length() == 5) { // Initiation
                expectedSize = value[1] | (value[2] << 8) | (value[3] << 16) | (value[4] << 24);
                currentTotalSize = 0;
                nextExpectedChunk = 0; // [GUARD] Start from Zero
                wallFile = LittleFS.open("/wallpaper.bin", "w");
                ble_is_syncing = true;
                power_manager_set_freq(FREQ_HIGH); // Turbo Mode
                
                uint8_t ack = 0x01;
                pControlChar->setValue(&ack, 1);
                pControlChar->notify();
                if (Serial) Serial.printf("BLE: Preparing for %d bytes // [INIT]\n", expectedSize);
            }
            else if (cmd == 0x02) { // Completion
                if (wallFile) wallFile.close();
                assets_wallpaper_clear_cache(); 
                ble_is_syncing = false;
                power_manager_set_freq(FREQ_MID);
                
                uint8_t status = (currentTotalSize == expectedSize) ? 0x01 : 0x15; // Success or NAK
                pControlChar->setValue(&status, 1);
                pControlChar->notify();
                if (Serial) Serial.printf("BLE: Wallpaper Transfer %s (%d/%d bytes) // [DONE]\n", (status == 0x01 ? "Success" : "FAILED"), currentTotalSize, expectedSize);
            }
            else if (cmd == 0x03) { ui_manager_request_state(STATE_EXEC_HR); }
            else if (cmd == 0x04) { ui_manager_report_sensors(0x04); }
            else if (cmd == 0x05) { ui_manager_report_sensors(0x05); }
            else if (cmd == 0x00) { ui_manager_request_state(STATE_WATCHFACE); } 
            else if (cmd == 0x08) { ui_manager_request_state(STATE_WATCHFACE); } // Stop HR (Return Home)
            else if (cmd == 0x09) { /* Stop Step Streaming Placeholder */ }
            else if (cmd == 0x07) { ESP.restart(); }
            else if (cmd == 0x0A && value.length() == 15) { 
                calibration_manager_apply((uint8_t*)value.data() + 1, 14); 
            }
            else if (cmd == 0x0B) { 
                calibration_manager_send_current(); 
            }
            else if (cmd == 0x0C) { 
                calibration_manager_reset(); 
            }
        }
    }
};

void ble_hal_init() {
    if (!power_manager_get_ble_enabled()) return;
    if (ble_initialized) return;

    NimBLEDevice::init("ESP32Watch");
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    NimBLEService* pService = pServer->createService(SERVICE_UUID);

    pService->createCharacteristic(UUID_TIME_SYNC, NIMBLE_PROPERTY::WRITE)->setCallbacks(new TimeSyncCallbacks());
    
    pControlChar = pService->createCharacteristic(UUID_CONTROL, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);
    pControlChar->setCallbacks(new ControlCallbacks());

    pHrChar = pService->createCharacteristic(UUID_HR_DATA, NIMBLE_PROPERTY::NOTIFY);

    pStepsChar = pService->createCharacteristic(UUID_STEPS, NIMBLE_PROPERTY::NOTIFY);

    pBatChar = pService->createCharacteristic(UUID_BATTERY, NIMBLE_PROPERTY::NOTIFY);

    pWallChar = pService->createCharacteristic(UUID_WALLPAPER, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);
    pWallChar->setCallbacks(new WallpaperCallbacks());

    pCalChar = pService->createCharacteristic(UUID_CALIBRATION, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);

    pService->start();
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->start();
    ble_initialized = true;
    if (Serial) Serial.println("BLE: Connected Architecture v6.3 Online // [READY]");
}

void ble_hal_notify_hr(uint8_t bpm, uint8_t spo2) {
    if (!ble_is_connected || ui_is_high_load) return;
    uint8_t data[4]; data[0] = bpm; data[1] = spo2;
    time_t now = time(NULL); struct tm* t = localtime(&now);
    uint16_t mins = t->tm_hour * 60 + t->tm_min;
    data[2] = mins & 0xFF; data[3] = (mins >> 8) & 0xFF;
    pHrChar->setValue(data, 4); pHrChar->notify();
}

void ble_hal_notify_steps(uint32_t steps) {
    if (!ble_is_connected || ui_is_high_load) return;
    uint8_t data[4]; data[0] = steps & 0xFF; data[1] = (steps >> 8) & 0xFF; data[2] = (steps >> 16) & 0xFF; data[3] = (steps >> 24) & 0xFF;
    pStepsChar->setValue(data, 4); pStepsChar->notify();
}

void ble_hal_notify_battery(uint8_t percent, bool charging) {
    if (!ble_is_connected || ui_is_high_load) return;
    uint8_t data[2]; data[0] = percent; data[1] = charging ? 1 : 0;
    pBatChar->setValue(data, 2); pBatChar->notify();
}

void ble_hal_notify_calibration(uint8_t* data, size_t len) {
    if (!ble_is_connected || pCalChar == nullptr) return;
    pCalChar->setValue(data, len);
    pCalChar->notify();
}

float ble_hal_get_sync_progress() {
    if (expectedSize == 0) return 0.0f;
    return (float)currentTotalSize / (float)expectedSize;
}

void ble_hal_update() {}

void ble_hal_update_enabled() {
    bool target = power_manager_get_ble_enabled();
    if (target && !ble_initialized) {
        ble_hal_init();
    } else if (target && ble_initialized) {
        NimBLEDevice::getAdvertising()->start();
        if (Serial) Serial.println("BLE: Advertising Started // [ON]");
    } else if (!target && ble_initialized) {
        NimBLEDevice::deinit(true);
        ble_initialized = false;
        ble_is_connected = false;
        ble_is_syncing = false;
        if (Serial) Serial.println("BLE: Radio Deinitialized // [OFF]");
    }
}
