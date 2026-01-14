# BLE Boot Loop - Fix Attempt

**Date:** 2026-01-14  
**Issue:** Boot loop when `CONFIG_BT_ENABLED=y` and Improv WiFi enabled

## Root Cause Identified

**Exact Crash Point:**
```
assert failed: s_prepare_reserved_regions memory_layout_utils.c:88 
(reserved[i + 1].start > reserved[i].start)
```

**Location:** `components/esp_system/port/memory_layout_utils.c:88`

**Problem:** After sorting reserved memory regions with `qsort()`, the validation assertion fails because two regions have the same start address. This violates the assumption that all regions must have unique, ascending start addresses.

**Memory Regions Before Crash:**
- 0x3ff82000 - 0x3ff82000 (zero-length region - suspicious!)
- 0x3ffae000 - 0x3ffae6e0
- 0x3ffae6e0 - 0x3ffaff10 (starts where previous ends)
- 0x3ffb0000 - 0x3ffb6388
- ... (13 total regions)

**Comparison Function:**
```c
static int s_compare_reserved_regions(const void *a, const void *b)
{
    const soc_reserved_region_t *r_a = (soc_reserved_region_t *)a;
    const soc_reserved_region_t *r_b = (soc_reserved_region_t *)b;
    return (int)r_a->start - (int)r_b->start;
}
```

The comparison function only compares start addresses. If two regions have the same start address, `qsort()` may not guarantee their relative order, causing the assertion to fail.

## Possible Solutions

### 1. ESP-IDF Version Issue
- Current: ESP-IDF 5.5.0
- May be a known bug in this version
- Check for updates or known issues

### 2. Zero-Length Region Issue
- Region `0x3ff82000 - 0x3ff82000` has start == end
- This might be invalid or incorrectly defined
- Could be from ROM layout table (`ESP_ROM_HAS_LAYOUT_TABLE`)

### 3. Adjacent Regions
- Region ending at `0x3ffae6e0` and next starting at `0x3ffae6e0`
- These are adjacent (end == start), which might be valid
- But if sorting puts them in wrong order, assertion fails

### 4. BLE Memory Regions
- BLE component reserves 4 memory regions via `SOC_RESERVE_MEMORY_REGION`
- These might conflict with other reserved regions
- May need to adjust BLE controller configuration

## Next Steps

1. **Check ESP-IDF GitHub Issues:**
   - Search for "memory_layout_utils assert" or "reserved regions"
   - Look for similar issues in ESP-IDF 5.5.0

2. **Try ESP-IDF Update:**
   - Check if newer version fixes this
   - Or try slightly older stable version

3. **Workaround Options:**
   - Patch `memory_layout_utils.c` to handle equal start addresses
   - Filter out zero-length regions before sorting
   - Modify comparison function to use end address as tiebreaker

4. **BLE Configuration:**
   - Try different BLE controller settings
   - Check if disabling certain BLE features helps
   - Verify BLE memory region definitions

## Current Status

- ✅ Root cause identified
- ✅ Exact crash point located
- ❌ No fix implemented yet
- 🔍 Investigating ESP-IDF version and workarounds
