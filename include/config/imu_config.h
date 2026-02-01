#pragma once
#include <LSM6DS3.h>

// Sensor Settings
#define IMU_I2C_ADDR    0x6A
#define IMU_MODE        I2C_MODE

// Tuning Parameters
#define IMU_ACCEL_FS    LSM6DS3_ACC_GYRO_FS_XL_16g
#define IMU_ACCEL_ODR   LSM6DS3_ACC_GYRO_ODR_XL_833Hz
#define IMU_GYRO_FS     LSM6DS3_ACC_GYRO_FS_G_2000dps
#define IMU_GYRO_ODR    LSM6DS3_ACC_GYRO_ODR_G_833Hz