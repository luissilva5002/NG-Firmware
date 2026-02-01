#pragma once
#include <Arduino.h>
#include <vector>
#include <NimBLEDevice.h>

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
    
    // Status Getters
    bool isAdvertising() const { return _advertising; }
    
    // Logic Flags (Required for main.cpp)
    bool isSamplingEnabled() const { return _samplingEnabled; }
    void setSamplingEnabled(bool state) { _samplingEnabled = state; }
    
    bool isOtaRequested() const { return _otaRequested; }
    void setOtaRequested(bool state) { _otaRequested = state; }

    // Helper for OTA Class
    NimBLEServer* getNimBLEServer() { return pServer; }

private:
    NimBLEServer* pServer;
    NimBLEService* pService;
    NimBLECharacteristic* pCharacteristic;
    
    bool _advertising;
    bool _samplingEnabled;
    bool _otaRequested;

    // Callbacks
    class MyServerCallbacks : public NimBLEServerCallbacks {
        void onConnect(NimBLEServer* pServer) override;
        void onDisconnect(NimBLEServer* pServer) override;
    };

    class MyCallbacks : public NimBLECharacteristicCallbacks {
    public:
        MyCallbacks(BLEManager* m) : pManager(m) {}
        void onWrite(NimBLECharacteristic* pCharacteristic) override;
    private:
        BLEManager* pManager;
    };
};