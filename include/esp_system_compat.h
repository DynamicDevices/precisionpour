/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * ESP System Compatibility Layer
 * Provides ESP.getChipModel() etc. compatibility for ESP-IDF
 */

#ifndef ESP_SYSTEM_COMPAT_H
#define ESP_SYSTEM_COMPAT_H

#ifdef ESP_PLATFORM
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

#else
// Arduino framework - ESP object already exists
#endif

#endif // ESP_SYSTEM_COMPAT_H
