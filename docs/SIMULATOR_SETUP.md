# ESP32 Simulator Setup Guide

## Overview

This guide covers setting up ESP32 simulators for testing your firmware without physical hardware.

## Option 1: Wokwi (Recommended) ⭐

Wokwi is a web-based ESP32 simulator that works great with PlatformIO projects.

### Installation

1. **Install Wokwi VS Code Extension**:
   - Open VS Code
   - Press `Ctrl+Shift+X` to open Extensions
   - Search for "Wokwi"
   - Install "Wokwi for VS Code" by Wokwi

2. **Configure for PlatformIO**:
   - Wokwi can use your PlatformIO build files
   - Or create a Wokwi project file (see below)

### Usage

1. **Online Simulator**:
   - Go to https://wokwi.com
   - Create a new ESP32 project
   - Copy your code into the editor
   - Add components (displays, sensors, etc.)

2. **VS Code Extension**:
   - Open your PlatformIO project
   - Press `F1` → "Wokwi: Start Simulator"
   - Or use the Wokwi icon in the sidebar

### Wokwi Configuration

The project includes a `wokwi.toml` file and `.wokwi/diagram.json` with the following setup:

**wokwi.toml**:
```toml
[wokwi]
version = 1
firmware = ".pio/build/esp32dev/firmware.bin"
elf = ".pio/build/esp32dev/firmware.elf"

[[wokwi.parts]]
type = "esp32-devkit-v1"
id = "esp"

[[wokwi.parts]]
type = "tft-ili9341"
id = "tft"
width = 320
height = 240

[[wokwi.parts]]
type = "xpt2046"
id = "touch"
```

**Pin Connections** (from `include/config.h`):
- Display: MOSI=23, MISO=19, SCLK=18, CS=5, DC=2, RST=4, BL=15
- Touch: CS=21, IRQ=22 (shares SPI bus with display)

The `.wokwi/diagram.json` file defines the complete circuit with all pin connections.

### Limitations

- Display simulation may not perfectly match hardware
- Touch input simulation is limited
- Some hardware-specific features may not work

## Option 2: ESP32 Hardware Simulator

A desktop application for ESP32 simulation.

### Installation

1. Download from: https://esp32hwsim.com
2. Install on your system
3. Load your compiled firmware

### Features

- Multiple ESP32 simulation
- Network connection testing
- Component integration

## Option 3: QEMU (Advanced)

QEMU can emulate ESP32, but setup is complex.

### Installation

```bash
# Install QEMU
sudo apt install qemu-system-xtensa

# Build ESP32 QEMU (complex, requires ESP-IDF)
```

**Note**: QEMU ESP32 support is experimental and may not work well with Arduino framework.

## Recommended Approach

For this touchscreen project:

1. **Development**: Use Wokwi for basic code testing
2. **Display Testing**: Use physical hardware (displays are hard to simulate accurately)
3. **Debugging**: Use PlatformIO's built-in debugging with physical hardware

## Wokwi Component Library

Wokwi supports many components:
- TFT displays (ILI9341, ST7789, etc.)
- Touch controllers (XPT2046, FT6236)
- Sensors, buttons, LEDs
- Wi-Fi simulation
- Serial communication

## Tips

- Start with Wokwi for logic testing
- Test display code on real hardware
- Use serial monitor for debugging
- Wokwi's display simulation is basic - real hardware testing is essential

## Resources

- [Wokwi Documentation](https://docs.wokwi.com)
- [Wokwi ESP32 Guide](https://docs.wokwi.com/guides/esp32)
- [ESP32 Hardware Simulator](https://esp32hwsim.com)
