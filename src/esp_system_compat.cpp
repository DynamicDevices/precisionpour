/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * ESP System Compatibility Layer Implementation
 */

#ifdef ESP_PLATFORM
#include "esp_system_compat.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "sdkconfig.h"

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

#endif // ESP_PLATFORM
