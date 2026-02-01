#include "app/device_controller.h"
#include "config/device_config.h"

void DeviceController::setup() {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    Wire.setClock(I2C_CLOCK_SPEED);

    ble.init();
    ota.begin(ble.getServer());
    imu.init();
    
    postTriggerData.reserve(BUFFER_LIMIT);
}

void DeviceController::loop() {
    // 1. Check for OTA override
    if (ble.isOtaRequested()) {
        currentState = OTA;
        // OTA Logic handled by OtaManager callbacks mostly
        return; 
    }

    // 2. State Machine
    switch (currentState) {
        case IDLE:
            if (ble.isConnected() && ble.isSamplingEnabled()) {
                currentState = SAMPLING;
                ringBuffer.reset();
            }
            break;

        case SAMPLING:
            if (!ble.isSamplingEnabled()) {
                currentState = IDLE; 
                break;
            }
            handleSampling();
            break;
            
        case SENDING:
             // Logic to stitch RingBuffer + PostTriggerData
             std::vector<DataPoint> fullRecord;
             ringBuffer.unrollTo(fullRecord);
             fullRecord.insert(fullRecord.end(), postTriggerData.begin(), postTriggerData.end());
             
             ble.sendSensorData(fullRecord);
             
             // Reset
             postTriggerData.clear();
             currentState = SAMPLING; 
             break;
    }
}

void DeviceController::handleSampling() {
    // Simple non-blocking read rate check
    static unsigned long lastMicros = 0;
    if (micros() - lastMicros < 1200) return;
    lastMicros = micros();

    DataPoint dp = imu.read();

    if (postTriggerData.size() > 0) {
        // We are in "Post-Trigger" capture mode
        postTriggerData.push_back(dp);
        if (postTriggerData.size() >= BUFFER_LIMIT) {
            currentState = SENDING;
        }
    } else {
        // We are in "Pre-Trigger" mode
        ringBuffer.push(dp);
        if (ringBuffer.isFull() && detector.isTrigger(dp)) {
             // Shot detected! Start filling the Post-Trigger vector
             postTriggerData.push_back(dp); 
             printf("TRIGGERED!\n");
        }
    }
}