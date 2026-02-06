#pragma once
#include <Arduino.h>
#include "shared_types.h"
#include "config/device_config.h"

class LEDManager {
public:
    void begin(uint8_t pin = LED_PIN);
    void render(DeviceState state);

private:
    uint8_t pin;
    unsigned long lastBlinkTime = 0;
    int ledState = LOW;
};
