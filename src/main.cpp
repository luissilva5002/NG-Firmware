#include <Arduino.h>
#include "SparkFunLSM6DS3.h"
#include "Wire.h"

#include "MAX17048/MAX17048.h"
#include "BLEManager/BLEManager.h"

#define BUTTON_PIN 13
#define I2C_SDA 4
#define I2C_SCL 5
#define DEBUG_MODE

LSM6DS3 myIMU(I2C_MODE, 0x6A);
MAX17048 fuelGauge;
BLEManager bleManager;

bool isButtonPressed = false;
unsigned long buttonPressTime = 0;

std::vector<DataPoint> listA, listB, listC;
bool isThresholdDetected = false;

uint8_t lastBatteryLevel = 0;

void initI2C() {
    Wire.begin(I2C_SDA, I2C_SCL);
}

void setup() {
    Serial.begin(115200);
    initI2C();

    fuelGauge.attatch(Wire);

    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // Initialize IMU
    if (myIMU.beginCore() != 0) Serial.println(F("Error at beginCore()."));
    myIMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL1_XL,
        LSM6DS3_ACC_GYRO_BW_XL_100Hz |
        LSM6DS3_ACC_GYRO_FS_XL_16g |
        LSM6DS3_ACC_GYRO_ODR_XL_104Hz);
    myIMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL2_G,
        LSM6DS3_ACC_GYRO_FS_G_2000dps |
        LSM6DS3_ACC_GYRO_ODR_G_104Hz);

    // Initialize BLE
    bleManager.initBLE("ESP32_BT");

    lastBatteryLevel = fuelGauge.accuratePercent();
}

void loop() {
    unsigned long currentTime = millis();

    // Button handling (simplified)
    if (digitalRead(BUTTON_PIN) == LOW) {
        if (!isButtonPressed) {
            isButtonPressed = true;
            buttonPressTime = currentTime;
        }
    } else {
        isButtonPressed = false;
    }

    // Read IMU
    DataPoint dp;
    for (int i = 0; i < 3; i++) {
        myIMU.readRegisterInt16(&dp.accel[i], LSM6DS3_ACC_GYRO_OUTX_L_XL + 2 * i);
        myIMU.readRegisterInt16(&dp.gyro[i], LSM6DS3_ACC_GYRO_OUTX_L_G + 2 * i);
    }
    listA.push_back(dp);

    // Example threshold detection
    if (dp.accel[0] > 1000) isThresholdDetected = true;

    if (isThresholdDetected) {
        listB.push_back(dp);
        if (listB.size() >= 50) {
            listC = listA;
            listC.insert(listC.end(), listB.begin(), listB.end());
            bleManager.sendSensorData(listC);
            listA.clear();
            listB.clear();
            listC.clear();
            isThresholdDetected = false;
        }
    }

    // Send battery percentage via BLE
    uint8_t newLevel = fuelGauge.accuratePercent();
    if (lastBatteryLevel == 255 || abs(newLevel - lastBatteryLevel) >= 1) {
        bleManager.sendBattery(newLevel);
        lastBatteryLevel = newLevel;
    }

    delay(10);
}
