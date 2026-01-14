# Zero-Length Memory Region Analysis

**Date:** 2026-01-14  
**Issue:** Zero-length memory region `0x3ff82000 - 0x3ff82000` causing boot loop

## Root Cause

**Zero-Length Region:**
- Start: `0x3ff82000`
- End: `0x3ff82000` (same as start - zero length!)

**Source in Code:**
From `memory_layout_utils.c`:
```c
#if ESP_ROM_HAS_LAYOUT_TABLE
    const ets_rom_layout_t *layout = ets_rom_layout_p;
    reserved[0].start = (intptr_t)layout->dram0_rtos_reserved_start;
#ifdef SOC_DIRAM_ROM_RESERVE_HIGH
    reserved[0].end = SOC_DIRAM_ROM_RESERVE_HIGH;
#else
    reserved[0].end = SOC_DIRAM_DRAM_HIGH;
#endif
```

**Problem:**
The first reserved region comes from the ROM layout table. If `dram0_rtos_reserved_start` equals `SOC_DIRAM_ROM_RESERVE_HIGH` (or `SOC_DIRAM_DRAM_HIGH`), we get a zero-length region.

## Why This Happens

### 1. ROM Layout Table Issue
- The ROM layout table (`ets_rom_layout_t`) may have incorrect or uninitialized values
- `dram0_rtos_reserved_start` might equal the end address
- This could be an ESP-IDF version bug or configuration issue

### 2. SOC Configuration
- `SOC_DIRAM_ROM_RESERVE_HIGH` or `SOC_DIRAM_DRAM_HIGH` might be incorrectly defined
- These constants should define the end of the reserved region, but may equal the start

### 3. ESP-IDF Version Bug
- ESP-IDF 5.5.0 may have a bug with ROM layout detection
- Similar issues reported in GitHub issues #4929, #12189

## Online Findings

### Known Issues:
1. **Memory Region Assertion Failures:** Similar issues reported in ESP-IDF GitHub
2. **BLE Memory Release:** Improper use of `esp_bt_controller_mem_release()` can cause memory region issues
3. **Overlapping Regions:** Overlapping memory regions can result in zero-length regions after adjustment

### Recommendations from Community:
1. **Update ESP-IDF:** Use latest stable version with bug fixes
2. **Review Memory Release Calls:** Ensure proper sequence of BLE initialization/deinitialization
3. **Check Configuration:** Verify BLE and memory configuration settings

## Impact

The zero-length region causes the assertion to fail:
```
assert failed: s_prepare_reserved_regions memory_layout_utils.c:88 
(reserved[i + 1].start > reserved[i].start)
```

After `qsort()`, if two regions have the same start address (including the zero-length region), the validation fails because the assertion requires strictly ascending start addresses.

## Possible Solutions

### 1. Filter Zero-Length Regions
Modify `s_prepare_reserved_regions()` to skip zero-length regions:
```c
// Filter out zero-length regions before sorting
size_t valid_count = 0;
for (size_t i = 0; i < count; i++) {
    if (reserved[i].start < reserved[i].end) {
        reserved[valid_count++] = reserved[i];
    }
}
count = valid_count;
```

### 2. Fix Comparison Function
Modify comparison to handle equal start addresses:
```c
static int s_compare_reserved_regions(const void *a, const void *b)
{
    const soc_reserved_region_t *r_a = (soc_reserved_region_t *)a;
    const soc_reserved_region_t *r_b = (soc_reserved_region_t *)b;
    if (r_a->start == r_b->start) {
        // Use end address as tiebreaker
        return (int)r_a->end - (int)r_b->end;
    }
    return (int)r_a->start - (int)r_b->start;
}
```

### 3. Fix Assertion
Change assertion to allow equal start addresses (if regions are non-overlapping):
```c
// Allow equal starts if ends are also equal (zero-length regions)
assert(reserved[i + 1].start > reserved[i].start || 
       (reserved[i + 1].start == reserved[i].start && 
        reserved[i + 1].end == reserved[i].end));
```

### 4. Check ROM Layout Table
Verify that `layout->dram0_rtos_reserved_start` is valid and not equal to the end address.

### 5. ESP-IDF Update
Check for ESP-IDF updates that fix this issue, or try a different version.

## Next Steps

1. **Check ESP-IDF Version:** Verify if this is a known bug in 5.5.0
2. **Implement Workaround:** Patch `memory_layout_utils.c` to handle zero-length regions
3. **Report Bug:** If confirmed as ESP-IDF bug, report to Espressif
4. **Test Fix:** Verify that filtering zero-length regions resolves the boot loop
