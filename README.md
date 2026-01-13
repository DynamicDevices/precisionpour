# PrecisionPour ESP32 Firmware

Firmware for ESP32-based touchscreen display system with flow meter monitoring, WiFi connectivity, and MQTT communication for the PrecisionPour beverage dispensing system.

**Copyright (c) 2026 Dynamic Devices Ltd. All rights reserved.**

This software is proprietary and confidential. See [LICENSE](LICENSE) for details.

## Quick Start

1. **Install PlatformIO**: Install VS Code and the PlatformIO IDE extension
2. **Configure**: Use `pio run -e esp32dev-idf -t menuconfig` to configure settings
3. **Build & Upload**: `pio run -t upload`
4. **Monitor**: `pio device monitor`

## Configuration

### KConfig (Recommended)

```bash
pio run -e esp32dev-idf -t menuconfig
```

Navigate to **PrecisionPour Configuration** to configure:
- Hardware pins (display, touch, flow meter)
- WiFi credentials
- MQTT server settings
- Operating mode (Production/Test)
- Debug options

### WiFi and MQTT

WiFi and MQTT credentials can be set via:
1. **KConfig** (highest priority)
2. **secrets.h** (fallback) - Copy `include/secrets.h.example` to `include/secrets.h`

## Hardware

### Display Module: 2.8" ESP32-32E
- **Display**: ILI9341 TFT (320x240 landscape)
- **Touch**: XPT2046 resistive touchscreen
- **Flow Meter**: YF-S201 Hall Effect sensor on GPIO26

See [docs/HARDWARE_SETUP.md](docs/HARDWARE_SETUP.md) for detailed pin configuration.

## Features

- **LVGL-based UI** with touch support
- **Flow meter integration** with real-time rate and volume tracking
- **WiFi connectivity** with auto-reconnect
- **MQTT communication** for cloud integration
- **Production mode** with QR code payment
- **Test mode** for hardware validation

## MQTT Protocol

The device subscribes to device-specific MQTT topics and responds to JSON commands.

See [docs/MQTT_PROTOCOL.md](docs/MQTT_PROTOCOL.md) for complete protocol documentation.

### "paid" Command Example

```json
{
  "id": "order_12345",
  "cost_per_ml": 0.005,
  "max_ml": 500,
  "currency": "GBP"
}
```

## Project Structure

```
precisionpour/
├── src/              # Source files
├── include/          # Header files
│   └── config.h      # Hardware configuration
├── platformio.ini    # PlatformIO configuration
└── docs/             # Documentation
```

## Debug Options

Enable debug tap-to-navigate via `menuconfig`:
- `DEBUG_QR_TAP_TO_POUR` - Tap QR code → Pouring screen
- `DEBUG_POURING_TAP_TO_FINISHED` - Tap Pouring → Finished screen
- `DEBUG_FINISHED_TAP_TO_QR` - Tap Finished → QR code screen

## Documentation

- [Hardware Setup](docs/HARDWARE_SETUP.md) - Pin configuration and hardware details
- [MQTT Protocol](docs/MQTT_PROTOCOL.md) - Communication protocol and commands
