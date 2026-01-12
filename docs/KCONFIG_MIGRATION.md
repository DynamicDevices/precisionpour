# KConfig Migration Guide

This guide explains how to use ESP-IDF's KConfig system for configuration management in the PrecisionPour firmware.

## Overview

The project now supports two configuration methods:

1. **Arduino Framework** (default): Uses `#define` macros in `include/config.h`
2. **ESP-IDF Framework**: Uses KConfig system with `idf.py menuconfig` interface

## Benefits of KConfig

- **Menu-based configuration**: Easy-to-use menu interface (`idf.py menuconfig`)
- **Dependency management**: Automatic handling of configuration dependencies
- **Type safety**: Compile-time validation of configuration values
- **Documentation**: Built-in help text for each option
- **Version control**: Configuration saved in `sdkconfig` file

## Quick Start

### Using ESP-IDF Framework

1. **Switch to ESP-IDF environment**:
   ```bash
   pio run -e esp32dev-idf
   ```

2. **Open configuration menu**:
   ```bash
   idf.py menuconfig
   ```
   
   Or with PlatformIO:
   ```bash
   pio run -e esp32dev-idf -t menuconfig
   ```

3. **Navigate to PrecisionPour Configuration**:
   - Use arrow keys to navigate
   - Press `Enter` to enter submenus
   - Press `Space` to toggle boolean options
   - Type values for string/int options
   - Press `?` for help on any option
   - Press `Esc` twice to exit and save

4. **Build and upload**:
   ```bash
   pio run -e esp32dev-idf -t upload
   ```

### Using Arduino Framework (Current Default)

Continue using the existing workflow:
```bash
pio run -e esp32dev -t upload
```

Edit `include/config.h` directly to change configuration values.

## Configuration Categories

The KConfig menu is organized into the following categories:

### Hardware Configuration
- **Display Pins (SPI)**: TFT display pin assignments
- **Touch Screen Pins**: Touch controller pin assignments
- **Flow Meter Pin**: Flow sensor interrupt pin
- **RGB LED Pins**: Status LED pin assignments
- **RFID/NFC Pins**: Optional RFID/NFC pin assignments

### Display Settings
- Display width/height
- Display rotation
- Touch sensitivity threshold

### Serial Configuration
- Serial baud rate

### Operating Mode
- **Production Mode**: PrecisionPour branded UI with QR code
- **Test Mode**: Hardware testing interface

### WiFi Configuration
- WiFi reconnect delay
- WiFi provisioning method (secrets.h or Improv BLE)
- Improv WiFi timeout
- Saved credentials option

### MQTT Configuration
- MQTT port
- Client ID prefix
- Topic prefix
- Reconnect delay
- Keepalive interval

### Error Recovery Configuration
- Watchdog timer enable/disable
- Watchdog timeout
- Maximum consecutive errors
- Error reset delay

### Pouring Mode Configuration
- Default cost per liter
- Currency symbol

### Development Options
- Debug level

## Configuration File Locations

### ESP-IDF Framework
- **KConfig file**: `Kconfig.projbuild` (defines menu structure)
- **Configuration file**: `sdkconfig` (saved configuration values)
- **Generated header**: `build/include/sdkconfig.h` (auto-generated)

### Arduino Framework
- **Configuration file**: `include/config.h` (edit directly)

## Migration Steps

### From Arduino to ESP-IDF

1. **Backup current configuration**:
   - Note down any custom values from `include/config.h`

2. **Switch to ESP-IDF environment**:
   ```bash
   pio run -e esp32dev-idf -t menuconfig
   ```

3. **Configure settings**:
   - Navigate through menus
   - Set values matching your Arduino configuration
   - Save and exit

4. **Test build**:
   ```bash
   pio run -e esp32dev-idf
   ```

5. **Update code if needed**:
   - Some Arduino-specific code may need adaptation
   - Check for any framework-specific APIs

### From ESP-IDF to Arduino

1. **Export configuration**:
   - Note down values from `sdkconfig` or menuconfig

2. **Update `include/config.h`**:
   - Set `#define` values to match KConfig settings

3. **Switch back to Arduino**:
   ```bash
   pio run -e esp32dev
   ```

## Code Compatibility

The `include/config.h` file automatically detects which framework is being used:

```cpp
#ifdef CONFIG_TFT_MOSI
    // ESP-IDF: Use KConfig values
    #define TFT_MOSI CONFIG_TFT_MOSI
    // ...
#else
    // Arduino: Use #define fallbacks
    #define TFT_MOSI 13
    // ...
#endif
```

This means your source code doesn't need to change - it always uses the same `#define` names regardless of framework.

## Common Tasks

### Changing a Pin Assignment

**ESP-IDF**:
```bash
idf.py menuconfig
# Navigate to: PrecisionPour Configuration > Hardware Configuration > Display Pins (SPI) > TFT MOSI Pin
# Enter new pin number
# Save and exit
```

**Arduino**:
```cpp
// Edit include/config.h
#define TFT_MOSI 13  // Change to new pin
```

### Enabling Test Mode

**ESP-IDF**:
```bash
idf.py menuconfig
# Navigate to: PrecisionPour Configuration > Operating Mode
# Select "Test Mode"
# Save and exit
```

**Arduino**:
```cpp
// Edit include/config.h
#define TEST_MODE 1
```

### Changing WiFi Provisioning Method

**ESP-IDF**:
```bash
idf.py menuconfig
# Navigate to: PrecisionPour Configuration > WiFi Configuration > WiFi Provisioning Method
# Select "Use Improv WiFi BLE Provisioning"
# Save and exit
```

**Arduino**:
```cpp
// Edit include/config.h
#define USE_IMPROV_WIFI 1
```

## Troubleshooting

### KConfig menu not appearing

- Ensure you're using the ESP-IDF environment: `pio run -e esp32dev-idf`
- Check that `Kconfig.projbuild` exists in project root
- Try cleaning build: `pio run -e esp32dev-idf -t clean`

### Configuration not taking effect

- Clean and rebuild: `pio run -e esp32dev-idf -t clean`
- Check `sdkconfig` file exists and has correct values
- Verify `include/config.h` has the `#ifdef CONFIG_TFT_MOSI` check

### Build errors with ESP-IDF

- Some Arduino libraries may not be compatible with ESP-IDF
- Check library compatibility before switching
- You may need ESP-IDF-specific versions of some libraries

## Advanced Usage

### Saving Configuration Presets

You can save different configurations for different hardware variants:

```bash
# Save current configuration
cp sdkconfig sdkconfig.production

# Load a preset
cp sdkconfig.production sdkconfig
idf.py menuconfig  # Verify settings
```

### Programmatic Configuration

You can also set configuration values from command line:

```bash
idf.py menuconfig --set-value CONFIG_TFT_MOSI 13
```

### Configuration Dependencies

KConfig automatically handles dependencies. For example:
- `IMPROV_WIFI_TIMEOUT_MS` only appears if `WIFI_IMPROV_BLE` is enabled
- `WATCHDOG_TIMEOUT_SEC` only appears if `ENABLE_WATCHDOG` is enabled

## Best Practices

1. **Version Control**: 
   - Commit `Kconfig.projbuild` (menu structure)
   - Consider committing `sdkconfig` for reproducible builds
   - Or document configuration in README

2. **Documentation**:
   - Keep help text in KConfig up to date
   - Document any non-obvious configuration choices

3. **Testing**:
   - Test both Arduino and ESP-IDF builds
   - Verify configuration works in both frameworks

4. **Default Values**:
   - Set sensible defaults in KConfig
   - Match defaults between KConfig and Arduino fallbacks

## References

- [ESP-IDF Configuration System](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/kconfig.html)
- [PlatformIO ESP-IDF](https://docs.platformio.org/en/latest/frameworks/espidf.html)
- [KConfig Syntax](https://www.kernel.org/doc/html/latest/kbuild/kconfig-language.html)
