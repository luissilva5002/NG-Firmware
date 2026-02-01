#pragma once
#include "sensors/imu_manager.h"
#include "ble/ble_manager.h"
#include "ota/ota_manager.h"
#include "core/ring_buffer.h"
#include "app/hit_detector.h"

enum DeviceState { IDLE, SAMPLING, CAPTURING, SENDING, OTA };

class DeviceController {
public:
    void setup();
    void loop();

private:
    ImuManager imu;
    BleManager ble;
    OtaManager ota;
    RingBuffer ringBuffer;
    HitDetector detector;

    DeviceState currentState = IDLE;
    std::vector<DataPoint> postTriggerData;
    
    void handleSampling();
    void handleButtons();
};