#pragma once
#include "ble/ble_protocol.h"

class HitDetector {
public:
    // Returns true if this specific data point triggers a shot
    bool isTrigger(const DataPoint& dp);
};