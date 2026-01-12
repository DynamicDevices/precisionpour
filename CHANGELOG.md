# Changelog - Project Improvements

## 2026 - Code Review Improvements

### Critical Fixes

#### 1. GPIO25 Pin Conflict Resolution
- **Issue**: GPIO25 was used for both `TOUCH_SCLK` and `FLOW_METER_PIN`, causing conflicts
- **Solution**: Changed `FLOW_METER_PIN` from GPIO25 to GPIO26
- **Impact**: Flow meter and touch screen can now work simultaneously without conflicts
- **Files Changed**:
  - `include/config.h`: Updated `FLOW_METER_PIN` definition
  - `docs/HARDWARE_SETUP.md`: Updated pin documentation
  - `docs/FLOW_METER.md`: Updated wiring instructions
  - `README.md`: Updated hardware configuration section

#### 2. Code Duplication Removal
- **Issue**: WiFi and MQTT maintenance functions were called twice in `main.cpp` loop
- **Solution**: Removed duplicate calls (lines 371-377)
- **Impact**: Cleaner code, slightly better performance
- **Files Changed**:
  - `src/main.cpp`: Removed duplicate `wifi_manager_loop()` and `mqtt_client_loop()` calls

### Error Recovery Enhancements

#### 3. Watchdog Timer Implementation
- **Feature**: Added ESP32 watchdog timer for system recovery
- **Configuration**: 
  - `ENABLE_WATCHDOG`: Enable/disable watchdog (default: 1)
  - `WATCHDOG_TIMEOUT_SEC`: Timeout in seconds (default: 60)
- **Behavior**: System resets if watchdog is not fed within timeout period
- **Files Changed**:
  - `include/config.h`: Added watchdog configuration
  - `src/main.cpp`: Added watchdog initialization and feeding

#### 4. Enhanced JSON Parsing and Validation
- **Feature**: Improved MQTT JSON payload validation with bounds checking
- **Validations Added**:
  - `unique_id`: Length validation (1-128 characters)
  - `cost_per_ml`: Range validation (0 < cost <= 1000)
  - `max_ml`: Range validation (0 < max <= 100000)
  - `currency`: Valid values (GBP, USD, or empty)
- **Error Tracking**: Added consecutive error counter with automatic reset
- **Recovery**: System resets after `MAX_CONSECUTIVE_ERRORS` (10) errors
- **Files Changed**:
  - `include/config.h`: Added error recovery configuration
  - `src/main.cpp`: Enhanced JSON validation and error tracking

### Testing Infrastructure

#### 5. Unit Tests Added
- **Test Files Created**:
  - `test/test_flow_meter.cpp`: Tests for flow rate and volume calculations
  - `test/test_mqtt_json.cpp`: Tests for MQTT JSON parsing and validation
- **Coverage**:
  - Flow rate calculations (0, 1, 10, 30 L/min)
  - Volume calculations from pulse counts
  - Edge cases and boundary conditions
  - Valid and invalid JSON payloads
  - Field validation logic
- **Framework**: Unity test framework (PlatformIO default)

### Code Quality

#### 6. Code Formatting Configuration
- **Feature**: Added `.clang-format` configuration file
- **Style**: Based on Google C++ Style Guide with ESP32/Arduino adaptations
- **Settings**:
  - 4-space indentation
  - 100 character line limit
  - Attached braces style
  - C++11 standard
- **Usage**: Run `clang-format -i <file>` to format code

### Documentation Updates

#### 7. Hardware Documentation Updates
- Updated all references to flow meter pin from GPIO25 to GPIO26
- Removed pin conflict warnings
- Updated wiring diagrams
- Updated troubleshooting sections
- **Files Updated**:
  - `docs/HARDWARE_SETUP.md`
  - `docs/FLOW_METER.md`
  - `README.md`

## Configuration Changes

### New Configuration Options (`include/config.h`)

```cpp
// Error Recovery Configuration
#define ENABLE_WATCHDOG 1                  // Enable ESP32 watchdog timer
#define WATCHDOG_TIMEOUT_SEC 60            // Watchdog timeout in seconds
#define MAX_CONSECUTIVE_ERRORS 10         // Max errors before reset
#define ERROR_RESET_DELAY_MS 5000         // Delay before reset (ms)
```

### Pin Changes

| Component | Old Pin | New Pin | Reason |
|-----------|---------|---------|--------|
| Flow Meter | GPIO25 | GPIO26 | Avoid conflict with TOUCH_SCLK |

## Testing

To run the new unit tests:

```bash
# Run all tests
pio test

# Run specific test
pio test -f test_flow_meter
pio test -f test_mqtt_json

# Verbose output
pio test -v
```

## Migration Notes

### For Existing Hardware

If you have existing hardware with flow meter connected to GPIO25:

1. **Option 1 (Recommended)**: Reconnect flow meter yellow wire to GPIO26
2. **Option 2**: Change `FLOW_METER_PIN` back to 25 in `include/config.h` if touch is not needed

### Code Formatting

To format existing code with the new `.clang-format`:

```bash
# Format all C++ files
find src include -name "*.cpp" -o -name "*.h" | xargs clang-format -i
```

## Summary

All recommendations from the code review have been implemented:
- ✅ Fixed GPIO25 pin conflict
- ✅ Removed duplicate code
- ✅ Added error recovery (watchdog + validation)
- ✅ Added unit tests
- ✅ Added code formatting configuration
- ✅ Updated documentation

The project is now more robust, maintainable, and better tested.
