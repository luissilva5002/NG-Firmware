#include <Arduino.h>
#include "SparkFunLSM6DS3.h"
#include "Wire.h"
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLE2902.h"
#include "esp_wifi.h"
#include "esp_sleep.h"

#define BUTTON_PIN 13
#define DEBOUNCE_TIME 50
#define SHORT_PRESS_TIME 2000
#define LONG_PRESS_TIME 5000

#define I2C_SDA 21
#define I2C_SCL 22
#define MAX17048_ADDR 0x36

#define DEBUG_MODE

BLECharacteristic *pCharacteristic;
BLEServer *pServer;
BLEService *pService;
LSM6DS3Core myIMU(I2C_MODE, 0x6A);

bool isAdvertising = true;
bool isButtonPressed = false;
unsigned long buttonPressTime = 0;

struct DataPoint {
    int16_t accel[3];
    int16_t gyro[3];
};

std::vector<DataPoint> listA, listB, listC;
bool isThresholdDetected = false;

uint8_t lastBatteryLevel = 0;

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) override {
        #ifdef DEBUG_MODE
        Serial.println("Device connected");
        #endif
    }

    void onDisconnect(BLEServer* pServer) override {
        #ifdef DEBUG_MODE
        Serial.println("Device disconnected");
        #endif
        pServer->startAdvertising();
    }
};

void initI2C() {
    Wire.begin(I2C_SDA, I2C_SCL);
}

uint8_t readBatteryPercentage() {
    Wire.beginTransmission(MAX17048_ADDR);
    Wire.write(0x04);
    Wire.endTransmission();
    Wire.requestFrom(MAX17048_ADDR, 2);
    uint16_t soc = (Wire.read() << 8) | Wire.read();
    return soc / 256;
}

void sendBatteryPercentage(uint8_t batteryLevel) {
    pCharacteristic->setValue(&batteryLevel, sizeof(batteryLevel));
    pCharacteristic->notify();
}

void sendBleData(std::vector<DataPoint>& data) {
    while (!data.empty()) {
        size_t batchSize = std::min(data.size(), size_t(20));
        std::vector<int16_t> batch;

        for (size_t i = 0; i < batchSize; i++) {
            batch.insert(batch.end(), std::begin(data[i].accel), std::end(data[i].accel));
            batch.insert(batch.end(), std::begin(data[i].gyro), std::end(data[i].gyro));
        }

        pCharacteristic->setValue((uint8_t*)batch.data(), batch.size() * sizeof(int16_t));
        pCharacteristic->notify();
        data.erase(data.begin(), data.begin() + batchSize);
    }
}

void setup() {
    Serial.begin(115200);
    initI2C();

    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
        #ifdef DEBUG_MODE
        Serial.println("Woke up from deep sleep.");
        #endif
    }

    setCpuFrequencyMhz(80);
    esp_wifi_stop();
    btStop();

    BLEDevice::init("ESP32_BT");
    BLEDevice::setMTU(256);
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    pService = pServer->createService("A8922A2A-6FA5-4C36-83A2-8016AEB7865B");
    pCharacteristic = pService->createCharacteristic(
        "9CCCEF3B-6AAB-43EA-B9E6-F7D5B461792E",
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->setValue("Hello World");
    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(pService->getUUID());
    pAdvertising->start();

    pinMode(BUTTON_PIN, INPUT_PULLUP);

    delay(1000);

    if (myIMU.beginCore() != 0) {
        #ifdef DEBUG_MODE
        Serial.println(F("Error at beginCore()."));
        #endif
    } else {
        #ifdef DEBUG_MODE
        Serial.println(F("beginCore() passed."));
        #endif
    }

    myIMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL1_XL,
        LSM6DS3_ACC_GYRO_BW_XL_100Hz |
        LSM6DS3_ACC_GYRO_FS_XL_16g | //-> max value of acceleration
        LSM6DS3_ACC_GYRO_ODR_XL_104Hz);

    myIMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL2_G,
        LSM6DS3_ACC_GYRO_FS_G_2000dps |
        LSM6DS3_ACC_GYRO_ODR_G_104Hz);

    uint8_t lastBatteryLevel = readBatteryPercentage();    
}

void loop() {
    static unsigned long lastSensorTime = 0;
    unsigned long currentTime = millis();

    // Handle button press
    if (digitalRead(BUTTON_PIN) == LOW) {
        if (!isButtonPressed) {
            isButtonPressed = true;
            buttonPressTime = currentTime;
        }

        unsigned long pressDuration = currentTime - buttonPressTime;
        if (pressDuration >= LONG_PRESS_TIME) {
            #ifdef DEBUG_MODE
            Serial.println("Entering Deep Sleep...");
            #endif
            esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, LOW);
            esp_deep_sleep_start();
        } else if (pressDuration >= SHORT_PRESS_TIME && isAdvertising) {
            #ifdef DEBUG_MODE
            Serial.println("Disconnecting BLE and stopping advertising...");
            #endif
            pServer->disconnect(pServer->getConnectedCount());
            isAdvertising = false;
        }
    } else if (isButtonPressed) {
        unsigned long pressDuration = currentTime - buttonPressTime;
        if (pressDuration < LONG_PRESS_TIME) {
            #ifdef DEBUG_MODE
            Serial.println("Restarting BLE advertising...");
            #endif
            BLEDevice::startAdvertising();
            isAdvertising = true;
        }
        isButtonPressed = false;
    }

    // Sensor data processing every 10ms
    if (currentTime - lastSensorTime >= 10) {
        lastSensorTime = currentTime;

        DataPoint dp;
        for (int i = 0; i < 3; i++) {
            myIMU.readRegisterInt16(&dp.accel[i], LSM6DS3_ACC_GYRO_OUTX_L_XL + 2 * i);
            myIMU.readRegisterInt16(&dp.gyro[i], LSM6DS3_ACC_GYRO_OUTX_L_G + 2 * i);
        }

        if (isThresholdDetected) {
            listB.push_back(dp);
            if (listB.size() == 50) {
                listC = listA;
                listC.insert(listC.end(), listB.begin(), listB.end());
                isThresholdDetected = false;
                listA.clear();
                listB.clear();
                sendBleData(listC);
                listC.clear();
            }
        } else {
            if (listA.size() == 50){
              if(dp.accel[0] < -1000 | dp.accel[0] > 1000 ){
                if(dp.accel[1] > 6000 | dp.accel[1] < -6000){
                  if(dp.accel[2] > 150 | dp.accel[2] < -150){
                    isThresholdDetected = true;
                  }
                }
              //ToDo: Not accurate  
              } else if (dp.accel[0] > 1000){
                if(dp.accel[1] < -6000){
                  if(dp.accel[2] < -150){
                    isThresholdDetected = true;
                  }
                }
              }
              listA.erase(listA.begin());
              
            }
            listA.push_back(dp);

            #ifdef DEBUG_MODE
            Serial.print(dp.accel[0]);
            Serial.print(", ");
            Serial.print(dp.accel[1]);
            Serial.print(", ");
            Serial.println(dp.accel[2]);
            #endif

            /*
            if (listA.size() == 50 
              && (dp.accel[0] < -1000 | dp.accel[0] > 1000)
              && (dp.accel[1] < -6000 | dp.accel[0] > 6000)
              && (dp.accel[2] < -150 | dp.accel[2] > 150)) {
                isThresholdDetected = true;
            }
            */            
        }
    }

    // Check battery with debounce
    uint8_t newLevel = readBatteryPercentage();
    if (lastBatteryLevel == 255 || abs(newLevel - lastBatteryLevel) >= 1) {
      sendBatteryPercentage(newLevel);
      lastBatteryLevel = newLevel;
    }
}





