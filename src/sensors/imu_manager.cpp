#include "sensors/imu_manager.h"
#include "Wire.h"

bool ImuManager::init() {
    if (myIMU.beginCore() != 0) {
        return false;
    }
    // Set High Speed config
    myIMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL1_XL, IMU_ACCEL_FS | IMU_ACCEL_ODR);
    myIMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL2_G, IMU_GYRO_FS | IMU_GYRO_ODR);
    return true;
}

DataPoint ImuManager::read() {
    DataPoint dp;
    for (int i = 0; i < 3; i++) {
        myIMU.readRegisterInt16(&dp.accel[i], LSM6DS3_ACC_GYRO_OUTX_L_XL + 2 * i);
        myIMU.readRegisterInt16(&dp.gyro[i], LSM6DS3_ACC_GYRO_OUTX_L_G + 2 * i);
    }
    return dp;
}