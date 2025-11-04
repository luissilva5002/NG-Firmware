#include <Arduino.h>
#include <vector>
#include <algorithm> 

#include <MAX17048.h> 
#include <LSM6DS3.h>

// Required original headers

#include "Wire.h"
#include "esp_wifi.h"
#include "esp_sleep.h"

// Custom headers
#include "BLEManager/BLEManager.h" // Your provided BLE wrapper

// --- Constants ---
#define BUTTON_PIN 0
#define SHORT_PRESS_TIME 2000
#define LONG_PRESS_TIME 5000

#define I2C_SDA 4
#define I2C_SCL 5

#define DEBUG_MODE

// --- Global Objects ---
MAX17048 fuelGauge; 
LSM6DS3Core myIMU(I2C_MODE, 0x6A);
BLEManager bleManager;

// --- Global Variables ---
bool isButtonPressed = false;
unsigned long buttonPressTime = 0;

// Data logging lists
std::vector<DataPoint> listA, listB, listC;
bool isThresholdDetected = false;

uint8_t lastBatteryLevel = 0;

// --- Function Prototypes ---
void initI2C();
uint8_t readBatteryPercentage();

// --- Functions ---
void initI2C() {
    Wire.begin(I2C_SDA, I2C_SCL);
    // Attach the Wire instance to the fuel gauge object
    fuelGauge.attach(Wire); 
}

uint8_t readBatteryPercentage() {
    // Use the percent() method from the MAX17048 library
    return fuelGauge.percent();
}

void setup() {
    Serial.begin(115200); // Initialize Serial to see printf output
    initI2C();

    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
        #ifdef DEBUG_MODE
        printf("Woke up from deep sleep.\n");
        #endif
    }

    setCpuFrequencyMhz(80);
    esp_wifi_stop();
    btStop();

    // Initialize BLE using the BLEManager
    bleManager.initBLE("ESP32_BT");
    bleManager.startAdvertising();
    
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    delay(1000);

    // Initial check for IMU
    if (myIMU.beginCore() != 0) {
        #ifdef DEBUG_MODE
        printf("Error at beginCore().\n");
        #endif
    } else {
        #ifdef DEBUG_MODE
        printf("beginCore() passed.\n");
        #endif
    }

    // Configure IMU registers
    myIMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL1_XL,
        LSM6DS3_ACC_GYRO_BW_XL_100Hz |
        LSM6DS3_ACC_GYRO_FS_XL_16g | 
        LSM6DS3_ACC_GYRO_ODR_XL_104Hz);

    myIMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL2_G,
        LSM6DS3_ACC_GYRO_FS_G_2000dps |
        LSM6DS3_ACC_GYRO_ODR_G_104Hz);

    // Read initial battery level
    lastBatteryLevel = readBatteryPercentage(); 
}

void loop() {
    static unsigned long lastSensorTime = 0;
    unsigned long currentTime = millis();

    // --- Handle button press ---
    if (digitalRead(BUTTON_PIN) == LOW) {
        if (!isButtonPressed) {
            isButtonPressed = true;
            buttonPressTime = currentTime;
        }

        unsigned long pressDuration = currentTime - buttonPressTime;
        if (pressDuration >= LONG_PRESS_TIME) {
            #ifdef DEBUG_MODE
            printf("Entering Deep Sleep...\n");
            #endif
            // Step 1: Enable wakeup for specific GPIO(s)
            gpio_wakeup_enable((gpio_num_t)BUTTON_PIN, GPIO_INTR_LOW_LEVEL); 

            // Step 2: Enable GPIO as a wakeup source
            esp_sleep_enable_gpio_wakeup();
        } else if (pressDuration >= SHORT_PRESS_TIME && bleManager.isAdvertising()) {
            #ifdef DEBUG_MODE
            printf("Disconnecting BLE and stopping advertising...\n");
            #endif
            // Stop advertising via the BLEManager
            bleManager.stopAdvertising();
        }
    } else if (isButtonPressed) {
        unsigned long pressDuration = currentTime - buttonPressTime;
        if (pressDuration < LONG_PRESS_TIME) {
            // Short press (or press released before LONG_PRESS_TIME)
            #ifdef DEBUG_MODE
            printf("Restarting BLE advertising...\n");
            #endif
            bleManager.startAdvertising();
        }
        isButtonPressed = false;
    }

    // --- Sensor data processing every 10ms ---
    if (currentTime - lastSensorTime >= 10) {
        lastSensorTime = currentTime;

        DataPoint dp;
        for (int i = 0; i < 3; i++) {
            // Read Accelerometer and Gyro data
            myIMU.readRegisterInt16(&dp.accel[i], LSM6DS3_ACC_GYRO_OUTX_L_XL + 2 * i);
            myIMU.readRegisterInt16(&dp.gyro[i], LSM6DS3_ACC_GYRO_OUTX_L_G + 2 * i);
        }

        if (isThresholdDetected) {
            listB.push_back(dp);
            if (listB.size() == 50) {
                // Combine listA (pre-trigger) and listB (post-trigger) into listC
                listC = listA;
                listC.insert(listC.end(), listB.begin(), listB.end());
                
                isThresholdDetected = false;
                listA.clear();
                listB.clear();
                
                // Send combined data
                bleManager.sendSensorData(listC);
                listC.clear();
            }
        } else {
            // Circular buffer for pre-trigger data (listA)
            if (listA.size() == 50){
                // Check threshold condition (complex trigger logic)
                bool triggerCondition1 = (dp.accel[0] < -1000 || dp.accel[0] > 1000) &&
                                        (dp.accel[1] > 6000 || dp.accel[1] < -6000) &&
                                        (dp.accel[2] > 150 || dp.accel[2] < -150);

                bool triggerCondition2 = (dp.accel[0] > 1000) &&
                                        (dp.accel[1] < -6000) &&
                                        (dp.accel[2] < -150);
                
                if (triggerCondition1 || triggerCondition2) {
                    isThresholdDetected = true;
                }
                
                // Remove oldest element from the front of the circular buffer
                listA.erase(listA.begin());
            }
            // Add new data point to the back of the circular buffer
            listA.push_back(dp);

            #ifdef DEBUG_MODE
            // Using printf with %d (or %hi) format specifiers for int16_t values
            printf("%d, %d, %d\n", dp.accel[0], dp.accel[1], dp.accel[2]);
            #endif 
        }
    }

    // --- Check battery with debounce ---
    uint8_t newLevel = readBatteryPercentage();
    // Only send notification if:
    // 1. lastBatteryLevel is 255 (initial state) OR
    // 2. The new level differs from the last sent level by 1% or more
    if (lastBatteryLevel == 255 || abs((int)newLevel - (int)lastBatteryLevel) >= 1) {
      bleManager.sendBattery(newLevel);
      lastBatteryLevel = newLevel;
    }
}