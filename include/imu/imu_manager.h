#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <LSM6DS3.h>
#include "shared_types.h"
#include "config/device_config.h"
#include "config/imu_config.h"

class IMUManager {
public:
    IMUManager();
    bool init(); 
    void readData(DataPoint& dp);

private:
    LSM6DS3Core myIMU;
};