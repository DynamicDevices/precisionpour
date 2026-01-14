# Zero-Length Memory Region Workaround

**Date:** 2026-01-14  
**Issue:** Boot loop caused by zero-length memory region assertion failure

## Workaround Implemented

### Patch Applied
Created `scripts/patch_memory_layout.py` that patches `memory_layout_utils.c` to filter out zero-length regions before sorting and validation.

**Location:** `components/heap/port/memory_layout_utils.c`

**Patch Code:**
```c
/* Filter out zero-length regions before sorting (workaround for ESP-IDF bug) */
size_t valid_count = 0;
for (size_t i = 0; i < count; i++) {
    if (reserved[i].start < reserved[i].end) {
        if (valid_count != i) {
            reserved[valid_count] = reserved[i];
        }
        valid_count++;
    } else {
        ESP_EARLY_LOGW(TAG, "Filtering out zero-length reserved region: 0x%08x - 0x%08x",
                       reserved[i].start, reserved[i].end);
    }
}
count = valid_count;
```

**How It Works:**
1. Before sorting, filters out any regions where `start >= end` (zero-length)
2. Logs a warning when filtering
3. Updates the count to reflect only valid regions
4. Then proceeds with normal sorting and validation

### Integration
- Added to `platformio.ini`: `extra_scripts = pre:scripts/patch_memory_layout.py`
- Runs automatically before each build
- Patches the framework file in place

## Current Status

✅ **Patch Applied:** Zero-length region filter is in place  
❌ **DRAM Overflow:** Separate issue - BLE uses too much memory (7944 bytes over)

## Next Steps

1. **Test Patch:** Temporarily disable BLE to verify patch fixes boot loop
2. **Address Memory:** Enable SPIRAM or optimize memory usage for BLE
3. **Verify:** Once memory issue resolved, test full Improv WiFi functionality

## Files Modified

- `scripts/patch_memory_layout.py` - Patch script
- `platformio.ini` - Added extra_scripts
- `patches/memory_layout_utils.patch` - Standard patch file (alternative method)
