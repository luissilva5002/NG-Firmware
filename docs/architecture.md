# NG Smasher Firmware Architecture

## Overview

This firmware powers the NG Smasher tennis sensor device.  
Its primary responsibilities are:

- High-frequency IMU sampling (833 Hz)
- Hit detection and burst capture
- BLE transmission of recorded bursts
- Battery monitoring via fuel gauge
- Secure OTA firmware updates

The firmware is designed for:

- Low-power wearable operation
- Protocol stability across app versions
- Migration compatibility (ESP32 → nRF52)

---

## Architecture Layers

The firmware is structured into clear functional layers:

### Core Layer (`core/`)

Provides reusable infrastructure:

- Logging
- Error codes
- Ring buffers
- Device state machine

This layer contains no device-specific logic.

---

### Sensor Layer (`sensors/`)

Hardware abstraction layer:

- IMU sampling and buffering
- Battery fuel gauge readings

Responsibilities:

- Provide clean sensor APIs
- Hide driver-level complexity

---

### Application Layer (`app/`)

Device behavior logic:

- Hit detection algorithm
- Burst recording rules
- Device mode control

This layer does not implement BLE directly.

---

### BLE Transport Layer (`ble/`)

Handles communication:

- Advertising and connection
- GATT services
- Packet framing
- Secure handshake protocol

BLE is treated as a transport channel, not business logic.

---

### OTA Layer (`ota/`)

Responsible for firmware upgrades:

- Receiving firmware chunks
- Signature verification
- Version rollback prevention
- Safe boot partition switching

OTA logic must remain isolated for security.

---

## Device Runtime State Machine

The firmware operates using a defined state model:

| State | Description |
|------|------------|
| BOOT | Power-on initialization |
| IDLE | Low-power waiting mode |
| ARMED | IMU sampling active |
| HIT_CAPTURE | Threshold detected, burst stored |
| TRANSFER | Burst transmitted over BLE |
| OTA_MODE | Firmware update session active |
| ERROR | Fault recovery mode |

All transitions are explicit and logged.

---

## Design Principles

- Strict separation of concerns
- Minimal logic in `main.cpp`
- Protocol versioning for compatibility
- Security-first OTA implementation
- Battery-aware operation

---

## Future Migration

The architecture is designed to migrate cleanly from:

- ESP32-C3 + NimBLE  
to  
- Nordic nRF52 BLE stack

Only low-level drivers and BLE transport should change.
