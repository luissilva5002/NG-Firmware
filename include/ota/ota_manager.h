#pragma once
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <Update.h>
#include "config/ble_config.h"

class OtaManager {
public:
    // Called in DeviceController::setup()
    void begin(NimBLEServer* server);

private:
    // Inner callback classes for handling OTA data flow
    class DataCallbacks : public NimBLECharacteristicCallbacks {
        void onWrite(NimBLECharacteristic* pCharacteristic) override;
    };

    class ControlCallbacks : public NimBLECharacteristicCallbacks {
        void onWrite(NimBLECharacteristic* pCharacteristic) override;
    };
};