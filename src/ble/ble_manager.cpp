#include "ble/ble_manager.h"

BleManager::BleManager() : 
    pServer(nullptr), 
    pCharacteristic(nullptr),
    _samplingEnabled(false),
    _otaRequested(false) 
{}

void BleManager::init() {
    // 1. Initialize NimBLE Stack
    // DEVICE_NAME is defined in include/config/ble_config.h
    NimBLEDevice::init(DEVICE_NAME);
    
    // Optimize for larger data packets (Sensor batches)
    NimBLEDevice::setMTU(256); 

    // 2. Create Server & Callbacks
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    // 3. Create Service
    NimBLEService* pService = pServer->createService(UUID_SERVICE_MAIN);
    
    // 4. Create Data Characteristic (Read/Write/Notify)
    pCharacteristic = pService->createCharacteristic(
        UUID_CHAR_DATA,
        NIMBLE_PROPERTY::READ |
        NIMBLE_PROPERTY::WRITE |
        NIMBLE_PROPERTY::NOTIFY
    );
    
    // Set initial value and attach Write Callbacks (for commands like START/STOP)
    pCharacteristic->setValue("Ready");
    pCharacteristic->setCallbacks(new CommandCallbacks(this)); 
    
    // 5. Start Service
    pService->start();

    // 6. Start Advertising so the phone can find us
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(pService->getUUID());
    pAdvertising->start();
}

bool BleManager::isConnected() {
    return pServer && (pServer->getConnectedCount() > 0);
}

void BleManager::updateBattery(uint8_t level) {
    if (pCharacteristic && isConnected()) {
        // We reuse the main characteristic for simplicity, 
        // or you could add a dedicated Battery Service later.
        // For now, adhering to your original logic:
        pCharacteristic->setValue(&level, sizeof(level));
        pCharacteristic->notify();
    }
}

void BleManager::sendSensorData(std::vector<DataPoint>& data) {
    if (!pCharacteristic || !isConnected()) return;

    size_t totalPoints = data.size();
    size_t processed = 0;

    // Process the vector in batches to fit within MTU limits
    while (processed < totalPoints) {
        
        // 1. Determine Batch Size (Keep it safe at 15 points per packet)
        size_t remaining = totalPoints - processed;
        size_t batchSize = std::min(remaining, size_t(15)); 

        // 2. Prepare the Payload (Flat byte array of Int16s)
        std::vector<int16_t> batch;
        batch.reserve(batchSize * 6); // 3 axes Accel + 3 axes Gyro

        for (size_t i = 0; i < batchSize; i++) {
            const DataPoint& dp = data[processed + i];
            batch.insert(batch.end(), std::begin(dp.accel), std::end(dp.accel));
            batch.insert(batch.end(), std::begin(dp.gyro), std::end(dp.gyro));
        }

        // 3. Send Notification
        // Casting vector data to uint8_t* for transmission
        pCharacteristic->setValue((uint8_t*)batch.data(), batch.size() * sizeof(int16_t));
        pCharacteristic->notify(); 
        
        // 4. Critical Flow Control
        processed += batchSize;

        // Wait 15ms to let the BLE radio transmit the packet. 
        // Without this, the ESP32 internal buffer overflows and packets get dropped.
        delay(15); 
    }
    
    // Clear the original vector now that transmission is complete
    data.clear();
}

// ------------------------------------------------------------
// Internal Callbacks Implementation
// ------------------------------------------------------------

void BleManager::ServerCallbacks::onConnect(NimBLEServer* pServer) {
    printf("[BLE] Device connected\n");
}

void BleManager::ServerCallbacks::onDisconnect(NimBLEServer* pServer) {
    printf("[BLE] Device disconnected - Restarting Advertisement\n");
    NimBLEDevice::startAdvertising(); 
}

void BleManager::CommandCallbacks::onWrite(NimBLECharacteristic* pChar) {
    std::string value = pChar->getValue();
    
    if (value.length() > 0) {
        // Check for text commands sent from the App
        if (value.find("START") != std::string::npos) {
            manager->setSamplingEnabled(true);
            printf("[BLE CMD] START Sampling\n");
        } 
        else if (value.find("STOP") != std::string::npos) {
            manager->setSamplingEnabled(false);
            printf("[BLE CMD] STOP Sampling\n");
        }
        else if (value.find("OTA_UP") != std::string::npos) {
            manager->setOtaRequested(true);
            printf("[BLE CMD] OTA Requested\n");
        }
    }
}