#pragma once
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <Update.h>

class BLEOta {
public:
    void begin(NimBLEServer* server) {
        NimBLEService* pService = server->createService("8018");
        
        // Data Characteristic (Write No Response for speed)
        NimBLECharacteristic* pDataChar = pService->createCharacteristic(
            "8019",
            NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
        );
        pDataChar->setCallbacks(new DataCallbacks());

        // Control Characteristic
        NimBLECharacteristic* pControlChar = pService->createCharacteristic(
            "8020",
            NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY
        );
        pControlChar->setCallbacks(new ControlCallbacks());

        pService->start();
    }

private:
    class DataCallbacks : public NimBLECharacteristicCallbacks {
        void onWrite(NimBLECharacteristic* pCharacteristic) override {
            std::string rxData = pCharacteristic->getValue();
            if (rxData.length() > 0 && Update.isRunning()) {
                Update.write((uint8_t*)rxData.data(), rxData.length());
            }
        }
    };

    class ControlCallbacks : public NimBLECharacteristicCallbacks {
        void onWrite(NimBLECharacteristic* pCharacteristic) override {
            std::string value = pCharacteristic->getValue();
            if (value.length() == 0) return;

            uint8_t cmd = value[0];
            if (cmd == 0x01) { // START OTA
                // Check for space
                if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
                    printf("OTA Start Failed: %s\n", Update.errorString());
                } else {
                    printf("OTA Start\n");
                }
            } 
            else if (cmd == 0x02) { // END OTA
                if (Update.end(true)) {
                    printf("OTA Success. Rebooting...\n");
                    delay(1000);
                    ESP.restart();
                } else {
                    printf("OTA End Failed: %s\n", Update.errorString());
                }
            }
        }
    };
};