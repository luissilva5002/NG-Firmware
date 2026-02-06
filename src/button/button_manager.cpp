#include "button/button_manager.h"

void ButtonManager::begin(uint8_t p) {
    pin = p;
    pinMode(pin, INPUT_PULLUP);
}

ButtonEvent ButtonManager::update() {
    bool pressed = digitalRead(pin) == LOW;
    unsigned long now = millis();
    ButtonEvent evt = ButtonEvent::NONE;

    if (pressed && !wasPressed) {
        pressStartTime = now;
        fired3s = false;
        fired5s = false;
    }

    if (pressed) {
        unsigned long held = now - pressStartTime;

        if (held >= 3000 && !fired3s) {
            fired3s = true;
            evt = ButtonEvent::HOLD_3S;
        }

        if (held >= 5000 && !fired5s) {
            fired5s = true;
            evt = ButtonEvent::HOLD_5S;
        }
    }

    if (!pressed && wasPressed) {
        evt = ButtonEvent::RELEASE;
    }

    wasPressed = pressed;
    return evt;
}
