#!/usr/bin/env python3
"""
Patch memory_layout_utils.c to filter zero-length regions
This is a workaround for ESP-IDF bug causing boot loop with BLE
"""

import os
import sys

def patch_memory_layout_utils():
    # Find the file in the build directory or framework
    framework_path = os.path.expanduser("~/.platformio/packages/framework-espidf")
    file_path = os.path.join(framework_path, "components/heap/port/memory_layout_utils.c")
    
    if not os.path.exists(file_path):
        print(f"Warning: {file_path} not found, patch may not apply")
        return
    
    # Read the file
    with open(file_path, 'r') as f:
        content = f.read()
    
    # Check if already patched
    if "Filter out zero-length regions" in content:
        print("File already patched")
        return
    
    # Find the insertion point (after memcpy, before qsort)
    insertion_point = "memcpy(reserved, &soc_reserved_memory_region_start, count * sizeof(soc_reserved_region_t));\n#endif\n\n    /* Sort by starting address */"
    
    patch_code = """memcpy(reserved, &soc_reserved_memory_region_start, count * sizeof(soc_reserved_region_t));
#endif

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

    /* Sort by starting address */"""
    
    if insertion_point in content:
        content = content.replace(insertion_point, patch_code)
        
        # Write back
        with open(file_path, 'w') as f:
            f.write(content)
        print(f"Patched {file_path}")
    else:
        print(f"Could not find insertion point in {file_path}")
        print("Patch may need manual application")

if __name__ == "__main__":
    patch_memory_layout_utils()
