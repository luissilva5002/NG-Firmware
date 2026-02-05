#include "power/power_manager.h"

void PowerManager::init(TwoWire &wirePort) {
    // MAX17048 typically attaches to the wire instance
    fuelGauge.attach(wirePort);
}

uint8_t PowerManager::getBatteryPercentage() {
    return fuelGauge.percent();
}