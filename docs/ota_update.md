# NG Smasher OTA Update System

## Overview

NG Smasher supports OTA firmware updates over BLE.

OTA is considered a critical security surface.

The device must never install unsigned firmware.

---

## OTA Requirements

OTA must provide:

- Authenticity (signed firmware)
- Integrity (CRC + signature)
- Anti-rollback protection
- Safe recovery on failure

---

## OTA Workflow

### 1. Enter OTA Mode

App sends `OTA_BEGIN`

Device responds:

- Current firmware version
- Expected image size

Device enters OTA_MODE state.

---

### 2. Firmware Transfer

Firmware is transferred in chunks:

- Chunk size: 200 bytes typical
- Sequence numbered
- CRC checked per chunk

Device writes chunks to update partition.

---

### 3. Signature Verification

After transfer:

Device verifies:

- ECDSA signature
- Trusted public key stored in flash

If invalid → reject immediately.

---

### 4. Version Check

Device rejects firmware if:

- Version <= current version

Prevents rollback attacks.

---

### 5. Commit Update

If valid:

- Mark new partition bootable
- Reboot device

---

## Failure Recovery

If update fails:

- Device remains on previous firmware
- OTA partition is erased
- App may retry

Device must never brick.

---

## Production Security

OTA must enforce:

- Signed firmware only
- Session authentication before OTA
- Rate limiting (DoS protection)

---

## Recommended Tools

- MCUboot (Nordic)
- ESP-IDF Secure Boot (ESP32)
- ECDSA signing pipeline in CI