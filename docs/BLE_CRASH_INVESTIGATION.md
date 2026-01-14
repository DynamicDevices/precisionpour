# BLE Boot Loop Crash Investigation

**Date:** 2026-01-14  
**Issue:** Device crashes before `app_main()` when `CONFIG_BT_ENABLED=y`

## Investigation Summary

### Early Logging Attempts
- Added constructor functions with `esp_rom_printf` logging
- **Result:** No `[EARLY]` messages appear in boot logs
- **Conclusion:** Crash happens before our constructors run

### Static Initialization Analysis
- Checked BLE component static initializers
- Found static arrays (lookup tables), static variables, but no obvious issues
- No constructor functions found in BLE component
- Memory allocation only happens at runtime, not during static init

### Key Finding: Memory Region Reservation

**Critical Discovery:**
The BLE controller code uses `SOC_RESERVE_MEMORY_REGION` macros to reserve memory regions:

```c
// From bt/controller/esp32/bt.c
SOC_RESERVE_MEMORY_REGION(SOC_MEM_BT_EM_START,   SOC_MEM_BT_EM_BREDR_REAL_END,  rom_bt_em);
SOC_RESERVE_MEMORY_REGION(SOC_MEM_BT_BSS_START,  SOC_MEM_BT_BSS_END,            rom_bt_bss);
SOC_RESERVE_MEMORY_REGION(SOC_MEM_BT_MISC_START, SOC_MEM_BT_MISC_END,           rom_bt_misc);
SOC_RESERVE_MEMORY_REGION(SOC_MEM_BT_DATA_START, SOC_MEM_BT_DATA_END,           rom_bt_data);
```

**Hypothesis:**
These memory region reservations happen during linking/loading. If:
1. The memory regions conflict with other components
2. The memory regions aren't properly initialized
3. The linker script has issues with these regions

Then the crash could happen during binary loading or early initialization, before our constructors run.

### Next Steps to Investigate

1. **Check Linker Scripts:**
   - Look for `rom_bt_*` sections in linker scripts
   - Check if memory regions overlap
   - Verify memory region sizes

2. **Check Component Dependencies:**
   - Verify BLE component dependencies are initialized
   - Check for circular dependencies
   - Verify component initialization order

3. **Check Memory Layout:**
   - Verify BLE memory regions don't conflict with other components
   - Check if SPIRAM configuration conflicts
   - Verify heap configuration

4. **Enable Diagnostic Logging:**
   - Try enabling panic backtraces (though may not help if crash is before FreeRTOS)
   - Check if there's a way to log during binary loading
   - Check bootloader logs for errors

5. **Check ESP-IDF Version:**
   - Verify if this is a known issue in ESP-IDF 5.5.0
   - Check ESP-IDF GitHub for similar issues
   - Consider trying a different ESP-IDF version

## Current Status

- ✅ Static initializers appear safe
- ✅ No constructor functions causing issues
- ❌ Crash happens before our code runs
- 🔍 Investigating memory region reservations and linker issues

## Critical Discovery: Heap Initialization Order

**Key Finding:** Heap initialization happens during `do_core_init()`, which runs **BEFORE** `do_global_ctors()`.

**ESP-IDF Startup Sequence:**
1. `call_start_cpu0()` → `start_cpu0_default()`
2. `do_core_init()` - Calls component init functions (ESP_SYSTEM_INIT_STAGE_CORE)
   - **Heap component registers an ESP_SYSTEM_INIT function**
   - **This processes reserved memory regions (including BLE's)**
3. `do_global_ctors()` - Calls constructor functions (our logging)
4. `do_secondary_init()` - Calls secondary component init functions
5. `esp_startup_start_app()` - Calls `app_main()`

**Implication:**
If the crash happens during heap initialization when processing BLE's reserved memory regions, it would occur **before** our constructor functions run, explaining why we never see `[EARLY]` log messages.

**Next Steps:**
1. Check if BLE memory regions conflict with heap initialization
2. Verify memory region addresses are valid
3. Check if there's a way to add logging to heap initialization
4. Consider if this is a known ESP-IDF issue

## Reserved Memory Region Processing

**How Reserved Regions Are Processed:**

1. `SOC_RESERVE_MEMORY_REGION` creates static variables in `.reserved_memory_address` linker section
2. `s_get_num_reserved_regions()` counts regions in this section
3. `s_prepare_reserved_regions()` reads the regions from the linker section
4. `soc_get_available_memory_regions()` uses these to exclude reserved regions from heap

**BLE Reserved Regions Found:**
- `reserved_region_rom_bt_em` (BT EM region)
- `reserved_region_rom_bt_bss` (BT BSS region)
- `reserved_region_rom_bt_misc` (BT MISC region)
- `reserved_region_rom_bt_data` (BT DATA region)

**Likely Crash Point:**
The crash likely occurs when `s_prepare_reserved_regions()` processes BLE's reserved memory regions during heap initialization. This function:
1. Uses `memcpy` to copy regions from `soc_reserved_memory_region_start` to `soc_reserved_memory_region_end`
2. Sorts them by address using `qsort`
3. Validates them (checks for overlaps, invalid addresses)

**Possible Crash Causes:**
- Invalid memory addresses in reserved regions
- Linker section not properly initialized
- Memory region overlaps causing assertion failures
- Stack overflow during `qsort` or validation

**Conclusion:**
This is an ESP-IDF framework issue where processing BLE's reserved memory regions during heap initialization causes a crash before any application code runs. The crash occurs in `do_core_init()`, before `do_global_ctors()`, which is why our constructor functions never execute.

## Exact Crash Point Identified

**UPDATE (2026-01-14 16:21):** Found the exact assertion failure!

**Crash Details:**
```
assert failed: s_prepare_reserved_regions memory_layout_utils.c:88 
(reserved[i + 1].start > reserved[i].start)
```

**Location:** `s_prepare_reserved_regions()` in `memory_layout_utils.c` line 88

**Problem:** Reserved memory regions are not properly sorted by start address. The function expects regions to be in ascending order after sorting, but the assertion fails, indicating:
1. The sorting algorithm (`qsort`) is not working correctly
2. There are overlapping or invalid memory regions
3. The reserved memory region data structure is corrupted

**Memory Regions Seen Before Crash:**
- 0x3ff82000 - 0x3ff82000 (zero-length region?)
- 0x3ffae000 - 0x3ffae6e0
- 0x3ffae6e0 - 0x3ffaff10
- 0x3ffb0000 - 0x3ffb6388
- ... (13 total regions)

**Note:** The first region (0x3ff82000 - 0x3ff82000) has the same start and end address, which might be causing issues with the sorting/comparison logic.

## Resolution Status

**UPDATE (2026-01-14):** Device is now running successfully with BLE enabled!

**Current Status:**
- ✅ BLE is enabled (`CONFIG_BT_ENABLED=y`)
- ✅ BLE component is linked (`libbt.a` present in build)
- ✅ Device boots successfully
- ✅ No boot loop detected
- ✅ Verbose logging enabled (`CONFIG_LOG_DEFAULT_LEVEL=5`)

**Possible Explanations:**
1. Issue was intermittent and resolved itself
2. Configuration is now correct after re-enabling
3. Issue only occurs under specific conditions
4. Verbose logging may have helped (unlikely but possible)

**Verification:**
- BLE symbols present in firmware
- Device running normally with MQTT client active
- Need to verify Improv WiFi functionality

**Recommendation:**
- Test Improv WiFi functionality
- Monitor device for stability
- If issue recurs, verbose logging is enabled to capture details

## Current Investigation Status

**Re-enabled Improv WiFi and BLE:**
- `CONFIG_USE_IMPROV_WIFI=y`
- `CONFIG_WIFI_IMPROV_BLE=y`
- `CONFIG_BT_ENABLED=y`

**Enabled Verbose Logging:**
- `CONFIG_LOG_DEFAULT_LEVEL=5` (VERBOSE)
- This should enable `ESP_EARLY_LOGV` messages from heap initialization
- Should see detailed logging about reserved memory region processing

**Next Steps:**
1. Upload firmware and monitor logs
2. Look for heap initialization messages (ESP_EARLY_LOGD/ESP_EARLY_LOGV)
3. Identify exact crash point in `s_prepare_reserved_regions()` or `soc_get_available_memory_regions()`
4. Check if BLE memory region addresses are valid
5. Consider ESP-IDF version issues or workarounds
