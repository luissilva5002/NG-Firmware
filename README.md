
```bash
ng-smasher-firmware/
│
├── README.md
├── LICENSE
├── THIRD_PARTY_NOTICES.md
├── CHANGELOG.md
├── VERSION
│
├── docs/
│   ├── architecture.md
│   ├── ble_protocol.md
│   ├── ota_update.md
│   ├── security.md
│   ├── power_profile.md
│   ├── manufacturing.md
│   └── certifications/
│       ├── ce/
│       │   ├── README.md
│       │   ├── rf_notes.md
│       │   ├── test_plan.md
│       │   └── compliance_assets/
│       ├── fcc/
│       │   ├── README.md
│       │   └── test_plan.md
│       └── rohs_reach/
│           └── README.md
│
├── licenses/
│   ├── Apache-2.0.txt
│   ├── MIT.txt
│   ├── NimBLE_LICENSE.txt
│   ├── SparkFun_LSM6DS3_LICENSE.txt
│   └── MAX170xx_LICENSE.txt
│
├── lib/                      # Vendored third-party libraries (frozen)
│   ├── NimBLE/
│   ├── LSM6DS3TR/
│   └── MAXFuelGauge/
│
├── include/                  # Public project headers
│   ├── config/
│   │   ├── device_config.h
│   │   ├── ble_config.h
│   │   ├── imu_config.h
│   │   ├── power_config.h
│   │   └── ota_config.h
│   │
│   ├── core/
│   │   ├── logger.h
│   │   ├── ring_buffer.h
│   │   ├── error_codes.h
│   │   └── state_machine.h
│   │
│   ├── ble/
│   │   ├── ble_manager.h
│   │   ├── ble_protocol.h
│   │   ├── ble_security.h
│   │   └── ble_packets.h
│   │
│   ├── sensors/
│   │   ├── imu_manager.h
│   │   └── battery_manager.h
│   │
│   ├── ota/
│   │   ├── ota_manager.h
│   │   └── firmware_version.h
│   │
│   └── app/
│       ├── hit_detector.h
│       ├── data_recorder.h
│       └── device_controller.h
│
├── src/                      # Implementation
│   ├── core/
│   │   ├── logger.cpp
│   │   ├── state_machine.cpp
│   │   └── error_codes.cpp
│   │
│   ├── ble/
│   │   ├── ble_manager.cpp
│   │   ├── ble_protocol.cpp
│   │   ├── ble_security.cpp
│   │   └── ble_packets.cpp
│   │
│   ├── sensors/
│   │   ├── imu_manager.cpp
│   │   └── battery_manager.cpp
│   │
│   ├── ota/
│   │   ├── ota_manager.cpp
│   │   └── firmware_version.cpp
│   │
│   ├── app/
│   │   ├── hit_detector.cpp
│   │   ├── data_recorder.cpp
│   │   └── device_controller.cpp
│   │
│   └── main.cpp
│
├── tests/
│   ├── test_ble_protocol.cpp
│   ├── test_hit_detector.cpp
│   └── test_ring_buffer.cpp
│
├── tools/
│   ├── packet_decoder.py
│   ├── firmware_sign.py
│   └── ble_debug_cli.py
│
└── platformio.ini (or CMakeLists.txt)
```