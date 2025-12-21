#include <Arduino.h>
#include <vector>
#include <algorithm>

#include <MAX17048.h>
#include <LSM6DS3.h>

#include "Wire.h"
#include "esp_wifi.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"

#include "BLEManager/BLEManager.h"

// ================= CONSTANTS =================

#define BUTTON_PIN 0
#define SHORT_PRESS_TIME 2000
#define LONG_PRESS_TIME 5000

#define I2C_SDA 4
#define I2C_SCL 5

#define LED_PIN 3
#define LED_BLINK_INTERVAL 500

#define DEBUG_MODE 0

#define PRE_BUF_SIZE    800
#define POST_BUF_SIZE   600

#define OMEGA_IMPACT_MIN      400000000L   // Minimum ω² peak to confirm impact

// ================= GLOBAL OBJECTS =================

MAX17048 fuelGauge;
LSM6DS3Core myIMU(I2C_MODE, 0x6A);
BLEManager bleManager;

// ================= GLOBAL VARIABLES =================

bool isButtonPressed = false;
unsigned long buttonPressTime = 0;
unsigned long lastBlinkTime = 0;
int ledState = LOW;

uint8_t lastBatteryLevel = 0;

// --- IMU Buffers ---
DataPoint preBuf[PRE_BUF_SIZE];
DataPoint postBuf[POST_BUF_SIZE];

uint16_t preIdx = 0;
uint16_t postIdx = 0;

// --- Logic Control ---
bool capturingPost = false; // Replaces the State Machine
int32_t omega2_prev = 0;
int32_t dOmega2_prev = 0;
uint16_t impactPreIdx = 0;

// ================= FUNCTION PROTOTYPES =================

void initI2C();
uint8_t readBatteryPercentage();
void handleLEDStatus();

// ================= FUNCTIONS =================

void initI2C() {
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(400000); // 400kHz Fast Mode
    fuelGauge.attach(Wire);
}

uint8_t readBatteryPercentage() {
    return fuelGauge.percent();
}

void handleLEDStatus() {
    unsigned long currentTime = millis();

    if (bleManager.isConnected()) {
        digitalWrite(LED_PIN, HIGH);
    }
    else if (bleManager.isAdvertising()) {
        if (currentTime - lastBlinkTime >= LED_BLINK_INTERVAL) {
            lastBlinkTime = currentTime;
            ledState = !ledState;
            digitalWrite(LED_PIN, ledState);
        }
    }
    else {
        digitalWrite(LED_PIN, LOW);
        ledState = LOW;
    }
}

// ================= SETUP =================

void setup() {
    Serial.begin(115200);
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

    bleManager.initBLE("ESP32_BT");
    bleManager.startAdvertising();

    delay(500);

    if (myIMU.beginCore() != 0) {
        #ifdef DEBUG_MODE
            printf("IMU init error\n");
        #endif
    }

    // Set Sensor to 833Hz
    myIMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL1_XL,
        LSM6DS3_ACC_GYRO_FS_XL_16g | 
        LSM6DS3_ACC_GYRO_ODR_XL_833Hz); 

    myIMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL2_G,
        LSM6DS3_ACC_GYRO_FS_G_2000dps | 
        LSM6DS3_ACC_GYRO_ODR_G_833Hz);

    lastBatteryLevel = readBatteryPercentage();
    
    #ifdef DEBUG_MODE
        printf("Setup Complete. 833Hz Cyclic Buffer Mode.\n");
    #endif
}

// ================= LOOP =================

void loop() {
    // 833Hz Timer
    static unsigned long lastMicros = 0;
    unsigned long currentMicros = micros();
    unsigned long currentTime = millis(); // For UI logic

    handleLEDStatus();

    // -------- BUTTON HANDLING --------
    if (digitalRead(BUTTON_PIN) == LOW) {
        if (!isButtonPressed) {
            isButtonPressed = true;
            buttonPressTime = currentTime;
        }
        unsigned long pressDuration = currentTime - buttonPressTime;
        if (pressDuration >= LONG_PRESS_TIME) {
            while (digitalRead(BUTTON_PIN) == LOW) delay(10);
            digitalWrite(LED_PIN, LOW);
            esp_deep_sleep_start();
        }
        else if (pressDuration >= SHORT_PRESS_TIME && bleManager.isAdvertising()) {
            bleManager.disconnect();
        }
    }
    else if (isButtonPressed) {
        bleManager.startAdvertising();
        isButtonPressed = false;
    }

    // -------- IMU SAMPLING (833 Hz) --------
    if (currentMicros - lastMicros >= 1200) { 
        lastMicros = currentMicros;

        DataPoint dp;
        // Read raw data
        for (int i = 0; i < 3; i++) {
            myIMU.readRegisterInt16(&dp.accel[i], LSM6DS3_ACC_GYRO_OUTX_L_XL + 2 * i);
            myIMU.readRegisterInt16(&dp.gyro[i],  LSM6DS3_ACC_GYRO_OUTX_L_G  + 2 * i);
        }

        // --- Calc Energy & Derivative ---
        // Using int64_t for calculation to prevent overflow with high thresholds
        int64_t wx = dp.gyro[0];
        int64_t wy = dp.gyro[1];
        int64_t wz = dp.gyro[2];

        int64_t omega2_64 = (wx*wx) + (wy*wy) + (wz*wz);
        int32_t omega2 = (int32_t)omega2_64; // Cast back to int32 for comparison/storage
        int32_t dOmega2 = omega2 - omega2_prev;

        #ifdef DEBUG_MODE
            printf("%ld,%ld\n", omega2, dOmega2); // Uncomment for Serial Plotter
        #endif
        
        if (capturingPost) {
            postBuf[postIdx++] = dp;

            if (postIdx >= POST_BUF_SIZE) {
                #ifdef DEBUG_MODE
                    printf(">> Post Buffer Full. Sending data...\n");
                #endif

                std::vector<DataPoint> impactFrame;
                impactFrame.reserve(PRE_BUF_SIZE + POST_BUF_SIZE);

                for (int i = 0; i < PRE_BUF_SIZE; i++) {
                    uint16_t idx = (impactPreIdx + i) % PRE_BUF_SIZE;
                    impactFrame.push_back(preBuf[idx]);
                }

                for (int i = 0; i < POST_BUF_SIZE; i++) {
                    impactFrame.push_back(postBuf[i]);
                }

                bleManager.sendSensorData(impactFrame);
                
                capturingPost = false;
                postIdx = 0;
                printf(">> Data Sent. Resuming monitoring.\n");
            }
        } 
        else {
            preBuf[preIdx] = dp;
            
            if (dOmega2_prev > 0 && dOmega2 < 0 && omega2 > OMEGA_IMPACT_MIN) {
                
                #ifdef DEBUG_MODE
                    printf(">> IMPACT TRIGGERED! Peak: %ld\n", omega2);
                #endif

                capturingPost = true;
                impactPreIdx = (preIdx + 1) % PRE_BUF_SIZE; // The next index would have been the oldest
                postIdx = 0;
            }

            preIdx = (preIdx + 1) % PRE_BUF_SIZE;
        }

        omega2_prev = omega2;
        dOmega2_prev = dOmega2;
    }

    uint8_t newLevel = readBatteryPercentage();
    if (lastBatteryLevel == 255 || abs((int)newLevel - (int)lastBatteryLevel) >= 1) {
        bleManager.sendBattery(newLevel);
        lastBatteryLevel = newLevel;
    }
}