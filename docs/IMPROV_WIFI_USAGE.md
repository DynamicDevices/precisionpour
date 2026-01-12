# Improv WiFi Usage Guide

This document explains how to use Improv WiFi provisioning with the PrecisionPour firmware.

## Overview

Improv WiFi allows you to configure WiFi credentials without hardcoding them in the firmware. The device uses **Bluetooth Low Energy (BLE)** to receive WiFi credentials through the Improv WiFi mobile app.

## How It Works

1. **Initial Boot**: Device tries to connect using saved credentials (if available) or hardcoded credentials from `secrets.h`
2. **Connection Failure**: If connection fails, Improv WiFi BLE provisioning starts automatically
3. **Provisioning**: Device advertises via BLE and accepts WiFi credentials through the Improv WiFi mobile app
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

### Using Improv WiFi Mobile App

1. **Download App**: Install "Improv WiFi" app on iOS or Android
   - iOS: [App Store](https://apps.apple.com/app/improv-wifi/id1641444767)
   - Android: [Google Play](https://play.google.com/store/apps/details?id=com.improvwifiapp)
2. **Enable Bluetooth**: Ensure Bluetooth is enabled on your phone
3. **Start Provisioning**: Device automatically starts BLE advertising when WiFi connection fails
4. **Open App**: Launch Improv WiFi app
5. **Scan for Device**: App will scan and detect "PrecisionPour" device
6. **Select Device**: Tap on "PrecisionPour" in the device list
7. **Configure WiFi**: Enter WiFi SSID and password in the app
8. **Connect**: Device receives credentials via BLE and connects automatically

## Behavior

### Normal Operation (WiFi Connected)

- Device uses saved credentials (if available) or hardcoded credentials
- Connects to WiFi automatically on boot
- No provisioning needed

### Provisioning Mode (WiFi Not Connected)

- Device enters provisioning mode automatically after connection failure
- Serial output shows: `[Improv WiFi BLE] Starting BLE provisioning...`
- Device advertises via BLE as "PrecisionPour"
- Device accepts credentials for 5 minutes (configurable timeout)
- After timeout, device stops BLE advertising and tries saved/hardcoded credentials again

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
[Improv WiFi BLE] Starting BLE provisioning...
[Improv WiFi BLE] BLE initialized
[Improv WiFi BLE] BLE provisioning active
[Improv WiFi BLE] Device advertising as 'PrecisionPour'
[Improv WiFi BLE] Connect with Improv WiFi mobile app
```

When credentials are received:

```
[Improv WiFi BLE] Received credentials for: YourWiFiNetwork
[WiFi] Connecting to: YourWiFiNetwork
[WiFi] Connected!
[WiFi] IP address: 192.168.1.100
[WiFi] Saved credentials for: YourWiFiNetwork
[Improv WiFi BLE] Provisioning successful!
[Improv WiFi BLE] Provisioning stopped - WiFi connected, BLE deinitialized
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
