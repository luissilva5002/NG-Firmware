#pragma once
#include <Arduino.h>
#include <vector>
#include <NimBLEDevice.h>
#include "ble_protocol.h"
#include "config/ble_config.h"

class BleManager {
public:
    BleManager();
    void init();
    void updateBattery(uint8_t level);
    void sendSensorData(std::vector<DataPoint>& data);
    
    // Flags controlled by App
    bool isConnected();
    bool isSamplingEnabled() const { return _samplingEnabled; }
    bool isOtaRequested() const { return _otaRequested; }
    
    // Setters for flags
    void setSamplingEnabled(bool s) { _samplingEnabled = s; }
    void setOtaRequested(bool s) { _otaRequested = s; }
    
    NimBLEServer* getServer() { return pServer; }

private:
    NimBLEServer* pServer;
    NimBLECharacteristic* pCharacteristic;
    bool _samplingEnabled = false;
    bool _otaRequested = false;

    // Inner callback classes
    class ServerCallbacks : public NimBLEServerCallbacks {
        void onConnect(NimBLEServer* pServer) override;
        void onDisconnect(NimBLEServer* pServer) override;
    };
    class CommandCallbacks : public NimBLECharacteristicCallbacks {
    public:
        CommandCallbacks(BleManager* m) : manager(m) {}
        void onWrite(NimBLECharacteristic* pChar) override;
    private:
        BleManager* manager;
    };
};