# Comprehensive BLE Boot Loop Review

**Date:** 2026-01-14  
**Issue:** Device crashes before `app_main()` runs when `USE_IMPROV_WIFI` is enabled  
**Status:** Investigating

## Executive Summary

The device enters a boot loop when Improv WiFi BLE provisioning is enabled. The crash occurs during static initialization, before any application code runs. This suggests the issue is in the BLE component itself or in how BLE headers are included.

## 1. Configuration Analysis

### Current Configuration
- ✅ `CONFIG_BT_ENABLED=y` - BLE is enabled
- ✅ `CONFIG_USE_IMPROV_WIFI=y` - Improv WiFi support enabled
- ✅ `CONFIG_WIFI_IMPROV_BLE=y` - BLE-based Improv WiFi enabled
- ✅ `CONFIG_BT_BLUEDROID_ENABLED=y` - Bluedroid stack enabled
- ✅ `CONFIG_BT_BLE_ENABLED=y` - BLE enabled
- ✅ `CONFIG_BT_GATTS_ENABLE=y` - GATT server enabled
- ✅ `CONFIG_ESP_COEX_ENABLED=y` - WiFi/BLE coexistence enabled
- ✅ `CONFIG_BTDM_CTRL_MODEM_SLEEP=y` - BLE modem sleep enabled

### Component Dependencies
- ✅ `REQUIRES bt` in `src/CMakeLists.txt` - BLE component properly linked
- ✅ WiFi modem sleep configured (`WIFI_PS_MIN_MODEM` when BLE active)

## 2. Code Structure Analysis

### Static Initializers in `wifi_improv.cpp`

**Static UUID Structures (Lines 41-75):**
```cpp
static esp_bt_uuid_t improv_service_uuid = {
    .len = ESP_UUID_LEN_128,
    .uuid = {.uuid128 = {...}}
};
```
- ✅ These are simple data structures with compile-time constants
- ✅ Should be safe for static initialization

**Static Variables (Lines 99-110):**
```cpp
static uint16_t improv_gatts_if = 0xFF;  // ESP_GATT_IF_NONE
static uint16_t improv_conn_id = 0;
// ... more simple integer initializations
```
- ✅ All initialized with compile-time constants
- ✅ Should be safe

**Static Advertisement Parameters (Lines 382-391):**
```cpp
static esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    // ...
};
```
- ⚠️ **POTENTIAL ISSUE**: Uses BLE constants (`ADV_TYPE_IND`, `BLE_ADDR_TYPE_PUBLIC`, `ADV_CHNL_ALL`, etc.)
- ⚠️ These constants are defined in BLE headers
- ⚠️ If BLE headers have static initializers, this could trigger them

### BLE Header Inclusion (Lines 32-38)
```cpp
#if USE_IMPROV_WIFI
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_gatt_common_api.h"
```
- ⚠️ Headers are included at file scope
- ⚠️ If BLE headers have static initializers, they run during static init
- ⚠️ This happens before `app_main()` runs

## 3. Initialization Order

### Current Sequence:
1. **Static Initialization Phase** (before `app_main()`)
   - Static UUID structures initialized
   - Static `esp_ble_adv_params_t` initialized (uses BLE constants)
   - BLE headers included → **Potential crash point**
   - BLE component static initializers may run here

2. **`app_main()` Phase**
   - NVS initialization
   - GPIO ISR service
   - Watchdog initialization
   - Display/LVGL initialization
   - WiFi manager initialization
     - If `IMPROV_START_BY_DEFAULT`: calls `wifi_improv_start_provisioning()`
   - MQTT initialization

### Problem:
The crash occurs in phase 1, before any application code runs. This means:
- ❌ Not a runtime initialization issue
- ❌ Not a WiFi/BLE coexistence issue (WiFi not initialized yet)
- ✅ Likely a BLE component static initialization issue

## 4. Root Cause Hypothesis

### Most Likely Cause:
When `CONFIG_BT_ENABLED=y` is set and BLE headers are included, the BLE component may have static initializers that:
1. Access uninitialized system resources
2. Depend on initialization order
3. Have bugs in ESP-IDF 5.5.0

### Evidence:
- ✅ Device works fine when `USE_IMPROV_WIFI` is disabled
- ✅ Build succeeds (no compilation errors)
- ✅ BLE component is properly linked
- ❌ Crash happens before `app_main()` runs
- ❌ No application logs appear

## 5. Known Issues from Research

Based on web search results:
1. **WiFi Modem Sleep**: Must be enabled when BLE is active
   - ✅ **FIXED**: Code now sets `WIFI_PS_MIN_MODEM` when BLE enabled
   
2. **Static Initialization**: BLE components should be initialized at runtime
   - ✅ **ALREADY DONE**: BLE initialization happens in `wifi_improv_start_provisioning()`
   - ⚠️ **ISSUE**: Static struct initializers may still trigger BLE component init

3. **Initialization Order**: BLE stack must be initialized before use
   - ✅ **ALREADY DONE**: BLE is initialized before use
   - ⚠️ **ISSUE**: Static initializers may run before BLE stack is ready

## 6. Recommended Solutions

### Solution 1: Move Static Struct Initializers to Runtime ⭐ **RECOMMENDED**
**Action:** Initialize `esp_ble_adv_params_t` in `wifi_improv_start_provisioning()` instead of as a static variable.

**Pros:**
- Avoids static initialization entirely
- BLE constants only accessed after BLE is initialized
- Minimal code changes

**Cons:**
- Slight runtime overhead (negligible)

**Implementation:**
```cpp
// Remove static declaration
// static esp_ble_adv_params_t adv_params = {...};

// Initialize in wifi_improv_start_provisioning()
static void init_adv_params(esp_ble_adv_params_t* params) {
    memset(params, 0, sizeof(esp_ble_adv_params_t));
    params->adv_int_min = 0x20;
    params->adv_int_max = 0x40;
    params->adv_type = ADV_TYPE_IND;
    params->own_addr_type = BLE_ADDR_TYPE_PUBLIC;
    params->peer_addr_type = BLE_ADDR_TYPE_PUBLIC;
    params->channel_map = ADV_CHNL_ALL;
    params->adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;
}
```

### Solution 2: Check ESP-IDF BLE Component Initialization
**Action:** Verify if BLE component requires explicit initialization before headers are included.

**Research Needed:**
- Check ESP-IDF documentation for BLE initialization requirements
- Search ESP-IDF GitHub issues for similar problems
- Check if ESP-IDF 5.5.0 has known BLE static init bugs

### Solution 3: Delay BLE Header Inclusion
**Action:** Move BLE headers to be included only in functions that use them, using forward declarations where possible.

**Pros:**
- Completely avoids static initialization issues
- Headers only included when needed

**Cons:**
- More complex code structure
- May require significant refactoring
- Forward declarations may not be sufficient for all types

### Solution 4: Conditional BLE Component Linking
**Action:** Make BLE component linking conditional on `USE_IMPROV_WIFI`.

**Pros:**
- BLE component not linked if not needed
- Avoids static initialization entirely

**Cons:**
- Requires CMake conditional logic
- May break if other code depends on BLE

## 7. Testing Plan

1. **Test Solution 1** (Move static struct to runtime)
   - Remove static `esp_ble_adv_params_t`
   - Initialize in `wifi_improv_start_provisioning()`
   - Build and test

2. **If Solution 1 fails:**
   - Test Solution 3 (Delay header inclusion)
   - Check ESP-IDF issues for known bugs
   - Consider Solution 4 (Conditional linking)

3. **If all solutions fail:**
   - Report bug to ESP-IDF
   - Consider workaround: Keep Improv WiFi disabled until fix available

## 8. Code Issues Found

### Issue 1: Inconsistent ESP_GATT_IF_NONE Usage
**Location:** `src/wifi/wifi_improv.cpp` line 137
```cpp
if (improv_gatts_if != ESP_GATT_IF_NONE && improv_error_handle != 0) {
```
**Problem:** Uses `ESP_GATT_IF_NONE` constant while line 130 uses `0xFF` to avoid the constant.

**Fix:** Use `0xFF` consistently or ensure `ESP_GATT_IF_NONE` is safe for static init.

### Issue 2: Static esp_ble_adv_params_t Initialization
**Location:** `src/wifi/wifi_improv.cpp` lines 382-391
**Problem:** Static struct uses BLE constants that may trigger BLE component static init.

**Fix:** Move to runtime initialization (Solution 1).

## 9. Next Steps

1. ✅ **IMMEDIATE**: Implement Solution 1 (Move static struct to runtime)
2. ⏳ **IF NEEDED**: Test Solution 3 (Delay header inclusion)
3. ⏳ **RESEARCH**: Check ESP-IDF issues for BLE static init bugs
4. ⏳ **DOCUMENTATION**: Update code comments explaining the fix

## 10. Testing Results

### Solution 1 Implementation (Move Static Struct to Runtime)
**Status:** ✅ Implemented, ❌ Boot loop persists

**Changes Made:**
- Moved `esp_ble_adv_params_t adv_params` from static initialization to runtime
- Added `init_adv_params()` function called after BLE initialization
- Fixed inconsistent `ESP_GATT_IF_NONE` usage

**Result:** Boot loop still occurs, indicating the issue is NOT the static struct initialization.

### Updated Root Cause Hypothesis

Since moving the static struct to runtime didn't fix the issue, the problem is likely:

1. **BLE Component Static Initializers**: When `CONFIG_BT_ENABLED=y`, the BLE component itself may have static initializers that run during static initialization, regardless of whether we include BLE headers or use BLE APIs.

2. **BLE Headers Static Initializers**: The BLE headers (`esp_bt.h`, `esp_gatts_api.h`, etc.) may have static initializers that run when included, even if we don't use them.

3. **Static UUID Structures**: The static `esp_bt_uuid_t` structures use `ESP_UUID_LEN_128` constant, which might trigger BLE component initialization.

## 11. Next Steps

### Immediate Actions:
1. ⏳ **Test Solution 3**: Delay BLE header inclusion (move headers to function scope)
2. ⏳ **Research**: Check ESP-IDF GitHub issues for BLE static init crashes in v5.5.0
3. ⏳ **Alternative**: Consider if BLE component can be conditionally linked

### If All Solutions Fail:
- Report bug to ESP-IDF with minimal reproduction case
- Consider workaround: Keep Improv WiFi disabled until ESP-IDF fix available
- Alternative: Use different BLE provisioning method (e.g., WiFi AP mode)

## 12. Conclusion

The boot loop is caused by static initialization issues in the BLE component when `CONFIG_BT_ENABLED=y`. Moving static struct initializers to runtime did not fix the issue, indicating the problem is deeper in the BLE component itself.

**Current Status:** Issue persists. Next step is to test Solution 3 (delay BLE header inclusion) or investigate ESP-IDF BLE component static initialization requirements.
