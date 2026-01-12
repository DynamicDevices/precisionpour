/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * Flow Meter Manager
 * 
 * Handles YF-S201 Hall Effect Flow Sensor reading and calculations
 * 
 * Specifications:
 * - Flow Rate Range: 1 to 30 liters per minute
 * - Pulses per Liter: 450
 * - Formula: Flow Rate (L/min) = Pulse Frequency (Hz) / 7.5
 * - Or: Frequency (Hz) = 7.5 * Flow rate (L/min)
 */

#ifndef FLOW_METER_H
#define FLOW_METER_H

// Include config.h first to get framework detection
#include "config.h"

// Use ESP_PLATFORM as primary detection (ESP-IDF always defines this)
#ifdef ESP_PLATFORM
    // ESP-IDF framework
    #ifndef CONFIG_TFT_MOSI
        // If CONFIG_TFT_MOSI not defined yet, include sdkconfig.h
        #ifdef __has_include
            #if __has_include("sdkconfig.h")
                #include "sdkconfig.h"
            #endif
        #endif
    #endif
    #ifdef CONFIG_TFT_MOSI
        #include "esp_idf_compat.h"
    #else
        // ESP-IDF but KConfig not loaded - use compat layer anyway
        #include "esp_idf_compat.h"
    #endif
    #include <stdint.h>
#endif

// Flow meter initialization
void flow_meter_init();

// Flow meter update (call in main loop)
void flow_meter_update();

// Get current flow rate in liters per minute
float flow_meter_get_flow_rate_lpm();

// Get total volume in liters (since last reset)
float flow_meter_get_total_volume_liters();

// Reset total volume counter
void flow_meter_reset_volume();

// Get pulse count (for debugging)
unsigned long flow_meter_get_pulse_count();

#endif // FLOW_METER_H
