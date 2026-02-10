#include "ota/ota_manager.h"

OTAManager::OTAManager() : currentState(OTAState::IDLE), bytesReceived(0), totalExpectedBytes(0) {}

void OTAManager::begin() {
    currentState = OTAState::IDLE;
}

bool OTAManager::startOTA(size_t firmwareSize) {
    if (currentState != OTAState::IDLE) {
        return false; // Already running
    }

    printf("OTA Manager: Starting...\n");
    currentState = OTAState::PREPARING;
    bytesReceived = 0;
    totalExpectedBytes = firmwareSize;

    if (storage.start(firmwareSize)) {
        currentState = OTAState::DOWNLOADING;
        return true;
    } else {
        currentState = OTAState::ERROR;
        return false;
    }
}

void OTAManager::processData(const uint8_t* data, size_t len) {
    if (currentState != OTAState::DOWNLOADING) return;

    if (!storage.write(data, len)) {
        printf("OTA Manager: Write failed!\n");
        currentState = OTAState::ERROR;
        storage.abort();
        return;
    }

    bytesReceived += len;
    
    // Optional: Print progress every 10KB
    if (bytesReceived % 10240 == 0) {
        printf("OTA: %u bytes received\n", bytesReceived);
    }
}

void OTAManager::finishOTA() {
    if (currentState != OTAState::DOWNLOADING) return;

    printf("OTA Manager: Finalizing...\n");
    currentState = OTAState::FINALIZING;

    if (storage.complete()) {
        printf("OTA Manager: Rebooting in 2 seconds...\n");
        delay(2000);
        ESP.restart();
    } else {
        printf("OTA Manager: Verification failed.\n");
        currentState = OTAState::ERROR;
        storage.abort();
    }
}

bool OTAManager::isOTAActive() {
    return currentState != OTAState::IDLE && currentState != OTAState::ERROR;
}

OTAState OTAManager::getState() {
    return currentState;
}