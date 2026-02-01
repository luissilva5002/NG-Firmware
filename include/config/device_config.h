#pragma once

// Pin Definitions
#define PIN_BUTTON      0
#define PIN_LED         3
#define PIN_I2C_SDA     4
#define PIN_I2C_SCL     5

// Hardware Settings
#define I2C_CLOCK_SPEED 400000
#define SERIAL_BAUD     115200

// Logic Constants
#define SHORT_PRESS_TIME 2000
#define LONG_PRESS_TIME  5000
#define BUFFER_LIMIT     400 
#define LED_BLINK_INTERVAL 500