# NG Smasher Manufacturing Firmware Notes

## Overview

This document defines firmware requirements for production and factory use.

---

## Production Build Types

Firmware must support:

- Development build (logging enabled)
- Release build (secure, optimized)
- Factory test build

---

## Factory Programming

Each device must be flashed with:

- Unique device ID
- Unique BLE key
- Firmware version tag

IDs must be recorded in manufacturing database.

---

## Hardware Self-Test

Factory firmware must verify:

- IMU responds on I2C/SPI
- Battery gauge responds
- BLE advertising works
- LEDs functional

---

## Serial Number Format

Example:

NGS-2026-000123


Stored in flash and advertised to app.

---

## OTA Readiness

Production devices must ship with:

- Bootloader installed
- OTA partition valid
- Secure public key burned in

---

## Compliance Notes

For CE/FCC:

- BLE radio settings must not exceed limits
- Firmware version must be traceable
- Logs must be disabled in release builds

---

## Required Documentation

Production release must include:

- Firmware version
- Protocol version
- Third-party license bundle
- OTA signing key policy

---

## End-of-Line Testing

Recommended:

- BLE connection test
- Burst packet integrity test
- Battery charge/discharge verification
