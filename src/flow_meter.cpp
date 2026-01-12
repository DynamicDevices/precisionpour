/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * Flow Meter Implementation
 * 
 * YF-S201 Hall Effect Flow Sensor reading and calculations
 */

// Project headers
#include "config.h"
#include "flow_meter.h"

// System/Standard library headers
// ESP-IDF framework headers
#include <driver/gpio.h>
#include <esp_log.h>
#define TAG "flow_meter"

// Project compatibility headers
#include "esp_idf_compat.h"

// Flow meter constants
#define PULSES_PER_LITER 450        // YF-S201 outputs 450 pulses per liter
#define PULSES_PER_LPM 7.5          // 450 pulses/L / 60 seconds = 7.5 pulses per L/min per Hz
#define CALCULATION_INTERVAL_MS 1000  // Calculate flow rate every 1 second

// Flow meter variables
static volatile unsigned long pulse_count = 0;  // Total pulse count (interrupt-safe)
static unsigned long last_pulse_count = 0;     // Pulse count at last calculation
static unsigned long last_calculation_time = 0; // Last time we calculated flow rate
static float current_flow_rate_lpm = 0.0;      // Current flow rate in L/min
static float total_volume_liters = 0.0;        // Total volume in liters
static unsigned long last_pulse_time = 0;       // Time of last pulse (for debouncing)

// Interrupt service routine - called on each pulse from flow meter
void IRAM_ATTR flow_meter_isr() {
    unsigned long current_time = millis();
    
           // Debounce: ignore pulses that come too quickly (< 10ms apart)
           // This prevents false readings from electrical noise
           if (current_time - last_pulse_time > 10) {
               // Use atomic increment to avoid volatile increment warning
               unsigned long temp = pulse_count;
               temp++;
               pulse_count = temp;
               last_pulse_time = current_time;
           }
}

void flow_meter_init() {
    ESP_LOGI(TAG, "\n=== Initializing Flow Meter ===");
    
    // Configure flow meter pin as input with pull-up
    pinMode(FLOW_METER_PIN, INPUT | INPUT_PULLUP_FLAG);
    
    // Attach interrupt to count pulses
    // RISING edge: pulse goes from LOW to HIGH
    attachInterrupt(digitalPinToInterrupt(FLOW_METER_PIN), flow_meter_isr, RISING);
    
    // Initialize variables
    pulse_count = 0;
    last_pulse_count = 0;
    last_calculation_time = millis();
    current_flow_rate_lpm = 0.0;
    total_volume_liters = 0.0;
    
    ESP_LOGI(TAG, "Flow meter initialized on pin %d", FLOW_METER_PIN);
    ESP_LOGI(TAG, "Flow meter ready - waiting for flow...");
}

void flow_meter_update() {
    unsigned long current_time = millis();
    
    // Calculate flow rate every second
    if (current_time - last_calculation_time >= CALCULATION_INTERVAL_MS) {
        // Disable interrupts briefly to safely read pulse_count
        noInterrupts();
        unsigned long current_pulse_count = pulse_count;
        interrupts();
        
        // Calculate pulses in the last second
        unsigned long pulses_in_interval = current_pulse_count - last_pulse_count;
        
        // Calculate flow rate: Frequency (Hz) = pulses per second
        // Flow Rate (L/min) = Frequency (Hz) / 7.5
        float frequency_hz = (float)pulses_in_interval;  // Pulses per second
        current_flow_rate_lpm = frequency_hz / PULSES_PER_LPM;
        
        // Update total volume: add volume that flowed in this interval
        // Volume (L) = pulses / pulses_per_liter
        float volume_in_interval = (float)pulses_in_interval / PULSES_PER_LITER;
        total_volume_liters += volume_in_interval;
        
        // Update for next calculation
        last_pulse_count = current_pulse_count;
        last_calculation_time = current_time;
        
        // Debug output (can be removed or made conditional)
        if (current_flow_rate_lpm > 0.1) {  // Only print if there's significant flow
            ESP_LOGI(TAG, "Flow: %.2f L/min, Total: %.3f L, Pulses: %lu",
                     current_flow_rate_lpm, total_volume_liters, current_pulse_count);
        }
    }
    
    // If no pulses for 2 seconds, assume flow has stopped
    if (current_time - last_pulse_time > 2000 && current_flow_rate_lpm > 0) {
        current_flow_rate_lpm = 0.0;
    }
}

float flow_meter_get_flow_rate_lpm() {
    return current_flow_rate_lpm;
}

float flow_meter_get_total_volume_liters() {
    return total_volume_liters;
}

void flow_meter_reset_volume() {
    noInterrupts();
    pulse_count = 0;
    interrupts();
    last_pulse_count = 0;
    total_volume_liters = 0.0;
    ESP_LOGI(TAG, "Volume counter reset");
}

unsigned long flow_meter_get_pulse_count() {
    noInterrupts();
    unsigned long count = pulse_count;
    interrupts();
    return count;
}
