#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <MAX17048.h> // Ensure your library is included

class PowerManager {
public:
    void init(TwoWire &wirePort);
    uint8_t getBatteryPercentage();
private:
    MAX17048 fuelGauge;
};