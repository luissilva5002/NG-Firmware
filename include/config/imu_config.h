#pragma once
#include <LSM6DS3.h> 

// --- IMU Init Settings ---
#define IMU_MODE I2C_MODE
#define IMU_I2C_ADDR 0x6A

// --- Trigger Thresholds (Raw Values) ---
#define TRIG_ACC_X_MIN -1000
#define TRIG_ACC_X_MAX 1000
#define TRIG_ACC_Y_POS 6000
#define TRIG_ACC_Y_NEG -6000
#define TRIG_ACC_Z_POS 150
#define TRIG_ACC_Z_NEG -150