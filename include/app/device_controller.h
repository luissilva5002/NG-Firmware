#pragma once
#include <Arduino.h>
#include <vector>
#include "config/config.h"
#include "shared_types.h"
#include "power/power_manager.h"
#include "imu/imu_manager.h"
#include "ble/ble_manager.h"
#include "driver/rtc_io.h"
#include "esp_wifi.h"
#include "esp_sleep.h"

class DeviceController {
public:
    DeviceController();
    void setup();
    void loop();

private:
    // Sub-systems
    PowerManager powerManager;
    IMUManager imuManager;
    BLEManager bleManager;

    // Buffers
    std::vector<DataPoint> listA;
    std::vector<DataPoint> listB;
    std::vector<DataPoint> listC;

    // State Variables
    bool isButtonPressed = false;
    unsigned long buttonPressTime = 0;
    unsigned long lastBlinkTime = 0;
    int ledState = LOW;
    bool isThresholdDetected = false;
    uint8_t lastBatteryLevel = 0;
    
    // Sampling Timing
    unsigned long lastMicros = 0;

    // Methods
    void handleButton();
    void handleLED();
    void handleSampling();
    void enterDeepSleep();
    bool checkThreshold(const DataPoint& dp);
};