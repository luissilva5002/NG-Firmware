#include "app/hit_detector.h"
#include <math.h> 

bool HitDetector::isTrigger(const DataPoint& dp) {
    // Your specific logic
    bool xTrigger = (dp.accel[0] < -1000 || dp.accel[0] > 1000);
    bool yTrigger = (dp.accel[1] > 6000 || dp.accel[1] < -6000);
    bool zTrigger = (dp.accel[2] > 150 || dp.accel[2] < -150);
    
    return (xTrigger && yTrigger && zTrigger);
}