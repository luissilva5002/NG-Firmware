#include "ble/ble_manager.h"

BLEManager::BLEManager() : pServer(nullptr), pService(nullptr), pCharacteristic(nullptr), advertising(true) {}

void BLEManager::initBLE() {
    printf("Initializing NimBLE...\n");
    NimBLEDevice::init(BLE_DEVICE_NAME);
    // NimBLE handles MTU negotiation automatically mostly, but we can hint
    NimBLEDevice::setMTU(256); 

    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

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
    startAdvertising();
}

void BLEManager::startAdvertising() {
    printf("Advertising...\n");
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(pService->getUUID());
    pAdvertising->start();
    advertising = true;
}

void BLEManager::stopAdvertising() {
    printf("Stopping advertising...\n");
    NimBLEDevice::getAdvertising()->stop();
    advertising = false;
}

void BLEManager::disconnect() {
    if (pServer && pServer->getConnectedCount() > 0) {
        // NimBLE allows disconnecting specific client, 0 usually implies first or all depending on implementation
        // For simple server:
        pServer->disconnect(0); 
    }
}

bool BLEManager::isConnected() {
    return pServer && (pServer->getConnectedCount() > 0);
}

void BLEManager::sendBattery(uint8_t batteryLevel) {
    if (pCharacteristic) {
        // Note: notify() in NimBLE can take pointer and size directly
        pCharacteristic->setValue(&batteryLevel, sizeof(batteryLevel));
        pCharacteristic->notify();
    }
}

void BLEManager::sendSensorData(std::vector<DataPoint>& data) {
    if (!pCharacteristic) return;
    printf("Sending sensor data (%d points)...\n", data.size());

    while (!data.empty()) {
        size_t batchSize = std::min(data.size(), size_t(20));
        std::vector<int16_t> batch;
        batch.reserve(batchSize * 6); // 6 int16s per datapoint

        for (size_t i = 0; i < batchSize; i++) {
            batch.insert(batch.end(), std::begin(data[i].accel), std::end(data[i].accel));
            batch.insert(batch.end(), std::begin(data[i].gyro), std::end(data[i].gyro));
        }

        pCharacteristic->setValue((uint8_t*)batch.data(), batch.size() * sizeof(int16_t));
        pCharacteristic->notify();
        data.erase(data.begin(), data.begin() + batchSize);
    }
}

// --- Callbacks ---

void BLEManager::MyServerCallbacks::onConnect(NimBLEServer* pServer) {
    printf("Device connected\n");
}

void BLEManager::MyServerCallbacks::onDisconnect(NimBLEServer* pServer) {
    printf("Device disconnected\n");
    // Auto-restart advertising on disconnect is standard
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