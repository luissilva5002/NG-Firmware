#include <Arduino.h>
#include "app/device_controller.h"

DeviceController* deviceController;

void setup() {
    // Note: Serial.begin is usually called inside deviceController->setup() if needed 
    // or implicitly by printf if platformio is configured, 
    // but explicit call is good for debugging.
    // Serial.begin(115200); 

    deviceController = new DeviceController();
    deviceController->setup();
}

void loop() {
    deviceController->loop();
}