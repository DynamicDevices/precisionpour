# Touchscreen Debugging Guide

This document describes how to debug touchscreen issues in the PrecisionPour firmware.

## Initialization Logging

When the firmware boots, the touchscreen initialization outputs detailed information:

```
[Touch] Initializing touch controller...
[Touch] CS pin configured: GPIO33
[Touch] SPI pins configured: SCLK=GPIO25, MOSI=GPIO32, MISO=GPIO39
[Touch] IRQ pin configured: GPIO36 (initial state: HIGH (not pressed))
[Touch] Initial read test: X=2048 Y=2048 Z1=0 Z2=4095
[Touch] Touch controller initialized and registered with LVGL
```

### What to Check

1. **Pin Configuration**: Verify all pins match your hardware
2. **IRQ Initial State**: Should be HIGH when not pressed (LOW when pressed)
3. **Initial Read Test**: 
   - X/Y should be around 2048 (middle of 0-4095 range) when not touched
   - Z1 should be low (0-100) when not touched
   - Z2 should be high (4000-4095) when not touched

## Runtime Logging

### IRQ Pin Changes
```
[Touch] IRQ pin changed: 1 -> 0 (LOW=pressed)
[Touch] IRQ interrupt triggered
```

This indicates the touch controller is detecting a touch via the IRQ pin.

### Touch Detection
```
[Touch] Pressed: X=160 Y=120 (IRQ=1, Pressure=1)
```

This shows:
- **X, Y**: Touch coordinates in display pixels (0-319, 0-239)
- **IRQ**: Whether IRQ pin detected touch (1=yes, 0=no)
- **Pressure**: Whether pressure sensors detected touch (1=yes, 0=no)

### Touch Release
```
[Touch] Released
```

Indicates the touch has been released.

## Log Level Configuration

The firmware uses LVGL log level `WARN` by default to reduce log spam. To see more detailed LVGL logs, edit `include/lv_conf.h`:

```cpp
#define LV_LOG_LEVEL LV_LOG_LEVEL_INFO  // Change from WARN to INFO
```

**Note**: This will generate a lot of output, including layout update messages.

## Common Issues

### No Touch Detection

**Symptoms**: No `[Touch] Pressed` messages when touching screen

**Debugging Steps**:
1. Check initialization logs - verify pins are correct
2. Check IRQ pin state changes - should see `IRQ pin changed` messages
3. Check initial read test - Z1/Z2 values should change when touching
4. Verify wiring - especially IRQ pin connection

**Possible Causes**:
- IRQ pin not connected or wrong pin
- Touch controller not powered
- SPI communication failure
- Touch controller hardware failure

### Touch Detected But UI Not Responding

**Symptoms**: See `[Touch] Pressed` messages but UI doesn't respond

**Debugging Steps**:
1. Verify LVGL input device is registered (check initialization logs)
2. Check if touch coordinates are reasonable (within 0-319, 0-239)
3. Verify screen callback is set (check for error messages)
4. Check LVGL event handling in serial output

**Possible Causes**:
- LVGL input device not properly registered
- Touch coordinates out of bounds
- Screen callback not set
- LVGL event handler not processing events

### Continuous Touch Detection

**Symptoms**: `[Touch] Pressed` messages appearing continuously without touching

**Debugging Steps**:
1. Check IRQ pin state - should be HIGH when not pressed
2. Check Z1/Z2 values - should indicate no pressure when not touched
3. Verify touch controller calibration

**Possible Causes**:
- IRQ pin stuck LOW (hardware issue)
- Pressure sensors reading false positives
- Touch controller calibration incorrect

### Incorrect Touch Coordinates

**Symptoms**: Touching one area but UI responds in different area

**Debugging Steps**:
1. Check raw X/Y values in touch logs
2. Verify display rotation matches touch calibration
3. Check touch calibration values in `lvgl_touch.cpp`:
   ```cpp
   #define TOUCH_X_MIN   100
   #define TOUCH_X_MAX   4000
   #define TOUCH_Y_MIN   100
   #define TOUCH_Y_MAX   4000
   ```

**Possible Causes**:
- Touch calibration values incorrect
- Display rotation mismatch
- Touch controller orientation incorrect

## Screen Switching

The pouring screen can be switched back to the production screen by:
1. **Touch**: Tap anywhere on the pouring screen
2. **MQTT**: Send `stop_pour` command

### Debugging Screen Switching

If touch-to-switch isn't working:

1. **Check Callback Registration**:
   ```
   [Pouring UI] Screen tapped - switching to main screen
   ```
   If you see this message, the touch is detected but callback may not be set.

2. **Check Callback Error**:
   ```
   [Pouring UI] ERROR: screen_switch_callback is NULL!
   ```
   This indicates the callback wasn't set in `setup()`. Check `main.cpp` line 263.

3. **Check Event Type**:
   The screen uses `LV_EVENT_CLICKED` for touch detection. If this doesn't work, try `LV_EVENT_PRESSED` instead.

## Reducing Log Noise

Touch logging is throttled to avoid spam:
- Touch press: Logged every 200ms or on state change
- IRQ changes: Always logged
- Touch release: Only logged on state change

To disable touch logging entirely, comment out the `Serial.printf` statements in `lvgl_touch_read()`.

## Hardware Testing

### Test IRQ Pin
```cpp
// In main loop, add:
int irq_state = digitalRead(TOUCH_IRQ);
Serial.printf("IRQ pin: %d\r\n", irq_state);
```
Should be HIGH (1) when not pressed, LOW (0) when pressed.

### Test SPI Communication
The initialization logs show initial X/Y/Z1/Z2 values. These should change when touching the screen. If they don't change, SPI communication may be failing.

### Test Pressure Sensors
```cpp
// In main loop, add:
uint16_t z1 = xpt2046_read(XPT2046_CMD_Z1);
uint16_t z2 = xpt2046_read(XPT2046_CMD_Z2);
Serial.printf("Z1=%d Z2=%d\r\n", z1, z2);
```
When not touched: Z1 should be low (0-100), Z2 should be high (4000-4095)
When touched: Z1 should increase, Z2 should decrease

## Serial Monitor Commands

Use PlatformIO serial monitor to view touch logs:
```bash
pio device monitor
```

Filter for touch-related messages:
```bash
pio device monitor --filter "Touch"
```

## Additional Resources

- Hardware pinout: [HARDWARE_SETUP.md](HARDWARE_SETUP.md)
- LVGL touch driver: `src/lvgl_touch.cpp`
- Touch configuration: `include/config.h`
