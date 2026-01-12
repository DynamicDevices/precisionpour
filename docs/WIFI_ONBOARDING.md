# WiFi Onboarding Options

This document describes available WiFi onboarding solutions for the PrecisionPour ESP32 firmware.

## Current Implementation

The firmware currently uses **hardcoded WiFi credentials** stored in `include/secrets.h`. This requires:
- Compiling credentials into the firmware
- Recompiling and reflashing to change WiFi networks
- Not ideal for production deployment

## Available Solutions

### 1. Improv WiFi (BLE-based) ⭐ Recommended

**Protocol**: [Improv WiFi](https://www.improv-wifi.com/) - Open standard for WiFi provisioning

**How it works**:
- Device broadcasts BLE advertisement when WiFi not configured
- User connects via Improv WiFi mobile app (iOS/Android)
- App sends WiFi credentials over BLE
- Device connects to WiFi and provides status

**Pros**:
- ✅ No hardcoded credentials needed
- ✅ Works with standard mobile app (no custom app required)
- ✅ Professional, standardized protocol
- ✅ Works even if device has no display/keyboard
- ✅ Secure BLE communication

**Cons**:
- ⚠️ Requires BLE support (ESP32 has this)
- ⚠️ User needs mobile app installed
- ⚠️ May need library implementation

**Libraries**:
- Official Improv WiFi protocol implementation needed
- May need to implement from [specification](https://www.improv-wifi.com/ble/)
- Or use ESP-IDF's built-in Improv support (if using ESP-IDF framework)

**Implementation Status**: Not yet implemented - would require adding BLE support

---

### 2. WiFiManager (Web-based Captive Portal)

**Library**: [WiFiManager by tzapu](https://github.com/tzapu/WiFiManager)

**How it works**:
- If WiFi connection fails, device creates AP (Access Point)
- Device hosts web-based configuration portal
- User connects to device's WiFi network
- User opens browser (captive portal redirects automatically)
- User selects WiFi network and enters password
- Device saves credentials and connects

**Pros**:
- ✅ No hardcoded credentials needed
- ✅ Works with any device with WiFi and browser
- ✅ Mature, well-tested library
- ✅ Easy to implement
- ✅ Credentials stored in EEPROM/SPIFFS

**Cons**:
- ⚠️ Requires user to connect to device's AP first
- ⚠️ Two-step process (connect to device, then configure)
- ⚠️ Less intuitive than BLE provisioning

**PlatformIO Installation**:
```ini
lib_deps = 
    tzapu/WiFiManager@^2.0.16
```

**Implementation Status**: Not yet implemented - library available

---

### 3. ESP-IDF WiFi Provisioning (Official)

**Framework**: ESP-IDF (not Arduino)

**How it works**:
- ESP-IDF includes built-in WiFi provisioning
- Supports BLE, SoftAP, and Console provisioning
- Official Espressif solution

**Pros**:
- ✅ Official Espressif solution
- ✅ Multiple provisioning methods
- ✅ Well-documented
- ✅ Production-ready

**Cons**:
- ⚠️ Requires ESP-IDF framework (not Arduino)
- ⚠️ Would require significant code rewrite
- ⚠️ More complex than Arduino libraries

**Implementation Status**: Not applicable (using Arduino framework)

---

### 4. AutoConnect

**Library**: [AutoConnect by hieromon](https://github.com/hieromon/AutoConnect)

**How it works**:
- Similar to WiFiManager
- Creates AP with web portal
- More customizable UI

**Pros**:
- ✅ Customizable web interface
- ✅ More features than WiFiManager
- ✅ Good documentation

**Cons**:
- ⚠️ Similar limitations to WiFiManager
- ⚠️ Larger library size

**PlatformIO Installation**:
```ini
lib_deps = 
    hieromon/AutoConnect@^1.4.0
```

**Implementation Status**: Not yet implemented - library available

---

## Recommendation

For **production deployment**, we recommend:

1. **Short-term**: Implement **WiFiManager** (easiest, web-based)
   - Quick to implement
   - Works with existing Arduino framework
   - No mobile app required
   - Credentials stored in EEPROM/SPIFFS

2. **Long-term**: Implement **Improv WiFi** (BLE-based)
   - More professional solution
   - Better user experience
   - Standardized protocol
   - Requires BLE implementation

## Implementation Plan

### Phase 1: WiFiManager (Quick Win)
1. Add WiFiManager library to `platformio.ini`
2. Modify `wifi_manager.cpp` to use WiFiManager
3. Fallback to AP mode if no saved credentials
4. Store credentials in EEPROM/SPIFFS
5. Test with captive portal

### Phase 2: Improv WiFi (Future Enhancement)
1. Research Improv WiFi Arduino implementation
2. Add BLE support to firmware
3. Implement Improv WiFi protocol
4. Test with Improv WiFi mobile app
5. Fallback to WiFiManager if BLE unavailable

## Current Status

- ✅ Hardcoded credentials working
- ⏳ WiFiManager not yet implemented
- ⏳ Improv WiFi not yet implemented
- ⏳ No credential storage (EEPROM/SPIFFS)

## Next Steps

To implement WiFi onboarding:

1. **Choose solution** (WiFiManager recommended for quick implementation)
2. **Add library** to `platformio.ini`
3. **Modify `wifi_manager.cpp`** to use onboarding library
4. **Add credential storage** (EEPROM or SPIFFS)
5. **Test provisioning flow**
6. **Update UI** to show provisioning status
7. **Document** for end users

## References

- [Improv WiFi Protocol](https://www.improv-wifi.com/)
- [WiFiManager Library](https://github.com/tzapu/WiFiManager)
- [AutoConnect Library](https://github.com/hieromon/AutoConnect)
- [ESP-IDF WiFi Provisioning](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/provisioning.html)
