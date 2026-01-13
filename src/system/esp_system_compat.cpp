/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * ESP System Compatibility Layer Implementation
 */

#include "system/esp_system_compat.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_efuse.h"
#include "sdkconfig.h"
#include <string.h>

const char* ESPClass::getChipModel() {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    static char model[32];
    switch (chip_info.model) {
        case CHIP_ESP32:
            snprintf(model, sizeof(model), "ESP32");
            break;
        case CHIP_ESP32S2:
            snprintf(model, sizeof(model), "ESP32-S2");
            break;
        case CHIP_ESP32S3:
            snprintf(model, sizeof(model), "ESP32-S3");
            break;
        case CHIP_ESP32C3:
            snprintf(model, sizeof(model), "ESP32-C3");
            break;
        case CHIP_ESP32C2:
            snprintf(model, sizeof(model), "ESP32-C2");
            break;
        case CHIP_ESP32C6:
            snprintf(model, sizeof(model), "ESP32-C6");
            break;
        case CHIP_ESP32H2:
            snprintf(model, sizeof(model), "ESP32-H2");
            break;
        default:
            snprintf(model, sizeof(model), "Unknown ESP");
            break;
    }
    return model;
}

uint8_t ESPClass::getChipRevision() {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    return chip_info.revision;
}

uint32_t ESPClass::getCpuFreqMHz() {
    // Use config value for CPU frequency
    return CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;
}

uint32_t ESPClass::getFlashChipSize() {
    uint32_t flash_size = 0;
    esp_flash_get_size(NULL, &flash_size);
    return flash_size;
}

uint32_t ESPClass::getFreeHeap() {
    return esp_get_free_heap_size();
}

void ESPClass::restart() {
    esp_restart();
}

// Define the ESP object
ESPClass ESP;

// Get SOC UID (unique chip identifier from flash chip)
bool get_soc_uid_string(char* uid_string, size_t uid_string_size) {
    if (uid_string == NULL || uid_string_size < 17) {
        return false;  // Need at least 17 bytes (16 hex chars + null for 64-bit UID)
    }
    
    // ESP32 flash chip unique ID is 64 bits (8 bytes)
    uint64_t chip_id = 0;
    esp_err_t ret = esp_flash_read_unique_chip_id(NULL, &chip_id);
    
    if (ret == ESP_OK && chip_id != 0) {
        // Format as hex string (16 hex characters for 64-bit)
        snprintf(uid_string, uid_string_size, "%016llX", (unsigned long long)chip_id);
        return true;
    }
    
    // Fallback to MAC address if SOC UID read fails
    uint8_t mac[6];
    ret = esp_efuse_mac_get_default(mac);
    if (ret == ESP_OK) {
        snprintf(uid_string, uid_string_size, "%02X%02X%02X%02X%02X%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return true;
    }
    
    return false;
}
