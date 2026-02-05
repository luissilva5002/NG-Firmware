#include "imu/imu_manager.h"

IMUManager::IMUManager() : myIMU(IMU_MODE, IMU_I2C_ADDR) {}

bool IMUManager::init() {
    // 1. Initialize I2C Bus
    // Note: We use the constants from DeviceConfig.h (SDA=4, SCL=5)
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_CLOCK_SPEED);

    // 2. Initialize Sensor Core
    if (myIMU.beginCore() != 0) {
        return false;
    }

    // 3. Configure Sensor Settings 
    // Uses the updated macros from IMUConfig.h (104Hz, 16g/2000dps)
    myIMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL1_XL, IMU_ACCEL_SETTINGS);
    myIMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL2_G,  IMU_GYRO_SETTINGS);

    return true;
}

void IMUManager::readData(DataPoint& dp) {
    // Reading Loop (Matches your working main.cpp logic)
    for (int i = 0; i < 3; i++) {
        // Read Accelerometer (X, Y, Z)
        // Base address OUTX_L_XL + offset
        myIMU.readRegisterInt16(&dp.accel[i], LSM6DS3_ACC_GYRO_OUTX_L_XL + 2 * i);
        
        // Read Gyroscope (X, Y, Z)
        // Base address OUTX_L_G + offset
        myIMU.readRegisterInt16(&dp.gyro[i], LSM6DS3_ACC_GYRO_OUTX_L_G + 2 * i);
    }
}