#pragma once
#include "config/imu_config.h"
#include "ble/ble_protocol.h"

class ImuManager {
public:
    bool init();
    DataPoint read();
private:
    LSM6DS3Core myIMU = LSM6DS3Core(IMU_MODE, IMU_I2C_ADDR);
};