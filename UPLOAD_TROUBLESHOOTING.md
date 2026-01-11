# ESP32 Upload Troubleshooting

## Current Issue: "Failed to connect to ESP32"

The ESP32 is not responding to upload attempts. Here are steps to resolve this:

## Step-by-Step Upload Procedure

### 1. Ensure ESP32 is in Bootloader Mode

**Method A: Manual Bootloader Entry**
1. **Hold BOOT button** (may be labeled "IO0" or "BOOT")
2. **Press and release RESET button** (while still holding BOOT)
3. **Release BOOT button**
4. ESP32 should now be in bootloader mode

**Method B: Auto-Reset (if supported)**
- Some ESP32 boards auto-enter bootloader when upload starts
- Try uploading without manual intervention

### 2. Try Upload Immediately After Bootloader Entry

The bootloader mode only lasts a few seconds. Upload must start within 1-2 seconds of entering bootloader mode.

**Quick Upload Command:**
```bash
cd /home/ajlennon/data_drive/dd/precisionpour
pio run -t upload
```

### 3. Try Different USB Ports

Your device has two USB ports:
- `/dev/ttyUSB0` - Enhanced Com Port
- `/dev/ttyUSB1` - Standard Com Port

Try both:
```bash
# Try USB0
pio run -t upload --upload-port /dev/ttyUSB0

# Try USB1
pio run -t upload --upload-port /dev/ttyUSB1
```

### 4. Check if Device is Already Running

If the device already has firmware, it might be running. Check serial output:

```bash
pio device monitor --port /dev/ttyUSB0 --baud 115200
```

If you see output, the device is running. You may need to:
- Press RESET to restart
- Or manually enter bootloader mode

### 5. Hardware Checks

- **USB Cable**: Use a data cable, not just a power cable
- **Power Supply**: Ensure ESP32 has adequate power
- **Connections**: Check all connections are secure
- **Driver Issues**: Verify USB-to-serial driver is installed

### 6. Alternative Upload Methods

**Method A: Hold BOOT During Entire Upload**
1. Hold BOOT button
2. Start upload: `pio run -t upload`
3. Keep holding BOOT until upload completes

**Method B: Use Arduino IDE**
- Sometimes Arduino IDE works when PlatformIO doesn't
- Export compiled binary from PlatformIO and upload via Arduino IDE

**Method C: Use esptool.py Directly**
```bash
# Find esptool
find ~/.platformio -name "esptool.py"

# Use it directly (example)
python3 ~/.platformio/packages/tool-esptoolpy/esptool.py \
  --port /dev/ttyUSB0 \
  --baud 115200 \
  write_flash 0x10000 .pio/build/esp32dev/firmware.bin
```

## What to Check on Your Display

If the device is already running, you should see:
- Splashscreen with Precision Pour logo
- Progress bar during initialization
- Main screen with QR code
- WiFi/MQTT status icons

If you see this, the firmware is already loaded and working!

## Next Steps

1. **Check if device is running**: Monitor serial output
2. **If running**: We can debug what's happening
3. **If not running**: Continue troubleshooting upload

Let me know what you see on the display or in the serial monitor!
