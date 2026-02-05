#pragma once
#include <Arduino.h>

struct DataPoint {
    int16_t accel[3];
    int16_t gyro[3];
};