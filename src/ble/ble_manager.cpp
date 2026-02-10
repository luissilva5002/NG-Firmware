#include "ble/ble_manager.h"

// Ensure these are defined if not already in ble_config.h
#ifndef OTA_SERVICE_UUID
#define OTA_SERVICE_UUID "12345678-1234-1234-1234-1234567890AB"
#endif
#ifndef OTA_CHAR_CONTROL
#define OTA_CHAR_CONTROL "12345678-1234-1234-1234-1234567890AC"
#endif
#ifndef OTA_CHAR_DATA
#define OTA_CHAR_DATA    "12345678-1234-1234-1234-1234567890AD"
#endif

// --- OTA Callbacks ---

class OTAControlCallbacks : public NimBLECharacteristicCallbacks {
    OTAManager* ota;
public:
    OTAControlCallbacks(OTAManager* _ota) : ota(_ota) {}
    void onWrite(NimBLECharacteristic* pChar) override {
        std::string value = pChar->getValue();
        if (value.length() == 0) return;

        uint8_t cmd = value[0];
        printf("OTA Control Cmd: 0x%02X\n", cmd);

        if (cmd == 0x01) { // START
            // Optional: Read size from bytes 1-4 if sent
            size_t size = 0;
            if (value.length() >= 5) {
                size = (uint8_t)value[1] | ((uint8_t)value[2] << 8) | ((uint8_t)value[3] << 16) | ((uint8_t)value[4] << 24);
            }
            ota->startOTA(size);
        } else if (cmd == 0x02) { // END
            ota->finishOTA();
        } else if (cmd == 0xFF) { // ABORT (Optional)
             // ota->abort(); 
        }
    }
};

class OTADataCallbacks : public NimBLECharacteristicCallbacks {
    OTAManager* ota;
public:
    OTADataCallbacks(OTAManager* _ota) : ota(_ota) {}
    void onWrite(NimBLECharacteristic* pChar) override {
        std::string value = pChar->getValue();
        if (value.length() > 0) {
            ota->processData((const uint8_t*)value.data(), value.length());
        }
    }
};


// --- Implementation ---

BLEManager::BLEManager() : pServer(nullptr), pService(nullptr), pCharacteristic(nullptr), otaManager(nullptr), advertising(true) {}

void BLEManager::init(OTAManager* otaParams) {
    this->otaManager = otaParams;

    printf("Initializing NimBLE...\n");
    NimBLEDevice::init(BLE_DEVICE_NAME);
    NimBLEDevice::setMTU(256); // Optimize for OTA throughput

    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // 1. Main Sensor Service
    pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ |
        NIMBLE_PROPERTY::WRITE |
        NIMBLE_PROPERTY::NOTIFY
    );
    pCharacteristic->setValue("Hello World");
    pCharacteristic->setCallbacks(new MyCallbacks(this));
    pService->start();

    // 2. OTA Service
    if (otaManager) {
        pOtaService = pServer->createService(OTA_SERVICE_UUID);
        
        // Control Characteristic
        pOtaControlCharacteristic = pOtaService->createCharacteristic(
            OTA_CHAR_CONTROL,
            NIMBLE_PROPERTY::WRITE
        );
        pOtaControlCharacteristic->setCallbacks(new OTAControlCallbacks(otaManager));

        // Data Characteristic (WriteNR is faster for bulk data)
        pOtaDataCharacteristic = pOtaService->createCharacteristic(
            OTA_CHAR_DATA,
            NIMBLE_PROPERTY::WRITE_NR
        );
        pOtaDataCharacteristic->setCallbacks(new OTADataCallbacks(otaManager));

        pOtaService->start();
    }

    startAdvertising();
}

void BLEManager::startAdvertising() {
    printf("Advertising...\n");
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    
    // Advertise Main Service
    pAdvertising->addServiceUUID(SERVICE_UUID);
    
    // Advertise OTA Service (So app knows we support it)
    if (otaManager) {
        pAdvertising->addServiceUUID(OTA_SERVICE_UUID);
    }
    
    pAdvertising->setScanResponse(true);
    pAdvertising->start();
    advertising = true;
}

void BLEManager::stopAdvertising() {
    printf("Stopping advertising...\n");
    NimBLEDevice::getAdvertising()->stop();
    advertising = false;
}

void BLEManager::disconnect() {
    if (pServer) {
        std::vector<uint16_t> peerIds = pServer->getPeerDevices();
        if (peerIds.empty()) {
            printf("No devices to disconnect.\n");
            return;
        }
        for (uint16_t id : peerIds) {
            printf("Force disconnecting Client ID: %d\n", id);
            pServer->disconnect(id); 
        }
    }
}

bool BLEManager::isConnected() {
    return pServer && (pServer->getConnectedCount() > 0);
}

void BLEManager::sendBattery(uint8_t batteryLevel) {
    if (pCharacteristic) {
        pCharacteristic->setValue(&batteryLevel, sizeof(batteryLevel));
        pCharacteristic->notify();
    }
}

void BLEManager::sendSensorData(std::vector<DataPoint>& data) {
    if (!pCharacteristic) return;
    // Note: Logging this might be too verbose during high-speed sampling
    // printf("Sending sensor data (%d points)...\n", data.size());

    while (!data.empty()) {
        size_t batchSize = std::min(data.size(), size_t(20));
        std::vector<int16_t> batch;
        batch.reserve(batchSize * 6); 

        for (size_t i = 0; i < batchSize; i++) {
            batch.insert(batch.end(), std::begin(data[i].accel), std::end(data[i].accel));
            batch.insert(batch.end(), std::begin(data[i].gyro), std::end(data[i].gyro));
        }

        pCharacteristic->setValue((uint8_t*)batch.data(), batch.size() * sizeof(int16_t));
        pCharacteristic->notify();

        delay(20); // Maintain flow control
        data.erase(data.begin(), data.begin() + batchSize);
    }
}

// --- Main Service Callbacks ---

void BLEManager::MyServerCallbacks::onConnect(NimBLEServer* pServer) {
    printf("Device connected\n");
}

void BLEManager::MyServerCallbacks::onDisconnect(NimBLEServer* pServer) {
    printf("Device disconnected\n");
    NimBLEDevice::startAdvertising();
}

void BLEManager::MyCallbacks::onWrite(NimBLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
        printf(">> BLE Cmd: %s\n", value.c_str());
        if (value.find("START") != std::string::npos) {
            pManager->setSamplingEnabled(true);
            printf("START Accepted\n");
        } else if (value.find("STOP") != std::string::npos) {
            pManager->setSamplingEnabled(false);
            printf("STOP Accepted\n");
        }
    }
}