#include "imu/imu_manager.h"


IMUManager::IMUManager() : myIMU(IMU_MODE, IMU_I2C_ADDR) {}

bool IMUManager::init() {

    if (myIMU.beginCore() != 0) {
        printf("IMU init error\n");
    }

    myIMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL1_XL, LSM6DS3_ACC_GYRO_FS_XL_16g | LSM6DS3_ACC_GYRO_ODR_XL_833Hz);
    myIMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL2_G, LSM6DS3_ACC_GYRO_FS_G_2000dps | LSM6DS3_ACC_GYRO_ODR_G_833Hz);

    printf("[IMU] Initialization complete.");
    return true;
}

void IMUManager::readData(DataPoint& dp) {
    // Reading Loop
    for (int i = 0; i < 3; i++) {
        // Read Accelerometer (X, Y, Z)
        myIMU.readRegisterInt16(&dp.accel[i], LSM6DS3_ACC_GYRO_OUTX_L_XL + 2 * i);
        
        // Read Gyroscope (X, Y, Z)
        myIMU.readRegisterInt16(&dp.gyro[i], LSM6DS3_ACC_GYRO_OUTX_L_G + 2 * i);
    }
}