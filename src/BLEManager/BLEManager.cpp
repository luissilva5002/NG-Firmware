#include "BLEManager.h"

BLEManager::BLEManager() : pServer(nullptr), pService(nullptr), pCharacteristic(nullptr), advertising(true) {}

void BLEManager::initBLE(const char* deviceName) {
    BLEDevice::init(deviceName);
    BLEDevice::setMTU(256);

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    pService = pServer->createService("A8922A2A-6FA5-4C36-83A2-8016AEB7865B");
    pCharacteristic = pService->createCharacteristic(
        "9CCCEF3B-6AAB-43EA-B9E6-F7D5B461792E",
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->setValue("Hello World");

    pCharacteristic->setCallbacks(new MyCallbacks(this));

    pService->start();
    startAdvertising();
}

void BLEManager::startAdvertising() {
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(pService->getUUID());
    pAdvertising->start();
    advertising = true;
}

void BLEManager::stopAdvertising() {
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pServer->disconnect(pServer->getConnectedCount());
    pAdvertising->stop();
    advertising = false;
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
    if (pCharacteristic) {
        pCharacteristic->setValue(&batteryLevel, sizeof(batteryLevel));
        pCharacteristic->notify();
    }
}

void BLEManager::sendSensorData(std::vector<DataPoint>& data) {
    if (!pCharacteristic) return;

    while (!data.empty()) {
        size_t batchSize = std::min(data.size(), size_t(20));
        std::vector<int16_t> batch;

        for (size_t i = 0; i < batchSize; i++) {
            batch.insert(batch.end(), std::begin(data[i].accel), std::end(data[i].accel));
            batch.insert(batch.end(), std::begin(data[i].gyro), std::end(data[i].gyro));
        }

        pCharacteristic->setValue((uint8_t*)batch.data(), batch.size() * sizeof(int16_t));
        pCharacteristic->notify();
        data.erase(data.begin(), data.begin() + batchSize);
    }
}

void BLEManager::MyCallbacks::onWrite(BLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    
    if (value.length() > 0) {
        // Check for specific commands
        if (value == "START") {
            pManager->setSamplingEnabled(true);
            Serial.println(">> Handshake Received: Sampling STARTED");
        } 
        else if (value == "STOP") {
            pManager->setSamplingEnabled(false);
            Serial.println(">> Handshake Received: Sampling STOPPED");
        }
    }
}
  
// ---------------- Server Callbacks ----------------
void BLEManager::MyServerCallbacks::onConnect(BLEServer* pServer) {
    Serial.println("Device connected");
}

void BLEManager::MyServerCallbacks::onDisconnect(BLEServer* pServer) {
    Serial.println("Device disconnected");
    pServer->startAdvertising();
}
