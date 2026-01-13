/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * Screen Manager Implementation
 * 
 * Manages screen state transitions and lifecycle
 */

// Project headers
#include "config.h"
#include "screen_manager.h"
#include "qr_code_screen.h"
#include "pouring_screen.h"
#include "finished_screen.h"
#include "flow_meter.h"

// System/Standard library headers
#include <lvgl.h>

// ESP-IDF framework headers
#include <esp_log.h>
#define TAG "screen_mgr"

// Project compatibility headers
#include "esp_idf_compat.h"

// Current screen state
static ScreenState current_state = SCREEN_SPLASH;

// Pouring parameters (stored for finished screen)
static float pouring_final_volume_ml = 0.0;
static float pouring_final_cost = 0.0;
static char pouring_currency[8] = {0};

// Forward declaration for callback
static void pouring_screen_switch_callback();

void screen_manager_init() {
    ESP_LOGI(TAG, "[Screen Manager] Initializing screen manager...");
    current_state = SCREEN_SPLASH;
    ESP_LOGI(TAG, "[Screen Manager] Screen manager initialized (state: SPLASH)");
}

ScreenState screen_manager_get_state() {
    return current_state;
}

void screen_manager_show_qr_code() {
    if (current_state == SCREEN_QR_CODE) {
        ESP_LOGW(TAG, "[Screen Manager] Already on QR code screen");
        return;
    }
    
    ESP_LOGI(TAG, "[Screen Manager] Transitioning to QR code screen...");
    
    // Clean up previous screen
    switch (current_state) {
        case SCREEN_POURING:
            pouring_screen_cleanup();
            break;
        case SCREEN_FINISHED:
            finished_screen_cleanup();
            break;
        default:
            break;
    }
    
    // Initialize QR code screen
    qr_code_screen_init();
    current_state = SCREEN_QR_CODE;
    
    ESP_LOGI(TAG, "[Screen Manager] Now on QR code screen");
}

void screen_manager_show_pouring(const char* unique_id, float cost_per_ml, int max_ml, const char* currency) {
    ESP_LOGI(TAG, "[Screen Manager] Transitioning to pouring screen...");
    
    // Store currency for finished screen
    if (currency != NULL) {
        strncpy(pouring_currency, currency, sizeof(pouring_currency) - 1);
        pouring_currency[sizeof(pouring_currency) - 1] = '\0';
    } else {
        pouring_currency[0] = '\0';
    }
    
    // Clean up previous screen
    if (current_state == SCREEN_QR_CODE) {
        qr_code_screen_cleanup();
    } else if (current_state == SCREEN_FINISHED) {
        finished_screen_cleanup();
    }
    
    // Initialize pouring screen
    pouring_screen_init();
    
    // Set switch callback
    pouring_screen_set_switch_callback(pouring_screen_switch_callback);
    
    // Start pour with parameters
    pouring_screen_start_pour(unique_id, cost_per_ml, max_ml, currency);
    current_state = SCREEN_POURING;
    
    ESP_LOGI(TAG, "[Screen Manager] Now on pouring screen");
}

void screen_manager_show_finished(float final_volume_ml, float final_cost, const char* currency) {
    ESP_LOGI(TAG, "[Screen Manager] Transitioning to finished screen...");
    
    // Store final values
    pouring_final_volume_ml = final_volume_ml;
    pouring_final_cost = final_cost;
    if (currency != NULL) {
        strncpy(pouring_currency, currency, sizeof(pouring_currency) - 1);
        pouring_currency[sizeof(pouring_currency) - 1] = '\0';
    } else {
        pouring_currency[0] = '\0';
    }
    
    // Clean up pouring screen
    if (current_state == SCREEN_POURING) {
        pouring_screen_cleanup();
    }
    
    // Initialize finished screen
    finished_screen_init(final_volume_ml, final_cost, currency);
    current_state = SCREEN_FINISHED;
    
    ESP_LOGI(TAG, "[Screen Manager] Now on finished screen");
}

void screen_manager_update() {
    switch (current_state) {
        case SCREEN_QR_CODE:
            qr_code_screen_update();
            break;
            
        case SCREEN_POURING:
            pouring_screen_update();
            
            // Check if pouring is complete (max volume reached)
            if (pouring_screen_is_max_reached()) {
                // Get final values
                float volume_liters = flow_meter_get_total_volume_liters();
                float volume_ml = volume_liters * 1000.0;
                
                // Calculate final cost
                float cost_per_ml = pouring_screen_get_cost_per_ml();
                float final_cost = volume_ml * cost_per_ml;
                
                // Get currency symbol (we'll need to add a getter or store it)
                // For now, use the stored currency
                const char* currency = (strlen(pouring_currency) > 0) ? pouring_currency : CURRENCY_SYMBOL;
                
                // Transition to finished screen
                ESP_LOGI(TAG, "[Screen Manager] Pouring complete, transitioning to finished screen");
                screen_manager_show_finished(volume_ml, final_cost, currency);
            }
            break;
            
        case SCREEN_FINISHED:
            // Check if timeout has elapsed
            if (finished_screen_update()) {
                // Timeout elapsed, return to QR code screen
                ESP_LOGI(TAG, "[Screen Manager] Finished screen timeout, returning to QR code screen");
                screen_manager_show_qr_code();
            }
            break;
            
        case SCREEN_SPLASH:
            // Splash screen is handled separately in main.cpp
            break;
    }
}

void screen_manager_cleanup() {
    // Clean up current screen
    switch (current_state) {
        case SCREEN_QR_CODE:
            qr_code_screen_cleanup();
            break;
        case SCREEN_POURING:
            pouring_screen_cleanup();
            break;
        case SCREEN_FINISHED:
            finished_screen_cleanup();
            break;
        default:
            break;
    }
    
    current_state = SCREEN_SPLASH;
    ESP_LOGI(TAG, "[Screen Manager] Screen manager cleaned up");
}

// Callback function for pouring screen to switch back to QR code screen
static void pouring_screen_switch_callback() {
    ESP_LOGI(TAG, "[Screen Manager] Pouring screen requested switch to QR code");
    screen_manager_show_qr_code();
}
