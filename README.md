# ESP32 Touchscreen Display Firmware

Firmware development workspace for ESP32-based touchscreen display.

## Development Environment Options

### Option 1: PlatformIO (Recommended) ⭐

**Pros:**
- Modern, professional development environment
- Integrated with VS Code
- Excellent library management
- Cross-platform support
- Built-in serial monitor and debugging tools

**Setup:**
1. Install VS Code
2. Install PlatformIO IDE extension
3. Open this project folder in VS Code
4. PlatformIO will automatically detect and set up the environment

**Usage:**
```bash
# Build the project
pio run

# Upload to ESP32
pio run -t upload

# Monitor serial output
pio device monitor
```

### Option 2: ESP-IDF (Official Framework)

**Pros:**
- Official Espressif development framework
- Full control over ESP32 features
- Advanced debugging capabilities
- Production-ready

**Setup:**
```bash
# Install ESP-IDF (Linux)
mkdir -p ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32

# Source the environment
. ~/esp/esp-idf/export.sh
```

### Option 3: Arduino IDE

**Pros:**
- Simple and beginner-friendly
- Large community and examples
- Quick prototyping

**Setup:**
1. Install Arduino IDE
2. Add ESP32 board support:
   - File > Preferences > Additional Board Manager URLs
   - Add: `https://dl.espressif.com/dl/package_esp32_index.json`
3. Tools > Board > Boards Manager > Search "ESP32" > Install
4. Install libraries: TFT_eSPI, LVGL (if needed)

## Project Structure

```
precisionpour/
├── platformio.ini          # PlatformIO configuration
├── src/                    # Source files (.cpp)
│   └── main.cpp            # Main firmware entry point
├── include/                # Header files (.h, .hpp)
│   ├── config.h            # Hardware pin definitions
│   ├── display.h           # Display driver interface
│   └── touch.h             # Touch controller interface
├── lib/                    # Custom libraries (not in PlatformIO registry)
├── test/                   # Unit tests
├── data/                   # Filesystem data (SPIFFS/LittleFS)
├── docs/                   # Documentation
│   ├── SETUP_GUIDE.md      # PlatformIO setup instructions
│   ├── QUICK_REFERENCE.md  # Command reference
│   └── HARDWARE_SETUP.md   # Hardware configuration guide
└── README.md               # This file
```

### Directory Usage

- **`src/`** - All `.cpp` source files. PlatformIO automatically compiles these.
- **`include/`** - Header files. Automatically added to include path.
- **`lib/`** - Custom libraries not available via PlatformIO's library manager.
- **`test/`** - Unit tests. Run with `pio test`.
- **`data/`** - Files to upload to ESP32 filesystem. Upload with `pio run -t uploadfs`.
- **`docs/`** - Project documentation and guides.

## Hardware Configuration

**Note:** Update the following based on your specific display model:

- Display driver (ILI9341, ST7789, etc.)
- Touch controller (XPT2046, FT6236, etc.)
- Pin connections (SPI pins, touch pins, etc.)
- Resolution and orientation

## UI Framework: LVGL

This project uses **LVGL (Light and Versatile Graphics Library)** for modern, professional UI development.

### Features
- ✅ Modern widget-based UI framework
- ✅ Touch input support
- ✅ Smooth animations and transitions
- ✅ Low memory footprint
- ✅ Hardware-accelerated rendering
- ✅ Extensive widget library

### Project Structure with LVGL

```
precisionpour/
├── src/
│   ├── main.cpp           # LVGL initialization and main loop
│   ├── lvgl_display.cpp    # Display driver integration
│   ├── lvgl_touch.cpp     # Touch controller integration
│   └── ui.cpp             # UI creation and management
├── include/
│   ├── lv_conf.h          # LVGL configuration
│   ├── lvgl_display.h     # Display driver interface
│   ├── lvgl_touch.h       # Touch driver interface
│   └── ui.h               # UI interface
└── lib/
    └── TFT_eSPI_Config/   # TFT_eSPI display configuration
        └── User_Setup.h
```

## Quick Start

1. **Install PlatformIO**: See [docs/SETUP_GUIDE.md](docs/SETUP_GUIDE.md)
2. **Configure Hardware**: Update `include/config.h` with your display pins
3. **Add Libraries**: Uncomment libraries in `platformio.ini` based on your display
4. **Build & Upload**: Use PlatformIO commands (see [docs/QUICK_REFERENCE.md](docs/QUICK_REFERENCE.md))

## Next Steps

1. Identify your display model and specifications
2. Update pin definitions in `include/config.h`
3. Choose and configure the appropriate graphics library in `platformio.ini`
4. Implement display and touch drivers using the interfaces in `include/`
5. Start developing your UI!

## Documentation

- **[Setup Guide](docs/SETUP_GUIDE.md)** - Detailed PlatformIO installation instructions
- **[Quick Reference](docs/QUICK_REFERENCE.md)** - Common commands and shortcuts
- **[Hardware Setup](docs/HARDWARE_SETUP.md)** - Hardware configuration guide
