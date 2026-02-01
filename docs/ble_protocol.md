# NG Smasher BLE Protocol Specification

## Overview

The NG Smasher communicates with the mobile app via BLE.

BLE is used for:

- Device identification
- Battery status reporting
- Hit burst transmission
- OTA firmware updates

All packets are versioned for long-term compatibility.

---

## BLE Roles

- Device = Peripheral
- Mobile App = Central

---

## Advertising

Advertising includes:

- Device name: `NG-SMASHER`
- Service UUID: Custom NG Service
- Device ID (optional manufacturer data)

---

## GATT Services

### NG Main Service

| Characteristic | UUID | Properties |
|--------------|------|------------|
| Control | XXX1 | Write |
| Status | XXX2 | Notify |
| Burst Data | XXX3 | Notify |
| Battery | XXX4 | Notify |
| OTA | XXX5 | Write/Notify |

---

## Protocol Versioning

The device always reports:

- Firmware version
- Protocol version

Example:

```json
{
  "fw": "1.2.0",
  "proto": 3
}
```

The app must refuse incompatible protocol versions.

Session Handshake (Required)

Before streaming is enabled:

Step 1: Device Hello

Device notifies:

Device ID

Protocol version

Firmware version

Step 2: App Challenge

App sends:

Random nonce (32-bit)

Step 3: Device Response

Device replies:

HMAC(DeviceKey, nonce)

Step 4: Session Authenticated

If valid:

Device enters STREAM_READY

Otherwise:

Disconnect immediately

Hit Burst Transmission

When a hit is detected:

800 samples are captured

Each sample contains:

ax, ay, az, gx, gy, gz (int16)


Total burst size:

800 × 6 × 2 bytes = 9600 bytes

Packet Framing

All packets use a common header:

Field	Size
Packet Type	1 byte
Sequence ID	2 bytes
Payload Length	2 bytes
CRC16	2 bytes
Packet Types
Type	Name
0x01	Battery Update
0x02	Hit Burst Start
0x03	Hit Burst Chunk
0x04	Hit Burst End
0x05	OTA Begin
0x06	OTA Chunk
0x07	OTA Complete
Reliability Rules

Burst chunks must be sequential

App requests retransmission if missing

CRC required for integrity

Disconnect resets sequence counters

Connection Parameters

Recommended:

MTU: 247

Connection interval: 30–80 ms

Slave latency: enabled

Notifications in burst mode only

Security Requirements

BLE bonding required

Secure Connections pairing required

No plaintext OTA allowed