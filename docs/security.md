# NG Smasher Security Model

## Overview

NG Smasher is a consumer BLE device with OTA capability.

Security goals:

- Prevent unauthorized connections
- Prevent firmware hijacking
- Protect user data integrity
- Ensure safe OTA upgrades

---

## Threat Model

Potential attacks:

- Random phone connects and streams data
- Firmware replaced with malicious image
- Replay of BLE burst packets
- Denial-of-service via connection spam

---

## BLE Security Requirements

Mandatory:

- Secure Connections pairing
- Bonding enabled
- MITM protection

Device rejects unpaired centrals.

---

## Application Authentication

BLE pairing alone is insufficient.

A session handshake is required:

- Nonce challenge
- HMAC response
- Device unique key

---

## Device Identity

Each device must have:

- Unique device ID
- Unique secret key (not shared)

Stored securely in flash.

---

## OTA Security Rules

OTA must enforce:

- Signed firmware only
- Version rollback prevention
- Authenticated session required

Unsigned firmware must never boot.

---

## Transport Integrity

All packets include:

- Sequence counter
- CRC16
- Protocol version

Prevents corruption and replay.

---

## Production Hardening

Recommended:

- Watchdog enabled
- Disable debug logs in release
- Lock flash readout protection
- Rate limit connection attempts

---

## Future NRF52 Enhancements

Nordic supports:

- Hardware crypto acceleration
- Trusted bootloader (MCUboot)
- Secure DFU pipelines
