# BLE Component Static Initialization Analysis

**Date:** 2026-01-14  
**Issue:** Device crashes before `app_main()` when `CONFIG_BT_ENABLED=y`

## Analysis Approach

We need to identify what static initialization code in the BLE component is causing the crash.

## What We Know

1. **Crash Timing:** Before `app_main()` runs - during static initialization
2. **Trigger:** `CONFIG_BT_ENABLED=y` causes ESP-IDF to link BLE component
3. **Behavior:** Device enters boot loop, no application logs appear
4. **Our Code:** Moving our static initializers to runtime didn't fix it

## What to Check

### 1. Static Variables with Initializers
- Large static arrays that might overflow
- Static structs with function pointers
- Static variables that call functions during initialization

### 2. Constructor Functions
- `__attribute__((constructor))` functions
- `__attribute__((init_priority))` functions
- Functions in `.init_array` section

### 3. Static Initializers That Access Resources
- Heap allocation during static init
- NVS access during static init
- Hardware access before initialization
- Stack overflow during static init

### 4. BLE Component Initialization Order
- What gets initialized when `CONFIG_BT_ENABLED=y`?
- Are there dependencies on other components?
- Is heap/NVS required before BLE component init?

## Investigation Commands

```bash
# Find static initializers in BLE component
find ~/.platformio/packages/framework-espidf/components/bt -name "*.c" | xargs grep "static.*="

# Find constructor functions
grep -r "__attribute__.*constructor" ~/.platformio/packages/framework-espidf/components/bt

# Find large static arrays
find ~/.platformio/packages/framework-espidf/components/bt -name "*.c" | xargs grep "static.*\[.*[0-9]{3,}"

# Check initialization functions
nm .pio/build/esp32dev-idf/firmware.elf | grep -i "init" | grep -i bt

# Check init_array section
objdump -s -j .init_array .pio/build/esp32dev-idf/firmware.elf
```

## Next Steps

1. Identify the specific static initializer causing the crash
2. Check if it accesses uninitialized resources (heap, NVS, hardware)
3. Determine if there's a workaround or if it's an ESP-IDF bug
4. Report to ESP-IDF if it's a framework issue
