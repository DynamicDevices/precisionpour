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
#include "flow/flow_meter.h"

// System/Standard library headers
// ESP-IDF framework headers
#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/portmacro.h>
#define TAG "flow_meter"

// Flow meter constants
#define PULSES_PER_LITER 450        // YF-S201 outputs 450 pulses per liter
#define PULSES_PER_LPM 7.5          // 450 pulses/L / 60 seconds = 7.5 pulses per L/min per Hz
#define CALCULATION_INTERVAL_MS 1000  // Calculate flow rate every 1 second

// Flow meter variables
static volatile uint64_t pulse_count = 0;  // Total pulse count (interrupt-safe)
static uint64_t last_pulse_count = 0;     // Pulse count at last calculation
static uint64_t last_calculation_time = 0; // Last time we calculated flow rate
static float current_flow_rate_lpm = 0.0;      // Current flow rate in L/min
static float total_volume_liters = 0.0;        // Total volume in liters
static volatile uint64_t last_pulse_time = 0;       // Time of last pulse (for debouncing)

// Interrupt service routine - called on each pulse from flow meter
void IRAM_ATTR flow_meter_isr(void* arg) {
    uint64_t current_time = esp_timer_get_time() / 1000ULL;
    
    // Debounce: ignore pulses that come too quickly (< 10ms apart)
    // This prevents false readings from electrical noise
    uint64_t last_pulse = last_pulse_time;
    if (current_time - last_pulse > 10) {
        // Use atomic increment to avoid volatile increment warning
        uint64_t temp = pulse_count;
        temp++;
        pulse_count = temp;
        last_pulse_time = current_time;
    }
}

// Wrapper function that matches voidFuncPtr signature (void (*)(void))
// This is called by gpio_isr_handler_wrapper
static void IRAM_ATTR flow_meter_isr_wrapper_void(void) {
    flow_meter_isr(NULL);
}

void flow_meter_init() {
    ESP_LOGI(TAG, "=== Initializing Flow Meter ===");
    
    // Configure flow meter pin as input with pull-up
    // Check if pin is input-only (GPIO34, GPIO35, GPIO36, GPIO39 on ESP32)
    bool is_input_only = (FLOW_METER_PIN == 34 || FLOW_METER_PIN == 35 || FLOW_METER_PIN == 36 || FLOW_METER_PIN == 39);
    
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << FLOW_METER_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    if (is_input_only) {
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    } else {
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    }
    io_conf.intr_type = GPIO_INTR_POSEDGE;  // RISING edge
    gpio_config(&io_conf);
    
    // Store handler in gpio_isr_handlers for the wrapper to call
    extern gpio_isr_handler_t gpio_isr_handlers[];
    extern void IRAM_ATTR gpio_isr_handler_wrapper(void* arg);
    
    // Store the wrapper in gpio_isr_handlers so gpio_isr_handler_wrapper can call it
    if (FLOW_METER_PIN < GPIO_NUM_MAX) {
        gpio_isr_handlers[FLOW_METER_PIN].pin = (gpio_num_t)FLOW_METER_PIN;
        gpio_isr_handlers[FLOW_METER_PIN].func = flow_meter_isr_wrapper_void;
        gpio_isr_handlers[FLOW_METER_PIN].type = GPIO_INTR_POSEDGE;
    }
    
    // Add ISR handler - use the wrapper function
    gpio_isr_handler_add((gpio_num_t)FLOW_METER_PIN, gpio_isr_handler_wrapper, (void*)(intptr_t)FLOW_METER_PIN);
    
    // Initialize variables
    pulse_count = 0;
    last_pulse_count = 0;
    last_calculation_time = esp_timer_get_time() / 1000ULL;
    current_flow_rate_lpm = 0.0;
    total_volume_liters = 0.0;
    
    ESP_LOGI(TAG, "Flow meter initialized on pin %d", FLOW_METER_PIN);
    ESP_LOGI(TAG, "Flow meter ready - waiting for flow...");
}

void flow_meter_update() {
    uint64_t current_time = esp_timer_get_time() / 1000ULL;
    
    // Calculate flow rate every second
    if (current_time - last_calculation_time >= CALCULATION_INTERVAL_MS) {
        // Disable interrupts briefly to safely read pulse_count
        portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
        portENTER_CRITICAL(&mux);
        uint64_t current_pulse_count = pulse_count;
        portEXIT_CRITICAL(&mux);
        
        // Calculate pulses in the last second
        uint64_t pulses_in_interval = current_pulse_count - last_pulse_count;
        
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
            ESP_LOGI(TAG, "Flow: %.2f L/min, Total: %.3f L, Pulses: %llu",
                     current_flow_rate_lpm, total_volume_liters, (unsigned long long)current_pulse_count);
        }
    }
    
    // If no pulses for 2 seconds, assume flow has stopped
    uint64_t last_pulse = last_pulse_time;
    if (current_time - last_pulse > 2000 && current_flow_rate_lpm > 0) {
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
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL(&mux);
    pulse_count = 0;
    portEXIT_CRITICAL(&mux);
    last_pulse_count = 0;
    total_volume_liters = 0.0;
    ESP_LOGI(TAG, "Volume counter reset");
}

uint64_t flow_meter_get_pulse_count() {
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL(&mux);
    uint64_t count = pulse_count;
    portEXIT_CRITICAL(&mux);
    return count;
}
