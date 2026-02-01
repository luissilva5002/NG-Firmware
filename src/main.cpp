#include <Arduino.h>
#include "app/device_controller.h"

DeviceController smasher;

void setup() {
    Serial.begin(115200);
    smasher.setup();
}

void loop() {
    smasher.loop();
}