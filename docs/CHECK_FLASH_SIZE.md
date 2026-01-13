# How to Check ESP32 Flash Size

## Method 1: Check Boot Log Output (Easiest)

When your ESP32 boots, it logs the flash size. Look for this in the serial monitor output:

```
Flash size: 2097152 bytes  (2MB)
```

Or check the boot messages which often show:
```
Detecting chip type... ESP32
Chip is ESP32-D0WDQ6 (revision 1)
Features: WiFi, BT, Dual Core, 240MHz, VRef calibration in efuse, Coding Scheme None
Crystal is 40MHz
MAC: XX:XX:XX:XX:XX:XX
Uploading stub...
Running stub...
Stub running...
Configuring flash size...
Auto-detected Flash size: 2MB
```

## Method 2: Check Serial Monitor Output

Your firmware already logs flash size on startup. Look for this line in the serial monitor:

```
I (xxxx) main: Flash size: 2097152 bytes
```

- `2097152 bytes` = 2MB
- `4194304 bytes` = 4MB
- `8388608 bytes` = 8MB

## Method 3: Use esptool.py (Command Line)

If you have esptool installed:

```bash
# Connect to device and read flash ID
esptool.py --port /dev/ttyUSB0 flash_id

# Or if using PlatformIO's esptool
pio run -t upload --upload-port /dev/ttyUSB0
# Then check the output for "Auto-detected Flash size"
```

## Method 4: Use ESP-IDF Commands

```bash
# Using idf.py (if you have ESP-IDF installed)
idf.py flash_id

# Or using espefuse
espefuse.py summary
```

## Method 5: Check Programmatically (Already in Your Code)

Your firmware already detects and logs flash size using `esp_flash_get_size()`:

```cpp
ESP_LOGI(TAG_MAIN, "Flash size: %d bytes", ESP.getFlashChipSize());
```

This is logged during startup in `main.cpp` line 193.

## Method 6: Check Boot Messages

The ESP32 bootloader often reports flash size. Look for messages like:

```
flash size: 2MB
```

or

```
Detected flash size: 2MB
```

## Quick Check

The easiest way is to:
1. Connect to serial monitor: `pio device monitor`
2. Reset the device
3. Look for the "Flash size:" log message in the output

## Converting Bytes to MB

- 1MB = 1,048,576 bytes (2^20)
- 2MB = 2,097,152 bytes
- 4MB = 4,194,304 bytes
- 8MB = 8,388,608 bytes

## Your Current Configuration

Based on your sdkconfig and the warning you received:
- **Hardware**: 2MB flash
- **sdkconfig**: Set to 2MB ✓
- **platformio.ini**: Now set to 2MB ✓

The warning should be resolved after the platformio.ini change.
