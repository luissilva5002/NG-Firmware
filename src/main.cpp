#include <Arduino.h>
#include <vector>
#include <algorithm> 

#include <MAX17048.h> 
#include <LSM6DS3.h>

// Required original headers
#include "Wire.h"
#include "esp_wifi.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"

// Custom headers
#include "BLEManager/BLEManager.h"

// --- Constants ---
#define BUTTON_PIN 0
#define SHORT_PRESS_TIME 2000
#define LONG_PRESS_TIME 5000

#define I2C_SDA 4
#define I2C_SCL 5

#define DEBUG_MODE

#define LED_PIN 3 
#define LED_BLINK_INTERVAL 500

// --- BUFFER SETTINGS ---
// 833 Hz means 1 sample every 1.2ms.
// To keep 0.5 seconds of history (like before), we need ~400 samples.
#define BUFFER_LIMIT 400 

// --- Global Objects ---
MAX17048 fuelGauge; 
LSM6DS3Core myIMU(I2C_MODE, 0x6A);
BLEManager bleManager;

// --- Global Variables ---
bool isButtonPressed = false;
unsigned long buttonPressTime = 0;
unsigned long lastBlinkTime = 0; 
int ledState = LOW;

// Data logging lists
std::vector<DataPoint> listA; // Pre-buffer
std::vector<DataPoint> listB; // Post-buffer
std::vector<DataPoint> listC; // Combined
bool isThresholdDetected = false;

uint8_t lastBatteryLevel = 0;

// --- Function Prototypes ---
void initI2C();
uint8_t readBatteryPercentage();
void handleLEDStatus(); 

// --- Functions ---
void initI2C() {
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(400000); // CRITICAL: Increase I2C speed to 400kHz for high freq sampling
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


void setup() {
    Serial.begin(115200); // Ensure Serial is started for Debug
    
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    gpio_hold_dis((gpio_num_t)BUTTON_PIN);
    
    initI2C();

    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_GPIO) {
        #ifdef DEBUG_MODE
        printf("Woke up from deep sleep.\n");
        #endif
    }

    // Configure button as wake-up source
    gpio_wakeup_enable((gpio_num_t)BUTTON_PIN, GPIO_INTR_LOW_LEVEL);
    esp_deep_sleep_enable_gpio_wakeup(1ULL << BUTTON_PIN, ESP_GPIO_WAKEUP_GPIO_LOW);

    setCpuFrequencyMhz(80);
    esp_wifi_stop();
    btStop();

    // Initialize BLE
    bleManager.initBLE("ESP32_BT");
    bleManager.startAdvertising();

    delay(1000);

    // Initial check for IMU
    if (myIMU.beginCore() != 0) {
        #ifdef DEBUG_MODE
        printf("Error at beginCore().\n");
        #endif
    }

    // --- Configure IMU for High Speed (833 Hz) ---
    // Note: 833Hz is the safe max for standard I2C. 
    myIMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL1_XL,
        LSM6DS3_ACC_GYRO_FS_XL_16g |
        LSM6DS3_ACC_GYRO_ODR_XL_833Hz);

    myIMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL2_G,
        LSM6DS3_ACC_GYRO_FS_G_2000dps |
        LSM6DS3_ACC_GYRO_ODR_G_833Hz);

    // Optimize Memory: Reserve space to prevent reallocation lag
    listA.reserve(BUFFER_LIMIT + 10);
    listB.reserve(BUFFER_LIMIT + 10);
    listC.reserve(BUFFER_LIMIT * 2 + 10);

    lastBatteryLevel = readBatteryPercentage(); 
    
    #ifdef DEBUG_MODE
    printf("Setup Complete. Sampling at 833Hz. Buffer size: %d\n", BUFFER_LIMIT);
    #endif
}

void loop() {
    // 833Hz = 1200 microseconds
    static unsigned long lastMicros = 0;
    unsigned long currentMicros = micros();
    unsigned long currentTime = millis();

    handleLEDStatus(); 

    // --- Handle button press ---
    if (digitalRead(BUTTON_PIN) == LOW) {

        if (!isButtonPressed) {
            isButtonPressed = true;
            buttonPressTime = currentTime;
        }

        unsigned long pressDuration = currentTime - buttonPressTime;

        if (pressDuration >= LONG_PRESS_TIME) {
            #ifdef DEBUG_MODE
            printf("Long press detected. Deep Sleep...\n");
            #endif
            while (digitalRead(BUTTON_PIN) == LOW) delay(10);
            digitalWrite(LED_PIN, LOW);

            // Deep sleep setup
            pinMode(BUTTON_PIN, INPUT_PULLUP);
            gpio_wakeup_enable((gpio_num_t)BUTTON_PIN, GPIO_INTR_LOW_LEVEL);
            esp_deep_sleep_enable_gpio_wakeup(1ULL << BUTTON_PIN, ESP_GPIO_WAKEUP_GPIO_LOW);
            gpio_hold_dis((gpio_num_t)BUTTON_PIN);
            esp_deep_sleep_start();

        } else if (pressDuration >= SHORT_PRESS_TIME && bleManager.isAdvertising()) {
            bleManager.disconnect();
        }
    } else if (isButtonPressed) {
        // Button released logic
        if ((currentTime - buttonPressTime) < LONG_PRESS_TIME) {
             bleManager.startAdvertising();
        }
        isButtonPressed = false;
    }

    // --- High Speed Sampling Loop (833Hz) ---
    if (currentMicros - lastMicros >= 1200) {
        lastMicros = currentMicros;

        DataPoint dp;
        for (int i = 0; i < 3; i++) {
            myIMU.readRegisterInt16(&dp.accel[i], LSM6DS3_ACC_GYRO_OUTX_L_XL + 2 * i);
            myIMU.readRegisterInt16(&dp.gyro[i], LSM6DS3_ACC_GYRO_OUTX_L_G + 2 * i);
        }

        if (isThresholdDetected) {
            // --- PHASE 2: Post-Trigger Recording ---
            listB.push_back(dp);
            
            // Wait for 400 samples (approx 0.5 seconds)
            if (listB.size() >= BUFFER_LIMIT) 
            {    
                #ifdef DEBUG_MODE
                printf("Captured Impact! Sending %d samples...\n", listA.size() + listB.size());
                #endif

                // Combine buffers
                listC = listA;
                listC.insert(listC.end(), listB.begin(), listB.end());
                
                // Send via BLE
                bleManager.sendSensorData(listC);
                
                // Reset
                isThresholdDetected = false;
                listA.clear();
                listB.clear();
                listC.clear();
                
                // Re-reserve memory just in case
                listA.reserve(BUFFER_LIMIT + 10);
                listB.reserve(BUFFER_LIMIT + 10);
            }
        } else {
            // --- PHASE 1: Pre-Trigger Monitoring ---
            
            // Maintain circular buffer size
            if (listA.size() >= BUFFER_LIMIT){
              // --- YOUR THRESHOLD LOGIC (Preserved) ---
              // Fixed '|' to '||' for correct C++ logical OR
              
              if(dp.accel[0] < -1000 || dp.accel[0] > 1000 ){
                if(dp.accel[1] > 6000 || dp.accel[1] < -6000){
                  if(dp.accel[2] > 150 || dp.accel[2] < -150){
                    isThresholdDetected = true;
                    #ifdef DEBUG_MODE
                    printf(">> Threshold Triggered!\n");
                    #endif
                  }
                }
              } else if (dp.accel[0] > 1000){
                if(dp.accel[1] < -6000){
                  if(dp.accel[2] < -150){
                    isThresholdDetected = true;
                    #ifdef DEBUG_MODE
                    printf(">> Threshold Triggered (Cond 2)!\n");
                    #endif
                  }
                }
              }
              
              // Remove oldest sample to keep buffer rolling
              listA.erase(listA.begin());
            }
            
            listA.push_back(dp);
        }
    }

    // --- Check battery ---
    uint8_t newLevel = readBatteryPercentage();
    if (lastBatteryLevel == 255 || abs((int)newLevel - (int)lastBatteryLevel) >= 1) {
      bleManager.sendBattery(newLevel);
      lastBatteryLevel = newLevel;
    }
}