# Improv WiFi Usage Guide

This document explains how to use Improv WiFi provisioning with the PrecisionPour firmware.

## Overview

Improv WiFi allows you to configure WiFi credentials without hardcoding them in the firmware. The device can receive WiFi credentials via Serial (USB) connection or through the Improv WiFi mobile app.

## How It Works

1. **Initial Boot**: Device tries to connect using saved credentials (if available) or hardcoded credentials from `secrets.h`
2. **Connection Failure**: If connection fails, Improv WiFi provisioning starts automatically
3. **Provisioning**: Device accepts WiFi credentials via Serial or BLE (Serial protocol currently implemented)
4. **Save & Connect**: Credentials are saved to EEPROM and device connects to WiFi
5. **Future Boots**: Device uses saved credentials automatically

## Configuration

In `include/config.h`:

```cpp
// Enable/disable Improv WiFi provisioning
#define USE_IMPROV_WIFI 1  // 1 = enabled, 0 = disabled

// Timeout for provisioning (5 minutes default)
#define IMPROV_WIFI_TIMEOUT_MS 300000

// Use saved credentials from EEPROM
#define USE_SAVED_CREDENTIALS 1  // 1 = use saved, 0 = always use secrets.h
```

## Usage Methods

### Method 1: Improv WiFi Mobile App (Recommended)

1. **Download App**: Install "Improv WiFi" app on iOS or Android
2. **Connect Device**: Connect ESP32 via USB to your computer/phone
3. **Open App**: Launch Improv WiFi app
4. **Select Device**: App should detect the device via Serial
5. **Configure WiFi**: Enter WiFi SSID and password in the app
6. **Connect**: Device receives credentials and connects automatically

### Method 2: Serial Commands (Advanced)

You can send Improv WiFi commands directly via Serial monitor:

1. **Open Serial Monitor**: Use PlatformIO serial monitor at 115200 baud
2. **Send Commands**: Follow Improv WiFi Serial protocol
3. **Device Responds**: Device processes commands and connects

**Note**: Serial protocol details are in the [Improv WiFi specification](https://www.improv-wifi.com/serial/).

## Behavior

### Normal Operation (WiFi Connected)

- Device uses saved credentials (if available) or hardcoded credentials
- Connects to WiFi automatically on boot
- No provisioning needed

### Provisioning Mode (WiFi Not Connected)

- Device enters provisioning mode automatically after connection failure
- Serial output shows: `[Improv WiFi] Starting Serial provisioning...`
- Device accepts credentials for 5 minutes (configurable timeout)
- After timeout, device tries saved/hardcoded credentials again

### Credential Priority

1. **Saved Credentials** (if `USE_SAVED_CREDENTIALS = 1` and credentials exist in EEPROM)
2. **Hardcoded Credentials** (from `secrets.h`)
3. **Improv WiFi Provisioning** (if connection fails)

## Clearing Saved Credentials

To clear saved credentials and force re-provisioning:

1. **Via Code**: Call `clear_saved_credentials()` function (not currently exposed in API)
2. **Via EEPROM**: Use PlatformIO to clear EEPROM/Preferences
3. **Factory Reset**: Reflash firmware (clears all EEPROM data)

## Serial Output

When provisioning is active, you'll see:

```
[Improv WiFi] Starting Serial provisioning...
[Improv WiFi] Serial provisioning active
[Improv WiFi] Send WiFi credentials via Serial or use Improv WiFi app
```

When credentials are received:

```
[Improv WiFi] Received credentials for: YourWiFiNetwork
[WiFi] Connecting to: YourWiFiNetwork
[WiFi] Connected!
[WiFi] IP address: 192.168.1.100
[WiFi] Saved credentials for: YourWiFiNetwork
[Improv WiFi] Provisioning successful!
```

## Troubleshooting

### Provisioning Not Starting

- Check `USE_IMPROV_WIFI` is set to `1` in `config.h`
- Verify WiFi connection actually failed (check serial output)
- Wait for automatic provisioning after connection failure

### Credentials Not Saving

- Check `USE_SAVED_CREDENTIALS` is set to `1` in `config.h`
- Verify EEPROM/Preferences is working (check serial output)
- Check for EEPROM errors in serial output

### Connection Fails After Provisioning

- Verify WiFi credentials are correct
- Check WiFi network is in range
- Check serial output for connection errors
- Try manual connection with saved credentials

### Provisioning Timeout

- Default timeout is 5 minutes (`IMPROV_WIFI_TIMEOUT_MS`)
- After timeout, device tries saved/hardcoded credentials
- To restart provisioning, wait for next connection failure or manually trigger

## Future Enhancements

- **BLE Support**: Add BLE-based provisioning (requires `ImprovWiFiBLE` class)
- **Web Portal**: Add WiFiManager as fallback option
- **UI Integration**: Show provisioning status on display
- **Manual Trigger**: Add button/command to manually start provisioning

## References

- [Improv WiFi Protocol](https://www.improv-wifi.com/)
- [Improv WiFi Serial Protocol](https://www.improv-wifi.com/serial/)
- [Improv WiFi Library](https://github.com/jnthas/Improv-WiFi-Library)
