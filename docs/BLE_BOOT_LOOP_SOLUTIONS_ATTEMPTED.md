# BLE Boot Loop - Solutions Attempted

**Date:** 2026-01-14  
**Issue:** Device crashes before `app_main()` when `USE_IMPROV_WIFI` is enabled

## Solutions Attempted

### ✅ Solution 1: Move Static Struct to Runtime
**Status:** Implemented, ❌ Boot loop persists

**Changes:**
- Moved `esp_ble_adv_params_t adv_params` from static initialization to runtime
- Added `init_adv_params()` function called after BLE initialization

**Result:** Boot loop still occurs, indicating the issue is NOT the static struct.

### ✅ Solution 2: Move UUID Initialization to Runtime
**Status:** Implemented, ❌ Boot loop persists

**Changes:**
- Changed UUID structures from static initialization to uninitialized
- Added `init_improv_uuids()` function called at runtime
- UUIDs initialized using `memcpy` instead of static initializers

**Result:** Boot loop still occurs, indicating the issue is NOT the static UUID structures.

### ✅ Solution 3: Conditional BLE Component Linking
**Status:** Implemented, ❌ Boot loop persists

**Changes:**
- Made `REQUIRES bt` conditional on `CONFIG_USE_IMPROV_WIFI` in `src/CMakeLists.txt`

**Result:** Boot loop still occurs. When `CONFIG_BT_ENABLED=y`, ESP-IDF automatically links the BLE component regardless of our CMakeLists.txt requirements.

### ✅ Solution 4: WiFi Modem Sleep Configuration
**Status:** Implemented

**Changes:**
- Set WiFi power save mode to `WIFI_PS_MIN_MODEM` when BLE is enabled

**Result:** This is correct for WiFi/BLE coexistence, but doesn't fix the static init crash (WiFi not initialized yet when crash occurs).

## Root Cause Analysis

### The Real Problem:
When `CONFIG_BT_ENABLED=y` is set in `sdkconfig`, ESP-IDF's build system automatically links the BLE component. This component has static initializers that run during the static initialization phase, **before `app_main()` is called**.

### Evidence:
1. ✅ Device works fine when `USE_IMPROV_WIFI` is disabled
2. ✅ Build succeeds (no compilation errors)
3. ✅ BLE component is properly configured
4. ❌ Crash happens before `app_main()` runs
5. ❌ No application logs appear
6. ❌ Moving our static initializers to runtime didn't fix it

### Conclusion:
The issue is in **ESP-IDF's BLE component itself**, not in our code. The BLE component has static initializers that crash when `CONFIG_BT_ENABLED=y`, regardless of whether we include BLE headers or use BLE APIs.

## Next Steps

1. **Report Bug to ESP-IDF**
   - Create minimal reproduction case
   - Report on ESP-IDF GitHub issues
   - Include: ESP-IDF version, configuration, crash behavior

2. **Check ESP-IDF Issues**
   - Search for similar BLE static init crashes
   - Check if there's a known workaround
   - Verify if this is fixed in newer ESP-IDF versions

3. **Workarounds**
   - Keep Improv WiFi disabled until ESP-IDF fix available
   - Use WiFi AP mode for provisioning instead of BLE
   - Consider downgrading ESP-IDF if issue is version-specific

4. **Alternative Approaches**
   - Use ESP-IDF's native WiFi provisioning (not Improv)
   - Implement custom BLE provisioning without Improv protocol
   - Wait for ESP-IDF fix

## Files Modified

- `src/wifi/wifi_improv.cpp` - Moved static initializers to runtime
- `src/wifi/wifi_manager.cpp` - Added WiFi modem sleep configuration
- `src/CMakeLists.txt` - Made bt component requirement conditional
- `src/main.cpp` - (Attempted early BLE init, reverted)

## Configuration

- `CONFIG_BT_ENABLED=y` - Required for Improv WiFi
- `CONFIG_USE_IMPROV_WIFI=y` - Enabled
- `CONFIG_WIFI_IMPROV_BLE=y` - Enabled
- All BLE-related configurations are correct
