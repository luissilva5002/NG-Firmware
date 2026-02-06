#pragma once
#include <Arduino.h>
#include <vector>
#include "config/config.h"
#include "shared_types.h" // Ensure DataPoint is defined here
#include "power/power_manager.h"
#include "imu/imu_manager.h"
#include "ble/ble_manager.h"
#include "button/button_manager.h" // Added
#include "led/led_manager.h"       // Added
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
    ButtonManager buttonManager; // New Manager
    LEDManager ledManager;       // New Manager

    // State
    DeviceState currentState;

    // Buffers
    std::vector<DataPoint> listA;
    std::vector<DataPoint> listB;
    std::vector<DataPoint> listC;

    // Sampling Logic Variables
    bool isThresholdDetected = false;
    uint8_t lastBatteryLevel = 0;
    unsigned long lastMicros = 0;

    // Private Methods
    void handleSampling();
    void enterDeepSleep();
    bool checkThreshold(const DataPoint& dp);
    void checkStateTransitions();
};