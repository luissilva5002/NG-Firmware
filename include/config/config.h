#pragma once

// --- Hardware Pins ---
#define BUTTON_PIN 0
#define LED_PIN 3 
#define I2C_SDA 4
#define I2C_SCL 5

// --- Timing & Logic ---
#define SHORT_PRESS_TIME 2000
#define LONG_PRESS_TIME 5000
#define LED_BLINK_INTERVAL 500
#define BUFFER_LIMIT 400 

// --- Thresholds (Accel Raw) ---
#define TRIG_ACC_X_MIN -1000
#define TRIG_ACC_X_MAX 1000
#define TRIG_ACC_Y_POS 6000
#define TRIG_ACC_Y_NEG -6000
#define TRIG_ACC_Z_POS 150
#define TRIG_ACC_Z_NEG -150