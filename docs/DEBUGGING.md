# Debugging Guide for Physical Hardware

## Uploading Firmware to ESP32

### Step 1: Put ESP32 into Bootloader Mode

The ESP32 needs to be in bootloader mode to accept firmware uploads. Follow these steps:

1. **Hold the BOOT button** (usually labeled "BOOT" or "IO0")
2. **Press and release the RESET button** (while still holding BOOT)
3. **Release the BOOT button**
4. The ESP32 should now be in bootloader mode

**Note**: Some ESP32 boards enter bootloader mode automatically when you try to upload. If upload fails, manually enter bootloader mode using the steps above.

### Step 2: Upload Firmware

```bash
# Upload to auto-detected port
pio run -t upload

# Or specify port manually
pio run -t upload --upload-port /dev/ttyUSB0
# or
pio run -t upload --upload-port /dev/ttyUSB1
```

### Step 3: Monitor Serial Output

After uploading, monitor the serial output to see what's happening:

```bash
# Monitor serial output
pio device monitor

# Or specify port and baud rate
pio device monitor --port /dev/ttyUSB0 --baud 115200
```

## Common Upload Issues

### Issue: "Failed to connect to ESP32"

**Solutions:**
1. Put ESP32 into bootloader mode (see Step 1 above)
2. Try the other USB port (`/dev/ttyUSB0` vs `/dev/ttyUSB1`)
3. Reduce upload speed in `platformio.ini`:
   ```ini
   upload_speed = 115200  ; Instead of 921600
   ```
4. Check USB cable (use a data cable, not just power)
5. Try unplugging and replugging the USB cable

### Issue: "Permission denied" on /dev/ttyUSB*

**Solution:**
Add your user to the `dialout` group:
```bash
sudo usermod -a -G dialout $USER
# Then log out and log back in
```

### Issue: "No serial data received"

**Solutions:**
1. Ensure ESP32 is in bootloader mode
2. Try holding BOOT button during entire upload
3. Check if another program is using the serial port
4. Try a different USB port on your computer

## Debugging Display Issues

### Check Serial Output

The firmware outputs detailed debug information. Look for:

```
[Setup] Starting ESP32 Setup...
[Display] Initializing display...
[Touch] Initializing touch...
[Flow Meter] Flow meter initialized on pin 25
[WiFi] Connecting to WiFi...
[MQTT] Connected!
```

### Display Not Showing Anything

1. **Check backlight**: The backlight should turn on early in initialization
2. **Check SPI connections**: Verify MOSI, MISO, SCLK, CS, DC, RST pins
3. **Check power**: Ensure ESP32 has adequate power supply
4. **Check serial output**: Look for display initialization errors

### Display Shows Garbage/Corrupted Image

1. **Check SPI speed**: May need to reduce SPI speed in TFT_eSPI config
2. **Check wiring**: Verify all SPI connections are correct
3. **Check display driver**: Ensure correct driver is selected (ILI9341)

### Touch Not Working

1. **Check touch controller**: Verify XPT2046 is correctly configured
2. **Check IRQ pin**: Ensure TOUCH_IRQ pin is connected (GPIO 22)
3. **Check serial output**: Look for touch initialization messages
4. **Calibration**: Touch may need calibration for accurate coordinates

## Debugging Flow Meter

### Check Flow Meter Readings

Monitor serial output for flow meter messages:
```
[Flow Meter] Flow: 5.23 L/min, Total: 0.125 L, Pulses: 56
```

### Flow Meter Not Detecting Flow

1. **Check wiring**: 
   - Red → 5V
   - Black → GND
   - Yellow → GPIO 25
2. **Check interrupt**: Verify GPIO 25 is interrupt-capable
3. **Check flow**: Ensure liquid is actually flowing through sensor
4. **Check direction**: Sensor has flow direction arrow - install correctly

## Debugging WiFi/MQTT

### WiFi Not Connecting

Check serial output for:
```
[WiFi] Connecting to WiFi: YourSSID
[WiFi] WiFi connected!
IP address: 192.168.x.x
```

**Troubleshooting:**
1. Check credentials in `include/secrets.h`
2. Verify WiFi network is 2.4GHz (ESP32 doesn't support 5GHz)
3. Check signal strength
4. Look for connection timeout messages

### MQTT Not Connecting

Check serial output for:
```
[MQTT] Attempting to connect...
[MQTT] Connected!
```

**Troubleshooting:**
1. Verify MQTT server address in `include/secrets.h`
2. Check if MQTT server requires authentication
3. Verify WiFi is connected first
4. Check firewall/network settings

## Using PlatformIO Serial Monitor

### Basic Usage
```bash
pio device monitor
```

### Advanced Options
```bash
# Specify port and baud rate
pio device monitor --port /dev/ttyUSB0 --baud 115200

# Filter output
pio device monitor --filter esp32_exception_decoder

# Show timestamps
pio device monitor --timestamp
```

### Exit Serial Monitor
Press `Ctrl+]` or `Ctrl+C` to exit the serial monitor.

## Reading Serial Output from Running Device

If the device is already running, you can monitor its output:

```bash
# Monitor on specific port
pio device monitor --port /dev/ttyUSB0 --baud 115200

# Or use screen/minicom
screen /dev/ttyUSB0 115200
# Exit screen: Ctrl+A then K, then Y
```

## Hardware Debugging Tips

1. **Use a multimeter** to verify power and connections
2. **Check pin assignments** in `include/config.h`
3. **Verify SPI/I2C connections** are correct
4. **Test components individually** if possible
5. **Check for loose connections**
6. **Verify power supply** is adequate (ESP32 needs good power)

## Getting Help

If you encounter issues:

1. Check serial output for error messages
2. Verify hardware connections
3. Check `include/config.h` for correct pin assignments
4. Review this debugging guide
5. Check PlatformIO documentation: https://docs.platformio.org/
