# PlatformIO Quick Reference

## Common Commands

### Building
```bash
pio run                    # Build the project
pio run -t clean           # Clean build files
pio run -v                 # Verbose build output
```

### Uploading
```bash
pio run -t upload          # Build and upload
pio run -t upload -t monitor  # Upload and open monitor
```

### Serial Monitor
```bash
pio device monitor         # Open serial monitor
pio device monitor --baud 115200  # With specific baud rate
pio device monitor --filter esp32_exception_decoder  # With filters
```

### Device Management
```bash
pio device list           # List connected devices
pio device monitor --port /dev/ttyUSB0  # Monitor specific port
```

### Library Management
```bash
pio lib list              # List installed libraries
pio lib search <name>     # Search for libraries
pio lib install <name>    # Install a library
pio lib update            # Update all libraries
```

### Project Management
```bash
pio project init          # Initialize new project
pio home                  # Open PlatformIO Home
pio upgrade               # Upgrade PlatformIO Core
```

## VS Code Shortcuts

- **Build**: `Ctrl+Alt+B` (or `Cmd+Alt+B` on Mac)
- **Upload**: `Ctrl+Alt+U` (or `Cmd+Alt+U` on Mac)
- **Serial Monitor**: `Ctrl+Alt+S` (or `Cmd+Alt+S` on Mac)
- **Clean**: `Ctrl+Alt+C` (or `Cmd+Alt+C` on Mac)

## PlatformIO Sidebar Icons

- 🏠 **Home** - PlatformIO home and project explorer
- 🔧 **Project Tasks** - Build, upload, clean, etc.
- 📚 **Libraries** - Manage libraries
- 🔌 **Devices** - Connected devices and serial monitor
- ⚙️ **Settings** - PlatformIO settings

## Common ESP32 Board Types

Update `board = ` in `platformio.ini` if needed:
- `esp32dev` - Generic ESP32 Development Board (default)
- `esp32doit-devkit-v1` - DOIT ESP32 DevKit v1
- `esp32-wrover-kit` - ESP32-WROVER-KIT
- `esp32-s3-devkitc-1` - ESP32-S3 DevKit
- `esp32-c3-devkitm-1` - ESP32-C3 DevKit

## Useful Build Flags

Add to `build_flags` in `platformio.ini`:
```ini
-DCORE_DEBUG_LEVEL=3        # Debug level (0-5)
-DBOARD_HAS_PSRAM          # Enable PSRAM support
-mfix-esp32-psram-cache-issue  # Fix PSRAM cache issues
-DARDUINO_USB_CDC_ON_BOOT=1  # Enable USB CDC on boot (ESP32-S2/S3)
```

## Library Installation Examples

Add to `lib_deps` in `platformio.ini`:
```ini
lib_deps = 
    lvgl/lvgl@^8.4.0                    # LVGL graphics library
    bodmer/TFT_eSPI@^2.5.43             # TFT display library
    adafruit/Adafruit GFX Library@^1.11.9
    adafruit/Adafruit TouchScreen@^1.1.4
    XPT2046_Touchscreen@^1.4.0          # Touch controller
```

## Troubleshooting Commands

```bash
# Check PlatformIO version
pio --version

# Check installed platforms
pio platform list

# Check installed boards
pio boards espressif32

# Update platform
pio platform update espressif32

# Check for issues
pio check
```
