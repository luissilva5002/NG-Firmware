#include <Arduino.h>
#include <vector>
#include <algorithm> 

#include <MAX17048.h> 
#include <LSM6DS3.h>
#include "Wire.h"
#include "esp_wifi.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"

// Your custom headers
#include "BLEManager/BLEManager.h"
#include "BLEOta.h"

// --- Constants ---
#define BUTTON_PIN 0
#define SHORT_PRESS_TIME 2000
#define LONG_PRESS_TIME 5000
#define I2C_SDA 4
#define I2C_SCL 5
#define DEBUG_MODE 1
#define LED_PIN 3 
#define LED_BLINK_INTERVAL 500
#define BUFFER_LIMIT 400 

// --- Global Objects ---
MAX17048 fuelGauge; 
LSM6DS3Core myIMU(I2C_MODE, 0x6A);
BLEManager bleManager;
BLEOta myOta; 

// --- Global Variables ---
bool isButtonPressed = false;
unsigned long buttonPressTime = 0;
unsigned long lastBlinkTime = 0; 
int ledState = LOW;
bool otaMaintenanceMode = false; 

std::vector<DataPoint> listA; 
std::vector<DataPoint> listB; 
std::vector<DataPoint> listC; 
bool isThresholdDetected = false;
uint8_t lastBatteryLevel = 0;

// --- Functions ---
void initI2C() {
    Wire.begin(I2C_SDA, I2C_SCL);
    fuelGauge.attach(Wire);
}

uint8_t readBatteryPercentage() { return fuelGauge.percent(); }

void handleLEDStatus() {
    unsigned long currentTime = millis();
    if (bleManager.isConnected()) {
        digitalWrite(LED_PIN, HIGH);
    } else if (bleManager.isAdvertising()) {
        if (currentTime - lastBlinkTime >= LED_BLINK_INTERVAL) {
            lastBlinkTime = currentTime;
            ledState = !ledState; 
            digitalWrite(LED_PIN, ledState);
        }
    } else {
        digitalWrite(LED_PIN, LOW);
        ledState = LOW;
    }
}

void setup() {
    delay(1000); 

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    gpio_hold_dis((gpio_num_t)BUTTON_PIN);
    
    initI2C();

    gpio_wakeup_enable((gpio_num_t)BUTTON_PIN, GPIO_INTR_LOW_LEVEL);
    esp_deep_sleep_enable_gpio_wakeup(1ULL << BUTTON_PIN, ESP_GPIO_WAKEUP_GPIO_LOW);

    setCpuFrequencyMhz(80);
    esp_wifi_stop();
    btStop(); 

    // 1. Init BLE
    bleManager.initBLE("Smasher");

    // 2. Attach OTA Service
    myOta.begin(bleManager.getNimBLEServer());
    
    bleManager.startAdvertising();
    delay(500);

    if (myIMU.beginCore() != 0) {
        printf("IMU init error\n");
    }

    // SWITCH TO FAST I2C NOW
    Wire.setClock(400000); 

    myIMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL1_XL, LSM6DS3_ACC_GYRO_FS_XL_16g | LSM6DS3_ACC_GYRO_ODR_XL_833Hz);
    myIMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL2_G, LSM6DS3_ACC_GYRO_FS_G_2000dps | LSM6DS3_ACC_GYRO_ODR_G_833Hz);

    listA.reserve(BUFFER_LIMIT + 10);
    listB.reserve(BUFFER_LIMIT + 10);
    listC.reserve(BUFFER_LIMIT * 2 + 10);
    lastBatteryLevel = readBatteryPercentage(); 
    
    printf("Setup Complete. Waiting for BLE Connection...\n");
}

void loop() {
    // --- OTA PRIORITY LOGIC ---
    if (Update.isRunning() || otaMaintenanceMode) {
        digitalWrite(LED_PIN, (millis() / 100) % 2); // Fast OTA blink
        return;
    } 
    
    // Check for Manual OTA Trigger via Command
    if (bleManager.isOtaRequested()) {
        printf("OTA Command Received. Pausing sensors...\n");
        bleManager.setOtaRequested(false);
        otaMaintenanceMode = true; 
        return;
    }

    // --- NORMAL SENSOR LOGIC ---
    static unsigned long lastMicros = 0;
    unsigned long currentMicros = micros();
    unsigned long currentTime = millis();

    handleLEDStatus(); 

    // --- Button Logic ---
    if (digitalRead(BUTTON_PIN) == LOW) {
        if (!isButtonPressed) {
            isButtonPressed = true;
            buttonPressTime = currentTime;
        }
        unsigned long pressDuration = currentTime - buttonPressTime;
        if (pressDuration >= LONG_PRESS_TIME) {
            printf("Deep Sleep...\n");
            while (digitalRead(BUTTON_PIN) == LOW) delay(10);
            digitalWrite(LED_PIN, LOW);
            pinMode(BUTTON_PIN, INPUT_PULLUP);
            gpio_wakeup_enable((gpio_num_t)BUTTON_PIN, GPIO_INTR_LOW_LEVEL);
            esp_deep_sleep_enable_gpio_wakeup(1ULL << BUTTON_PIN, ESP_GPIO_WAKEUP_GPIO_LOW);
            gpio_hold_dis((gpio_num_t)BUTTON_PIN);
            esp_deep_sleep_start();
        } else if (pressDuration >= SHORT_PRESS_TIME && bleManager.isAdvertising()) {
            bleManager.disconnect();
        }
    } else if (isButtonPressed) {
        if ((currentTime - buttonPressTime) < LONG_PRESS_TIME) {
             bleManager.startAdvertising();
        }
        isButtonPressed = false;
    }

    // --- SAMPLING LOGIC ---
    if (bleManager.isConnected()) {
        
        // Check the flag set by the callback
        if (bleManager.isSamplingEnabled()) {
            
            static long lastPrint = 0;
            if (millis() - lastPrint > 1000) {
                printf("Sampling Active... Buffer A: %d\n", listA.size());
                lastPrint = millis();
            }

            if (currentMicros - lastMicros >= 1200) {
                lastMicros = currentMicros;

                DataPoint dp;
                // ... (Reading Logic) ...
                for (int i = 0; i < 3; i++) {
                    myIMU.readRegisterInt16(&dp.accel[i], LSM6DS3_ACC_GYRO_OUTX_L_XL + 2 * i);
                    myIMU.readRegisterInt16(&dp.gyro[i], LSM6DS3_ACC_GYRO_OUTX_L_G + 2 * i);
                }

                if (isThresholdDetected) {
                     listB.push_back(dp);
                     
                     if (listB.size() >= BUFFER_LIMIT) {    
                        printf("Captured! Sending...\n");

                        listC = listA;
                        listC.insert(listC.end(), listB.begin(), listB.end());

                        bleManager.sendSensorData(listC);

                        isThresholdDetected = false;

                        listA.clear(); listB.clear(); listC.clear();
                        listA.reserve(BUFFER_LIMIT); listB.reserve(BUFFER_LIMIT);
                     }
                } else {
                    if (listA.size() >= BUFFER_LIMIT) {
                         // Threshold check logic...
                         if((dp.accel[0] < -1000 || dp.accel[0] > 1000) && (dp.accel[1] > 6000 || dp.accel[1] < -6000) && (dp.accel[2] > 150 || dp.accel[2] < -150)) {
                             isThresholdDetected = true;
                             printf("Triggered!\n");
                         }
                         listA.erase(listA.begin());
                    }
                    listA.push_back(dp);
                }
            }
        } else {
            // Connected, but IDLE (Waiting for START)
            static long lastIdlePrint = 0;
            if (millis() - lastIdlePrint > 2000) {
                 printf("Connected but IDLE. Waiting for START command...\n");
                 lastIdlePrint = millis();
            }
            // Clear buffers to be ready
            if (!listA.empty()) listA.clear();
            isThresholdDetected = false;
        }
        
        // --- Battery Logic (Always runs when connected) ---
        static unsigned long lastBatCheck = 0;
        if (currentTime - lastBatCheck > 5000) { 
            lastBatCheck = currentTime;
            uint8_t newLevel = readBatteryPercentage();
            if (lastBatteryLevel == 255 || abs((int)newLevel - (int)lastBatteryLevel) >= 1) {
                bleManager.sendBattery(newLevel);
                lastBatteryLevel = newLevel;
            }
        }
    } 
}