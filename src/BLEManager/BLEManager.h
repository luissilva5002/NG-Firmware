#pragma once
#include <Arduino.h>
#include <vector>
#include "BLEDevice.h"

// Struct for accelerometer + gyro data
struct DataPoint {
    int16_t accel[3];
    int16_t gyro[3];
};

class BLEManager {
public:
    BLEManager();
    void initBLE(const char* deviceName);
    void startAdvertising();
    void stopAdvertising();
    void sendBattery(uint8_t batteryLevel);
    void sendSensorData(std::vector<DataPoint>& data);
    bool isAdvertising() const { return advertising; }

private:
    BLEServer* pServer;
    BLEService* pService;
    BLECharacteristic* pCharacteristic;
    bool advertising;

    class MyServerCallbacks : public BLEServerCallbacks {
        void onConnect(BLEServer* pServer) override;
        void onDisconnect(BLEServer* pServer) override;
    };
};
