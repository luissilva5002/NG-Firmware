#pragma once
#include <Arduino.h>

enum class ButtonEvent {
    NONE,
    HOLD_3S,
    HOLD_5S,
    RELEASE
};

class ButtonManager {
public:
    void begin(uint8_t pin);
    ButtonEvent update(); // Call every loop

private:
    uint8_t pin;
    bool wasPressed = false;
    unsigned long pressStartTime = 0;
    bool fired3s = false;
    bool fired5s = false;
};
