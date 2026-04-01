#include "ble_hal.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <time.h>
#include <sys/time.h>
#include <LittleFS.h>
#include "ui_manager.h"
#include "power_manager.h"
#include "assets_wallpaper.h" // [GUARD] Cache Control

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

static BLEServer* pServer = nullptr;
static BLECharacteristic* pControlChar = nullptr;
static BLECharacteristic* pHrChar = nullptr;
static BLECharacteristic* pStepsChar = nullptr;
static BLECharacteristic* pBatChar = nullptr;
static BLECharacteristic* pWallChar = nullptr;

static File wallFile;
static uint32_t expectedSize = 0;
static uint32_t currentTotalSize = 0;

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        ble_is_connected = true;
        if (Serial) Serial.println("BLE: Connected // [READY]");
    }
    void onDisconnect(BLEServer* pServer) {
        ble_is_connected = false;
        ble_is_syncing = false;
        if (wallFile) wallFile.close();
        BLEDevice::startAdvertising();
        power_manager_set_freq(FREQ_MID);
        if (Serial) Serial.println("BLE: Connection Lost // [ABORTED]");
    }
};

class TimeSyncCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
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

class WallpaperCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        if (!wallFile) return;
        std::string value = pCharacteristic->getValue();
        uint8_t* data = (uint8_t*)value.data();
        
        // Protocol [3.2] Chunking: [0-1] Index, [2-3] Total, [4-5] Len, [6..] Payload
        if (value.length() >= 6) {
            uint16_t chunkIdx = data[0] | (data[1] << 8);
            uint16_t len = data[4] | (data[5] << 8);
            
            if (len > 0) {
                wallFile.write(&data[6], len);
                currentTotalSize += len;
                
                // Reply ACK 0x06
                uint8_t ack = 0x06;
                pWallChar->setValue(&ack, 1);
                pWallChar->notify();
                
                if (chunkIdx % 20 == 0 && Serial) Serial.printf("BLE: Received Chunk %d // [WRITING]\n", chunkIdx);
            }
        }
    }
};

class ControlCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            uint8_t cmd = value[0];
            if (cmd == 0x01 && value.length() == 5) { // Initiation
                expectedSize = value[1] | (value[2] << 8) | (value[3] << 16) | (value[4] << 24);
                currentTotalSize = 0;
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
            else if (cmd == 0x00) { ui_manager_request_state(STATE_WATCHFACE); } // New: Go Home
            else if (cmd == 0x07) { ESP.restart(); }
        }
    }
};

void ble_hal_init() {
    BLEDevice::init("ESP32Watch");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService* pService = pServer->createService(SERVICE_UUID);

    pService->createCharacteristic(UUID_TIME_SYNC, BLECharacteristic::PROPERTY_WRITE)->setCallbacks(new TimeSyncCallbacks());
    
    pControlChar = pService->createCharacteristic(UUID_CONTROL, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
    pControlChar->setCallbacks(new ControlCallbacks());
    pControlChar->addDescriptor(new BLE2902());

    pHrChar = pService->createCharacteristic(UUID_HR_DATA, BLECharacteristic::PROPERTY_NOTIFY);
    pHrChar->addDescriptor(new BLE2902());

    pStepsChar = pService->createCharacteristic(UUID_STEPS, BLECharacteristic::PROPERTY_NOTIFY);
    pStepsChar->addDescriptor(new BLE2902());

    pBatChar = pService->createCharacteristic(UUID_BATTERY, BLECharacteristic::PROPERTY_NOTIFY);
    pBatChar->addDescriptor(new BLE2902());

    pWallChar = pService->createCharacteristic(UUID_WALLPAPER, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
    pWallChar->setCallbacks(new WallpaperCallbacks());
    pWallChar->addDescriptor(new BLE2902());

    pService->start();
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->start();
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

float ble_hal_get_sync_progress() {
    if (expectedSize == 0) return 0.0f;
    return (float)currentTotalSize / (float)expectedSize;
}

void ble_hal_update() {}
