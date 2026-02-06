#pragma once
#include <Arduino.h>

// Define the states here so everyone can see them
enum class DeviceState {
    ADVERTISING,
    CONNECTED,
    SAMPLING,
    SLEEP
};

// ... existing DataPoint struct or other shared types ...
struct DataPoint {
    int16_t accel[3];
    int16_t gyro[3];
};