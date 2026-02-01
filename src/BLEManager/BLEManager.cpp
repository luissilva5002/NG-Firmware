#include "BLEManager.h"

BLEManager::BLEManager() : 
    pServer(nullptr), 
    pService(nullptr), 
    pCharacteristic(nullptr), 
    _advertising(false),
    _samplingEnabled(false),
    _otaRequested(false) 
{}

void BLEManager::initBLE(const char* deviceName) {
    NimBLEDevice::init(deviceName);
    NimBLEDevice::setMTU(256); // Optimized for larger data packets

    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    pService = pServer->createService("A8922A2A-6FA5-4C36-83A2-8016AEB7865B");
    
    pCharacteristic = pService->createCharacteristic(
        "9CCCEF3B-6AAB-43EA-B9E6-F7D5B461792E",
        NIMBLE_PROPERTY::READ |
        NIMBLE_PROPERTY::WRITE |
        NIMBLE_PROPERTY::NOTIFY
    );
    
    // Use setValue to set initial data
    pCharacteristic->setValue("Hello World");
    pCharacteristic->setCallbacks(new MyCallbacks(this)); // Attach Write Callback
    
    pService->start();

    startAdvertising();
}

void BLEManager::startAdvertising() {
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(pService->getUUID());
    pAdvertising->start();
    _advertising = true;
}

void BLEManager::stopAdvertising() {
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    if (pServer->getConnectedCount() > 0) {
        // Forcing disconnect:
        pServer->disconnect(0); 
    }
    pAdvertising->stop();
    _advertising = false;
}

void BLEManager::disconnect() {
    if (pServer && pServer->getConnectedCount() > 0) {
        pServer->disconnect(0);
    }
}

bool BLEManager::isConnected() {
    return pServer && (pServer->getConnectedCount() > 0);
}

void BLEManager::sendBattery(uint8_t batteryLevel) {
    if (pCharacteristic && isConnected()) {
        pCharacteristic->setValue(&batteryLevel, sizeof(batteryLevel));
        pCharacteristic->notify();
    }
}

void BLEManager::sendSensorData(std::vector<DataPoint>& data) {
    if (!pCharacteristic || !isConnected()) return;

    // Batching logic preserved
    while (!data.empty()) {
        size_t batchSize = std::min(data.size(), size_t(20)); // Adjust based on MTU 256
        std::vector<int16_t> batch;
        batch.reserve(batchSize * 6);

        for (size_t i = 0; i < batchSize; i++) {
            batch.insert(batch.end(), std::begin(data[i].accel), std::end(data[i].accel));
            batch.insert(batch.end(), std::begin(data[i].gyro), std::end(data[i].gyro));
        }

        pCharacteristic->setValue((uint8_t*)batch.data(), batch.size() * sizeof(int16_t));
        pCharacteristic->notify();
        data.erase(data.begin(), data.begin() + batchSize);
    }
}
  
// ---------------- Server Callbacks ----------------
void BLEManager::MyServerCallbacks::onConnect(NimBLEServer* pServer) {
    printf("Device connected\n");
}

void BLEManager::MyServerCallbacks::onDisconnect(NimBLEServer* pServer) {
    printf("Device disconnected\n");
    NimBLEDevice::startAdvertising(); // Auto-restart advertising
}

// ---------------- Characteristic Callbacks (Command Logic) ----------------
void BLEManager::MyCallbacks::onWrite(NimBLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    
    if (value.length() > 0) {
        if (value.find("START") != std::string::npos) {
            pManager->setSamplingEnabled(true);
            printf("Command: START Sampling\n");
        } 
        else if (value.find("STOP") != std::string::npos) {
            pManager->setSamplingEnabled(false);
            printf("Command: STOP Sampling\n");
        }
        else if (value.find("OTA_UP") != std::string::npos) {
            pManager->setOtaRequested(true);
            printf("Command: OTA Requested\n");
        }
    }
}