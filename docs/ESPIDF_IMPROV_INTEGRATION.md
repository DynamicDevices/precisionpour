# ESP-IDF Improv WiFi Integration

## Overview

The ESP-IDF Improv component provides a native implementation of the Improv WiFi provisioning protocol for ESP-IDF projects. This replaces the Arduino-based Improv WiFi library currently used.

## Component Installation

The Improv component has been added to `idf_component.yml`:

```yaml
dependencies:
  improv/improv: "^1.2.5"
```

The component will be automatically downloaded when building the project with PlatformIO.

## Component Location

ESP-IDF components are typically installed in:
- `managed_components/` (created automatically by ESP-IDF Component Manager)
- Or can be manually placed in `components/` directory

## API Overview

Based on the ESP Component Registry documentation, the ESP-IDF Improv component provides:

1. **Initialization**: `improv_ble_init()` or similar function
2. **State Callbacks**: Set callbacks for WiFi state changes
3. **Provisioning**: Handles WiFi credential provisioning over BLE
4. **Status Reporting**: Reports provisioning status and errors

## Migration from Arduino Improv Library

### Current Implementation (Arduino)
- Uses `ImprovWiFiBLE` class from Arduino library
- Uses `NimBLE-Arduino` for BLE functionality
- Manual BLE initialization and management

### ESP-IDF Improv Component
- Native ESP-IDF component (no Arduino dependencies)
- Uses ESP-IDF's built-in BLE stack (no NimBLE-Arduino needed)
- Integrated with ESP-IDF WiFi and BLE APIs

## Integration Steps

1. **Remove Arduino Improv Dependencies**:
   - Remove `ImprovWiFiBLE.h` includes
   - Remove `NimBLE-Arduino` dependency
   - Remove Arduino-specific BLE code

2. **Add ESP-IDF Improv Component**:
   - Component is already added to `idf_component.yml`
   - Include header: `#include "improv.h"` or similar

3. **Update WiFi Manager**:
   - Replace `ImprovWiFiBLE` class usage with ESP-IDF Improv API
   - Update BLE initialization to use ESP-IDF BLE APIs
   - Update callbacks to match ESP-IDF Improv API

4. **Test Integration**:
   - Verify BLE advertising works
   - Test WiFi credential provisioning
   - Verify WiFi connection after provisioning

## References

- ESP Component Registry: https://components.espressif.com/components/improv/improv
- Improv WiFi Protocol: https://www.improv-wifi.com/
- ESPHome Improv Implementation: https://www.esphome.io/components/esp32_improv/

## Next Steps

1. Build project to download Improv component
2. Check component header files for API documentation
3. Update `src/wifi_manager.cpp` to use ESP-IDF Improv API
4. Remove Arduino Improv and NimBLE dependencies
5. Test provisioning functionality
