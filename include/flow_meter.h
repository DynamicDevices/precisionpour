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

#include <Arduino.h>

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
