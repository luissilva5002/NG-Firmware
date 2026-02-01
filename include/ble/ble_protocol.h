#pragma once
#include <stdint.h>

// The standard data unit for our sensor
struct DataPoint {
    int16_t accel[3];
    int16_t gyro[3];
};