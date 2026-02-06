#include "app/device_controller.h"

DeviceController::DeviceController() {
    listA.reserve(BUFFER_LIMIT + 10);
    listB.reserve(BUFFER_LIMIT + 10);
    listC.reserve(BUFFER_LIMIT * 2 + 10);
}

void DeviceController::setup() {
    delay(1000); 

    // 1. GPIO Setup
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    gpio_hold_dis((gpio_num_t)BUTTON_PIN);

    // 2. IMU Setup & I2C Init
    // Note: IMUManager::init() now handles Wire.begin(SDA, SCL)
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    powerManager.init(Wire);
    imuManager.init();

    // 4. Deep Sleep & WiFi Config
    gpio_wakeup_enable((gpio_num_t)BUTTON_PIN, GPIO_INTR_LOW_LEVEL);
    esp_deep_sleep_enable_gpio_wakeup(1ULL << BUTTON_PIN, ESP_GPIO_WAKEUP_GPIO_LOW);
    setCpuFrequencyMhz(80);
    esp_wifi_stop(); 

    // 5. BLE Setup
    bleManager.init();
    
    // 6. Initial State
    lastBatteryLevel = powerManager.getBatteryPercentage();

    printf("Setup Complete.\n");
}

void DeviceController::loop() {
    handleLED();
    handleButton();
    handleSampling();
}

void DeviceController::handleLED() {
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
    }
}

void DeviceController::handleButton() {
    if (digitalRead(BUTTON_PIN) == LOW) {
        if (!isButtonPressed) {
            isButtonPressed = true;
            buttonPressTime = millis();
        }
        unsigned long pressDuration = millis() - buttonPressTime;
        
        if (pressDuration >= LONG_PRESS_TIME) {
            enterDeepSleep();
        } else if (pressDuration >= SHORT_PRESS_TIME && bleManager.isAdvertising()) {
            bleManager.disconnect();
        }
    } else if (isButtonPressed) {
        // Button released
        if ((millis() - buttonPressTime) < LONG_PRESS_TIME) {
             if (!bleManager.isConnected() && !bleManager.isAdvertising()) {
                 bleManager.startAdvertising();
             }
        }
        isButtonPressed = false;
    }
}

void DeviceController::enterDeepSleep() {
    printf("Deep Sleep...\n");
    while (digitalRead(BUTTON_PIN) == LOW) delay(10); // Wait for release
    
    digitalWrite(LED_PIN, LOW);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    gpio_wakeup_enable((gpio_num_t)BUTTON_PIN, GPIO_INTR_LOW_LEVEL);
    esp_deep_sleep_enable_gpio_wakeup(1ULL << BUTTON_PIN, ESP_GPIO_WAKEUP_GPIO_LOW);
    
    // Disable hold to allow state change during sleep if needed, usually fine
    gpio_hold_dis((gpio_num_t)BUTTON_PIN);
    
    esp_deep_sleep_start();
}

bool DeviceController::checkThreshold(const DataPoint& dp) {
    return (dp.accel[0] < TRIG_ACC_X_MIN || dp.accel[0] > TRIG_ACC_X_MAX) && 
           (dp.accel[1] > TRIG_ACC_Y_POS || dp.accel[1] < TRIG_ACC_Y_NEG);
}

void DeviceController::handleSampling() {
    if (!bleManager.isConnected()) return;

    // --- Battery Logic ---
    static unsigned long lastBatCheck = 0;
    if (millis() - lastBatCheck > 5000) {
        lastBatCheck = millis();
        uint8_t newLevel = powerManager.getBatteryPercentage();
        if (lastBatteryLevel == 255 || abs((int)newLevel - (int)lastBatteryLevel) >= 1) {
            bleManager.sendBattery(newLevel);
            lastBatteryLevel = newLevel;
        }
    }

    // --- Sampling Logic ---
    if (bleManager.isSamplingEnabled()) {
        unsigned long currentMicros = micros();
        if (currentMicros - lastMicros >= 1200) {
            lastMicros = currentMicros;

            DataPoint dp;
            imuManager.readData(dp);

            if (isThresholdDetected) {
                // Recording POST-trigger data
                listB.push_back(dp);
                
                if (listB.size() >= BUFFER_LIMIT) {
                    printf("Captured! Sending...\n");
                    
                    listC = listA; // Pre-trigger
                    listC.insert(listC.end(), listB.begin(), listB.end()); // Post-trigger

                    bleManager.sendSensorData(listC);

                    isThresholdDetected = false;
                    listA.clear(); listB.clear(); listC.clear();
                    listA.reserve(BUFFER_LIMIT); listB.reserve(BUFFER_LIMIT);
                }
            } else {
                // Filling PRE-trigger buffer
                if (listA.size() >= BUFFER_LIMIT) {
                    if (checkThreshold(dp)) {
                        isThresholdDetected = true;
                        printf("Triggered!\n");
                    }
                    listA.erase(listA.begin());
                }
                listA.push_back(dp);
            }
        }
    } else {
        // Idle cleanup
        if (!listA.empty()) listA.clear();
        isThresholdDetected = false;
    }
}