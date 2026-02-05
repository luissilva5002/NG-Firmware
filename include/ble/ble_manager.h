#pragma once
#include <Arduino.h>
#include <vector>
#include <NimBLEDevice.h>

#include "shared_types.h"
#include "config/config.h"

class BLEManager {
public:
    BLEManager();
    void initBLE();
    void startAdvertising();
    void stopAdvertising();
    void disconnect();
    bool isConnected();
    bool isAdvertising() const { return advertising; }

    // Logic State
    bool isSamplingEnabled() const { return _samplingEnabled; }
    void setSamplingEnabled(bool state) { _samplingEnabled = state; }

    // Data Sending
    void sendBattery(uint8_t batteryLevel);
    void sendSensorData(std::vector<DataPoint>& data);

private:
    NimBLEServer* pServer;
    NimBLEService* pService;
    NimBLECharacteristic* pCharacteristic;
    bool advertising;
    bool _samplingEnabled = false;

    // Callbacks
    class MyServerCallbacks : public NimBLEServerCallbacks {
        void onConnect(NimBLEServer* pServer) override;
        void onDisconnect(NimBLEServer* pServer) override;
    };

    class MyCallbacks : public NimBLECharacteristicCallbacks {
    public:
        MyCallbacks(BLEManager* manager) : pManager(manager) {}
        void onWrite(NimBLECharacteristic* pCharacteristic) override;
    private:
        BLEManager* pManager;
    };
};