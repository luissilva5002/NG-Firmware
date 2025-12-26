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
    void disconnect();
    bool isConnected();
    void sendBattery(uint8_t batteryLevel);
    void sendSensorData(std::vector<DataPoint>& data);
    
    bool isAdvertising() const { return advertising; }
    bool isSamplingEnabled() const { return _samplingEnabled; }
    void setSamplingEnabled(bool state) { _samplingEnabled = state; }

private:
    BLEServer* pServer;
    BLEService* pService;
    BLECharacteristic* pCharacteristic;
    bool advertising;
    
    // Flag to control the sampling loop
    bool _samplingEnabled = false; 

    class MyServerCallbacks : public BLEServerCallbacks {
        void onConnect(BLEServer* pServer) override;
        void onDisconnect(BLEServer* pServer) override;
    };

    // --- NEW: Callback for writing (Receiving commands from App) ---
    class MyCallbacks : public BLECharacteristicCallbacks {
    public:
        MyCallbacks(BLEManager* manager) : pManager(manager) {}
        void onWrite(BLECharacteristic* pCharacteristic) override;
    private:
        BLEManager* pManager;
    };
};