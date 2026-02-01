# NG Smasher Power Profile

## Overview

NG Smasher is optimized for wearable battery operation.

Main power consumers:

- IMU sampling
- BLE radio bursts
- CPU processing
- LEDs

---

## Sensor Operation

IMU sampling:

- Frequency: 833 Hz
- Mode: Continuous
- Typical IMU draw: ~1 mA

---

## Burst Transmission Model

Hit event every ~2 seconds:

- 800 samples transmitted
- Total burst size: ~9600 bytes

BLE throughput optimized via:

- High MTU
- Burst notifications
- Idle connection intervals

---

## Expected Consumption

| Platform | Avg Current |
|---------|------------|
| ESP32 BLE (Bluedroid) | ~117 mA |
| ESP32 NimBLE | ~95–105 mA |
| NRF52 Optimized | ~5–15 mA |

---

## Power Optimizations

Required:

- Disable fixed LED or reduce brightness
- Increase BLE connection interval outside bursts
- Sleep CPU between sampling windows
- Use interrupt-based hit detection (future)

---

## Battery Reporting

Fuel gauge sampling overhead:

- <0.1 mA
- Negligible impact

Battery notifications sent only on 1% change.

---

## Production Target

Goal runtime:

- ESP32: Prototype only
- NRF52: Multi-day wearable operation
