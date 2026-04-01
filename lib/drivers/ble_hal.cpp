#include "ble_hal.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <time.h>
#include <sys/time.h>
#include "ui_manager.h" // [GUARD] Remote state control

extern bool ble_is_syncing;
extern bool ble_is_connected;
extern bool ui_is_high_load;
extern AppState current_state;

#define SERVICE_UUID           "12345678-1234-1234-1234-123456789abc"
#define UUID_TIME_SYNC         "12345678-1234-1234-1234-123456789001"
#define UUID_WALLPAPER         "12345678-1234-1234-1234-123456789002"
#define UUID_HR_DATA           "12345678-1234-1234-1234-123456789003"
#define UUID_STEPS             "12345678-1234-1234-1234-123456789004"
#define UUID_BATTERY           "12345678-1234-1234-1234-123456789005"
#define UUID_CONTROL           "12345678-1234-1234-1234-123456789006"

static BLEServer* pServer = nullptr;
static BLECharacteristic* pControlChar = nullptr;
static BLECharacteristic* pHrChar = nullptr;
static BLECharacteristic* pStepsChar = nullptr;
static BLECharacteristic* pBatChar = nullptr;

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        ble_is_connected = true;
        if (Serial) Serial.println("BLE: Connected // [READY]");
    }
    void onDisconnect(BLEServer* pServer) {
        ble_is_connected = false;
        ble_is_syncing = false;
        BLEDevice::startAdvertising();
        if (Serial) Serial.println("BLE: Disconnected // [WAIT]");
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

// [3.4] Control Commands
class ControlCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            uint8_t cmd = value[0];
            if (cmd == 0x03) { // HR/SpO2 Reading Request
                ui_manager_request_state(STATE_EXEC_HR);
                if (Serial) Serial.println("BLE: Remote HR Request // [CMD]");
            }
            else if (cmd == 0x04) { // Request Step Count
                ui_manager_report_sensors(0x04);
            }
            else if (cmd == 0x05) { // Request Battery Status
                ui_manager_report_sensors(0x05);
            }
            else if (cmd == 0x07) { // Reboot
                ESP.restart();
            }
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

    pService->createCharacteristic(UUID_WALLPAPER, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY)->addDescriptor(new BLE2902());

    pService->start();
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->start();
    if (Serial) Serial.println("BLE: Prototype Protocol v6.2.2 Online // [READY]");
}

// [GUIDE 3.3] Sensor Notifications
void ble_hal_notify_hr(uint8_t bpm, uint8_t spo2) {
    if (!ble_is_connected) return;
    uint8_t data[4];
    data[0] = bpm;
    data[1] = spo2;
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    uint16_t mins = t->tm_hour * 60 + t->tm_min;
    data[2] = mins & 0xFF;
    data[3] = (mins >> 8) & 0xFF;
    pHrChar->setValue(data, 4);
    pHrChar->notify();
}

void ble_hal_notify_steps(uint32_t steps) {
    if (!ble_is_connected) return;
    uint8_t data[4];
    data[0] = steps & 0xFF;
    data[1] = (steps >> 8) & 0xFF;
    data[2] = (steps >> 16) & 0xFF;
    data[3] = (steps >> 24) & 0xFF;
    pStepsChar->setValue(data, 4);
    pStepsChar->notify();
}

void ble_hal_notify_battery(uint8_t percent, bool charging) {
    if (!ble_is_connected) return;
    uint8_t data[2];
    data[0] = percent;
    data[1] = charging ? 1 : 0;
    pBatChar->setValue(data, 2);
    pBatChar->notify();
}

void ble_hal_update() {}
