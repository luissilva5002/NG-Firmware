#include "app/device_controller.h"

DeviceController::DeviceController() : currentState(DeviceState::ADVERTISING) {
    listA.reserve(BUFFER_LIMIT + 10);
    listB.reserve(BUFFER_LIMIT + 10);
    listC.reserve(BUFFER_LIMIT * 2 + 10);
}

void DeviceController::setup() {
    delay(1000); 

    // 1. Hardware Init
    // Initialize Managers
    ledManager.begin(LED_PIN);
    buttonManager.begin(BUTTON_PIN);

    // Disable hold on button (important for deep sleep wake)
    gpio_hold_dis((gpio_num_t)BUTTON_PIN);

    // 2. I2C & IMU
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    powerManager.init(Wire);
    imuManager.init();

    // 3. Power Saving
    setCpuFrequencyMhz(80);
    esp_wifi_stop(); 

    // 4. BLE Setup
    bleManager.init(); // Starts advertising automatically
    
    // 5. Initial State
    lastBatteryLevel = powerManager.getBatteryPercentage();
    currentState = DeviceState::ADVERTISING;

    printf("Setup Complete. State: ADVERTISING\n");
}

void DeviceController::loop() {
    ButtonEvent btnEvent = buttonManager.update();
    
    // --- BUTTON 3S LOGIC ---
    if (btnEvent == ButtonEvent::HOLD_3S) {
        printf("Button: 3s Hold -> Force ADVERTISING\n");
        
        if (bleManager.isConnected()) {
            bleManager.disconnect(); 
            
            // 🛑 CRITICAL FIX: Wait for the disconnect to actually happen.
            // We loop here until isConnected() returns false.
            unsigned long startWait = millis();
            while (bleManager.isConnected() && (millis() - startWait < 500)) {
                delay(10); // Give the BLE stack CPU time to process the event
            }
            
            if (!bleManager.isConnected()) {
                printf("Disconnect Successful.\n");
            } else {
                printf("Disconnect Timed Out!\n");
            }
        }
        
        // Now it is safe to change state
        bleManager.setSamplingEnabled(false);
        currentState = DeviceState::ADVERTISING;
        return; // Restart loop to prevent immediate state checks
    }
    
    else if (btnEvent == ButtonEvent::HOLD_5S) {
        printf("Button: 5s Hold -> SLEEP\n");
        currentState = DeviceState::SLEEP;
    }

    // 3. State Machine Logic
    switch (currentState) {
        case DeviceState::ADVERTISING:
            // Logic: Wait for connection
            if (bleManager.isConnected()) {
                printf("State -> CONNECTED\n");
                currentState = DeviceState::CONNECTED;
            }
            break;

        case DeviceState::CONNECTED:
            // Logic: Wait for START command or Disconnect
            if (!bleManager.isConnected()) {
                printf("Disconnected -> ADVERTISING\n");
                bleManager.startAdvertising();
                currentState = DeviceState::ADVERTISING;
            } 
            else if (bleManager.isSamplingEnabled()) { // Flag set by "START" cmd
                printf("Cmd START -> SAMPLING\n");
                currentState = DeviceState::SAMPLING;
            }
            break;

        case DeviceState::SAMPLING:
            // Logic: Read Sensors
            if (!bleManager.isConnected()) {
                currentState = DeviceState::ADVERTISING;
            } 
            else if (!bleManager.isSamplingEnabled()) { // Flag cleared by "STOP" cmd
                printf("Cmd STOP -> CONNECTED\n");
                currentState = DeviceState::CONNECTED;
            } 
            else {
                handleSampling();
            }
            break;

        case DeviceState::SLEEP:
            // Logic: Enter Deep Sleep
            ledManager.render(DeviceState::SLEEP); // Ensure LED is off immediately
            enterDeepSleep(); 
            break;
    }

    // 4. Update LED based on final state
    ledManager.render(currentState);
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
    // Note: Battery check removed for brevity, add back if needed

    unsigned long currentMicros = micros();
    if (currentMicros - lastMicros >= 1200) { // ~833Hz
        lastMicros = currentMicros;

        DataPoint dp;
        imuManager.readData(dp);

        if (isThresholdDetected) {
            // --- POST-TRIGGER RECORDING ---
            listB.push_back(dp);
            
            if (listB.size() >= BUFFER_LIMIT) {
                printf("Buffer Full. Sending Data...\n");
                
                // Combine Pre + Post
                listC = listA; 
                listC.insert(listC.end(), listB.begin(), listB.end());

                bleManager.sendSensorData(listC);

                // Reset Buffers
                isThresholdDetected = false;
                listA.clear(); 
                listB.clear(); 
                listC.clear();
                // Re-reserve to avoid reallocations
                listA.reserve(BUFFER_LIMIT); 
                listB.reserve(BUFFER_LIMIT);
            }
        } else {
            // --- PRE-TRIGGER BUFFERING ---
            if (listA.size() >= BUFFER_LIMIT) {
                // Check threshold only when buffer is full (sliding window)
                if (checkThreshold(dp)) {
                    isThresholdDetected = true;
                    printf("Threshold Triggered!\n");
                }
                listA.erase(listA.begin()); // Remove oldest
            }
            listA.push_back(dp);
        }
    }
}