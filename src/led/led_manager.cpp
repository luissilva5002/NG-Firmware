#include "led/led_manager.h"

void LEDManager::begin(uint8_t p) {
    pin = p;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

void LEDManager::render(DeviceState state) {
    unsigned long currentTime = millis();

    switch(state) {
        case DeviceState::ADVERTISING:
            if (currentTime - lastBlinkTime >= LED_BLINK_INTERVAL) {
                lastBlinkTime = currentTime;
                ledState = !ledState;
                digitalWrite(pin, ledState);
            }
            break;

        case DeviceState::CONNECTED:
        case DeviceState::SAMPLING:
            digitalWrite(pin, HIGH);
            break;

        case DeviceState::SLEEP:
            digitalWrite(pin, LOW);
            break;
    }
}
