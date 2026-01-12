/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * ESP System Compatibility Layer
 * Provides ESP.getChipModel() etc. compatibility for ESP-IDF
 */

#ifndef ESP_SYSTEM_COMPAT_H
#define ESP_SYSTEM_COMPAT_H

// ESP-IDF framework
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_mac.h"
#include "esp_efuse.h"
#include <stdio.h>

class ESPClass {
public:
    const char* getChipModel();
    uint8_t getChipRevision();
    uint32_t getCpuFreqMHz();
    uint32_t getFlashChipSize();
    uint32_t getFreeHeap();
    void restart();
};

extern ESPClass ESP;

// Helper function to get SOC UID (unique chip identifier)
// Returns true on success, false on failure
// uid_string must be at least 33 bytes (32 hex chars + null terminator)
bool get_soc_uid_string(char* uid_string, size_t uid_string_size);

#endif // ESP_SYSTEM_COMPAT_H
