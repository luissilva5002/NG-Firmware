#pragma once
#include <Arduino.h>
#include "ota/firmware_storage.h"

enum class OTAState {
    IDLE,
    PREPARING,
    DOWNLOADING,
    FINALIZING,
    ERROR
};

class OTAManager {
public:
    OTAManager();
    
    void begin();
    
    // Called when "Start OTA" command is received
    bool startOTA(size_t firmwareSize);
    
    // Called when data packet is received
    void processData(const uint8_t* data, size_t len);
    
    // Called when "End OTA" command is received
    void finishOTA();
    
    // To check if we should block other device operations
    bool isOTAActive();
    
    OTAState getState();

private:
    FirmwareStorage storage;
    OTAState currentState;
    size_t bytesReceived;
    size_t totalExpectedBytes;
};