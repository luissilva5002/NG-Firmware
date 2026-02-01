#include "ota/ota_manager.h"

void OtaManager::begin(NimBLEServer* server) {
    // Create the OTA Service using UUID from config
    NimBLEService* pService = server->createService(UUID_SERVICE_OTA);
    
    // 1. Data Characteristic (Fast Writes for firmware chunks)
    // We use WRITE_NR (No Response) to speed up transmission
    NimBLECharacteristic* pDataChar = pService->createCharacteristic(
        UUID_CHAR_OTA_DATA,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    pDataChar->setCallbacks(new DataCallbacks());

    // 2. Control Characteristic (Commands: Start, End, Status)
    NimBLECharacteristic* pControlChar = pService->createCharacteristic(
        UUID_CHAR_OTA_CONTROL,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY
    );
    pControlChar->setCallbacks(new ControlCallbacks());

    pService->start();
}

// ------------------------------------------------------------
// Internal Callback Implementations
// ------------------------------------------------------------

void OtaManager::DataCallbacks::onWrite(NimBLECharacteristic* pCharacteristic) {
    std::string rxData = pCharacteristic->getValue();
    // Only write data if an update process has actually started
    if (rxData.length() > 0 && Update.isRunning()) {
        Update.write((uint8_t*)rxData.data(), rxData.length());
    }
}

void OtaManager::ControlCallbacks::onWrite(NimBLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() == 0) return;

    // Simple protocol: Byte 0 determines the command
    uint8_t cmd = value[0];

    // Command 0x01: START OTA
    if (cmd == 0x01) { 
        // Begin update (Size unknown, save to Flash)
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
            printf("[OTA] Start Failed: %s\n", Update.errorString());
        } else {
            printf("[OTA] Start\n");
        }
    } 
    // Command 0x02: END OTA
    else if (cmd == 0x02) { 
        // End update and check if successful
        if (Update.end(true)) {
            printf("[OTA] Success. Rebooting...\n");
            delay(1000);
            ESP.restart();
        } else {
            printf("[OTA] End Failed: %s\n", Update.errorString());
        }
    }
}