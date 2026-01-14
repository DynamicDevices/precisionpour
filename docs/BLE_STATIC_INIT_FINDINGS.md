# BLE Component Static Initialization Findings

**Date:** 2026-01-14  
**Investigation:** What static initialization code is causing the crash?

## Static Initializers Found in BLE Component

### 1. Static Arrays (Lookup Tables)
- `static const uint8_t inv_sbox[256]` - AES decryption lookup table
- `static const uint8_t sbox[256]` - AES encryption lookup table  
- `static const unsigned int k256[64]` - SHA256 constants
- `static const btc_func_t profile_tab[BTC_PID_NUM]` - Function pointer table
- `static uint8_t s_hci_driver_uart_rx_data[CONFIG_BT_LE_HCI_RX_PROC_DATA_LEN]` - UART RX buffer

**Analysis:** These are simple constant arrays or function pointer tables. Should be safe for static initialization.

### 2. Static Variables
- `static DRAM_ATTR esp_bt_controller_status_t btdm_controller_status = ESP_BT_CONTROLLER_STATUS_IDLE`
- `static ble_npl_count_info_t g_ctrl_npl_info = {...}`
- Various static structs initialized with zeros or constants

**Analysis:** Simple enum/struct initializations. Should be safe.

### 3. Memory Allocation Functions
- `btc_init_mem()` calls `osi_calloc()` which allocates memory
- `btc_init()` calls `btc_init_mem()`
- `esp_bluedroid_init()` calls `btc_init()`

**Analysis:** These are called at runtime, not during static initialization. Should not be the issue.

### 4. Constructor Functions
- Found `control_constructor` in binary (likely from LVGL, not BLE)
- No `__attribute__((constructor))` functions found in BLE component
- No `ESP_SYSTEM_INIT` functions found

**Analysis:** No constructor functions in BLE component that would run during static init.

## The Mystery

The crash happens **before `app_main()` runs**, which means it's during static initialization. However:

1. ✅ No constructor functions found
2. ✅ Static arrays are simple constants (should be safe)
3. ✅ Memory allocation only happens at runtime
4. ❌ **But the crash still occurs**

## Possible Causes

### Hypothesis 1: Static Array Size Overflow
Large static arrays might overflow available memory during static initialization, but:
- Most arrays are small (< 1KB)
- Largest found: `native_flash_sectors[1024 * 1024 / 2048]` (512KB) - but this is in test code, not compiled

### Hypothesis 2: Static Initializer Accessing Hardware
Static initializers might access hardware before it's ready, but:
- No hardware access found in static initializers
- All hardware access is in runtime functions

### Hypothesis 3: Stack Overflow During Static Init
Large static arrays might cause stack overflow, but:
- Static arrays are in `.data` or `.rodata` sections, not stack
- Stack overflow would show in logs

### Hypothesis 4: BLE Component Linking Issue
When `CONFIG_BT_ENABLED=y`, ESP-IDF links the entire BLE component, which might have:
- Dependencies on other components that aren't initialized
- Static initializers that access uninitialized resources
- Code that runs during binary loading

## Next Steps

1. **Enable Panic Backtraces**: Check if we can get more info about the crash
2. **Check Component Dependencies**: See if BLE component depends on something that needs initialization
3. **Check Linker Scripts**: See if BLE component has special linker requirements
4. **Try Disabling BLE Features**: See if disabling specific BLE features (like BLE Mesh) helps

## Key Finding

The `profile_tab` static array contains function pointers. While the array itself is safe, if any of those handler functions have static initializers that access uninitialized resources, that could cause issues.

However, function pointers themselves don't call functions - they're just addresses. So this shouldn't be the issue.

## Conclusion

The static initialization code in the BLE component appears safe. The crash must be from:
1. A dependency issue (BLE component depends on something not initialized)
2. A linker issue (BLE component code placement causes problems)
3. An ESP-IDF framework issue (BLE component initialization order problem)
4. Something else entirely
