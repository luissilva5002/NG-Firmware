#include "imu/imu_manager.h"

// Define WHO_AM_I register address if not exposed by the library
#define LSM6DS3_WHO_AM_I_REG 0x0F
#define LSM6DS3_WHO_AM_I_ID  0x69

IMUManager::IMUManager() : myIMU(IMU_MODE, IMU_I2C_ADDR) {}

bool IMUManager::init() {
    // 1. Initialize I2C Bus explicitly on your pins
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_CLOCK_SPEED);

    // 2. MANUAL CHECK (Bypass beginCore to prevent pin reset)
    uint8_t deviceID = 0;
    
    // Read the WHO_AM_I register (0x0F)
    // Note: readRegister returns 0 on success in this library
    if (myIMU.readRegister(&deviceID, LSM6DS3_WHO_AM_I_REG) != 0) {
        printf("IMU: I2C Read Failed\n");
        return false; 
    }
    
    if (deviceID != LSM6DS3_WHO_AM_I_ID) {
        printf("IMU: ID Mismatch. Read: 0x%02X, Expected: 0x%02X\n", deviceID, LSM6DS3_WHO_AM_I_ID);
        return false; 
    }

    // 3. Configure Sensor Settings 
    // Using the macros you defined in imu_config.h
    if (myIMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL1_XL, IMU_ACCEL_SETTINGS) != 0) {
        return false;
    }
    
    if (myIMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL2_G,  IMU_GYRO_SETTINGS) != 0) {
        return false;
    }

    // Optional: Enable Block Data Update (BDU) for data consistency
    // Prevents reading high/low bytes from different samples
    myIMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL3_C, 0x44); 

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