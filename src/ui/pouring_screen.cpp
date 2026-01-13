/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * Pouring Screen Implementation
 * 
 * Displays flow rate, volume, and cost during active pouring
 * Uses base_screen for standard layout
 */

// Project headers
#include "config.h"
#include "ui/pouring_screen.h"
#include "ui/base_screen.h"
#include "ui/screen_manager.h"
#include "flow/flow_meter.h"

// System/Standard library headers
#include <lvgl.h>
#include <string.h>

// ESP-IDF framework headers
#include <esp_log.h>
#define TAG "pouring"

// ESP-IDF framework headers
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Brand colors
#define COLOR_TEXT lv_color_hex(0xFFFFFF) // White
#define COLOR_GOLDEN lv_color_hex(0xFFD700) // Golden yellow

// UI objects
static lv_obj_t* flow_rate_label = NULL;
static lv_obj_t* flow_rate_value = NULL;
static lv_obj_t* volume_label = NULL;
static lv_obj_t* volume_value = NULL;
static lv_obj_t* cost_per_unit_label = NULL;
static lv_obj_t* cost_per_unit_value = NULL;
static lv_obj_t* total_cost_label = NULL;
static lv_obj_t* total_cost_value = NULL;

// Pouring parameters (from MQTT "paid" command)
static char pour_unique_id[64] = {0};
static float cost_per_ml = 0.0;
static int max_ml = 0;
static bool pour_active = false;
static char currency_symbol[8] = {0};

// Callback function to switch back to QR code screen
static void (*screen_switch_callback)(void) = NULL;

// Screen active state
static bool pouring_screen_active = false;

// Forward declaration
static void pouring_screen_touch_cb(lv_event_t *e);

void pouring_screen_init() {
    ESP_LOGI(TAG, "=== Initializing Pouring Screen ===");
    
    // Log debug option status
    #ifdef DEBUG_POURING_TAP_TO_FINISHED
    ESP_LOGI(TAG, "[Pouring Screen] DEBUG_POURING_TAP_TO_FINISHED is defined, value: %d", DEBUG_POURING_TAP_TO_FINISHED);
    if (DEBUG_POURING_TAP_TO_FINISHED) {
        ESP_LOGI(TAG, "[Pouring Screen] Debug mode: Tap to finished screen enabled");
    }
    #else
    ESP_LOGI(TAG, "[Pouring Screen] DEBUG_POURING_TAP_TO_FINISHED is NOT defined");
    #endif
    
    pouring_screen_active = true;
    
    // Create base screen layout (logo, WiFi icon, data icon)
    lv_obj_t* content_area = base_screen_create(lv_scr_act());
    if (content_area == NULL) {
        ESP_LOGE(TAG, "[Pouring Screen] ERROR: Failed to create base screen!");
        return;
    }
    
    // Create pouring information labels in content area
    // Flow Rate
    flow_rate_label = lv_label_create(content_area);
    if (flow_rate_label != NULL) {
        lv_label_set_text(flow_rate_label, "Flow Rate:");
        lv_obj_set_style_text_color(flow_rate_label, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(flow_rate_label, &lv_font_montserrat_14, 0);
        lv_obj_align(flow_rate_label, LV_ALIGN_TOP_LEFT, 10, 10);
    }
    
    flow_rate_value = lv_label_create(content_area);
    if (flow_rate_value != NULL) {
        lv_label_set_text(flow_rate_value, "0.00 mL/min");
        lv_obj_set_style_text_color(flow_rate_value, COLOR_GOLDEN, 0);
        lv_obj_set_style_text_font(flow_rate_value, &lv_font_montserrat_14, 0);
        lv_obj_align(flow_rate_value, LV_ALIGN_TOP_LEFT, 10, 30);
    }
    
    // Volume
    volume_label = lv_label_create(content_area);
    if (volume_label != NULL) {
        lv_label_set_text(volume_label, "Volume:");
        lv_obj_set_style_text_color(volume_label, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(volume_label, &lv_font_montserrat_14, 0);
        lv_obj_align(volume_label, LV_ALIGN_TOP_LEFT, 10, 60);
    }
    
    volume_value = lv_label_create(content_area);
    if (volume_value != NULL) {
        lv_label_set_text(volume_value, "0 ml");
        lv_obj_set_style_text_color(volume_value, COLOR_GOLDEN, 0);
        lv_obj_set_style_text_font(volume_value, &lv_font_montserrat_14, 0);
        lv_obj_align(volume_value, LV_ALIGN_TOP_LEFT, 10, 80);
    }
    
    // Cost per Unit (per ml)
    cost_per_unit_label = lv_label_create(content_area);
    if (cost_per_unit_label != NULL) {
        lv_label_set_text(cost_per_unit_label, "Cost per ml:");
        lv_obj_set_style_text_color(cost_per_unit_label, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(cost_per_unit_label, &lv_font_montserrat_14, 0);
        lv_obj_align(cost_per_unit_label, LV_ALIGN_TOP_RIGHT, -10, 10);
    }
    
    cost_per_unit_value = lv_label_create(content_area);
    if (cost_per_unit_value != NULL) {
        char init_str[32];
        const char* symbol = (strlen(currency_symbol) > 0) ? currency_symbol : CURRENCY_SYMBOL;
        snprintf(init_str, sizeof(init_str), "%s0.0000", symbol);
        lv_label_set_text(cost_per_unit_value, init_str);
        lv_obj_set_style_text_color(cost_per_unit_value, COLOR_GOLDEN, 0);
        lv_obj_set_style_text_font(cost_per_unit_value, &lv_font_montserrat_14, 0);
        lv_obj_align(cost_per_unit_value, LV_ALIGN_TOP_RIGHT, -10, 30);
    }
    
    // Total Cost
    total_cost_label = lv_label_create(content_area);
    if (total_cost_label != NULL) {
        lv_label_set_text(total_cost_label, "Total Cost:");
        lv_obj_set_style_text_color(total_cost_label, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(total_cost_label, &lv_font_montserrat_14, 0);
        lv_obj_align(total_cost_label, LV_ALIGN_TOP_RIGHT, -10, 60);
    }
    
    total_cost_value = lv_label_create(content_area);
    if (total_cost_value != NULL) {
        char init_str[32];
        const char* symbol = (strlen(currency_symbol) > 0) ? currency_symbol : CURRENCY_SYMBOL;
        snprintf(init_str, sizeof(init_str), "%s0.00", symbol);
        lv_label_set_text(total_cost_value, init_str);
        lv_obj_set_style_text_color(total_cost_value, COLOR_GOLDEN, 0);
        lv_obj_set_style_text_font(total_cost_value, &lv_font_montserrat_14, 0);
        lv_obj_align(total_cost_value, LV_ALIGN_TOP_RIGHT, -10, 80);
    }
    
    // Add touch event handler to screen - tap anywhere to return to QR code screen
    lv_obj_add_event_cb(lv_scr_act(), pouring_screen_touch_cb, LV_EVENT_CLICKED, NULL);
    
    // Force refresh
    lv_timer_handler();
    vTaskDelay(pdMS_TO_TICKS(10));
    lv_timer_handler();
    
    ESP_LOGI(TAG, "[Pouring Screen] Pouring Screen initialized");
}

// Touch event callback for pouring screen - switch back to QR code screen on tap
// Or transition to finished screen if debug option is enabled
static void pouring_screen_touch_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_CLICKED) {
        #ifdef DEBUG_POURING_TAP_TO_FINISHED
        if (DEBUG_POURING_TAP_TO_FINISHED) {
            // Debug mode: transition to finished screen with current values
            ESP_LOGI(TAG, "[Pouring Screen] Debug: Screen tapped - transitioning to finished screen");
            
            // Get current volume and cost
            float volume_liters = flow_meter_get_total_volume_liters();
            float volume_ml = volume_liters * 1000.0;  // Convert to ml
            
            // Calculate total cost
            float total_cost = 0.0;
            if (pour_active && cost_per_ml > 0.0) {
                total_cost = volume_ml * cost_per_ml;
            } else {
                // Use test values if no pour is active
                total_cost = volume_ml * 0.005f;  // Default £0.005 per ml
            }
            
            // Get currency symbol
            const char* currency = (strlen(currency_symbol) > 0) ? currency_symbol : CURRENCY_SYMBOL;
            
            ESP_LOGI(TAG, "[Pouring Screen] Debug: Transitioning with volume=%.2f ml, cost=%.2f, currency=%s",
                     volume_ml, total_cost, currency);
            
            // Transition to finished screen
            screen_manager_show_finished(volume_ml, total_cost, currency);
            return;
        }
        #endif
        
        // Normal mode: switch back to QR code screen
        ESP_LOGI(TAG, "[Pouring Screen] Screen tapped - switching to QR code screen");
        
        // Call the callback to switch back to QR code screen
        if (screen_switch_callback != NULL) {
            screen_switch_callback();
        } else {
            ESP_LOGE(TAG, "[Pouring Screen] ERROR: screen_switch_callback is NULL!");
        }
    }
}

void pouring_screen_update() {
    // Only update if screen is active
    if (!pouring_screen_active) {
        return;
    }
    
    // Update base screen (WiFi and data icons)
    base_screen_update();
    
    // Update flow rate display (convert L/min to mL/min)
    if (flow_rate_value != NULL) {
        float flow_rate_lpm = flow_meter_get_flow_rate_lpm();
        float flow_rate_mlpm = flow_rate_lpm * 1000.0;  // Convert liters to milliliters
        char flow_str[32];
        snprintf(flow_str, sizeof(flow_str), "%.2f mL/min", flow_rate_mlpm);
        lv_label_set_text(flow_rate_value, flow_str);
    }
    
    // Update volume display (convert liters to milliliters)
    if (volume_value != NULL) {
        float volume_liters = flow_meter_get_total_volume_liters();
        float volume_ml = volume_liters * 1000.0;  // Convert liters to milliliters
        char volume_str[32];
        snprintf(volume_str, sizeof(volume_str), "%.0f ml", volume_ml);
        lv_label_set_text(volume_value, volume_str);
    }
    
    // Update cost per ml display
    if (cost_per_unit_value != NULL && pour_active) {
        char cost_str[32];
        const char* symbol = (strlen(currency_symbol) > 0) ? currency_symbol : CURRENCY_SYMBOL;
        snprintf(cost_str, sizeof(cost_str), "%s%.4f", symbol, cost_per_ml);
        lv_label_set_text(cost_per_unit_value, cost_str);
    }
    
    // Update total cost display (using cost per ml)
    if (total_cost_value != NULL && pour_active) {
        float volume_liters = flow_meter_get_total_volume_liters();
        float volume_ml = volume_liters * 1000.0;  // Convert to ml
        float total_cost = volume_ml * cost_per_ml;  // Cost = ml * cost_per_ml
        char cost_str[32];
        const char* symbol = (strlen(currency_symbol) > 0) ? currency_symbol : CURRENCY_SYMBOL;
        snprintf(cost_str, sizeof(cost_str), "%s%.2f", symbol, total_cost);
        lv_label_set_text(total_cost_value, cost_str);
        
        // Check if max ml reached
        if (volume_ml >= max_ml) {
            ESP_LOGW(TAG, "[Pouring Screen] Maximum volume reached!");
        }
    }
}

void pouring_screen_reset() {
    pour_active = false;
    cost_per_ml = 0.0;
    max_ml = 0;
    pour_unique_id[0] = '\0';
    currency_symbol[0] = '\0';
    
    // Reset flow meter volume
    flow_meter_reset_volume();
    
    ESP_LOGI(TAG, "[Pouring Screen] Pouring screen reset");
}

void pouring_screen_set_params(const char* unique_id, float cost_per_ml_param, int max_ml_param, const char* currency) {
    // Store parameters
    if (unique_id != NULL) {
        strncpy(pour_unique_id, unique_id, sizeof(pour_unique_id) - 1);
        pour_unique_id[sizeof(pour_unique_id) - 1] = '\0';
    }
    cost_per_ml = cost_per_ml_param;
    max_ml = max_ml_param;
    if (currency != NULL) {
        strncpy(currency_symbol, currency, sizeof(currency_symbol) - 1);
        currency_symbol[sizeof(currency_symbol) - 1] = '\0';
    } else {
        currency_symbol[0] = '\0';
    }
    
    pour_active = true;
    
    ESP_LOGI(TAG, "[Pouring Screen] Updated pour parameters:");
    ESP_LOGI(TAG, "  ID: %s", pour_unique_id);
    ESP_LOGI(TAG, "  Cost per ml: %s%.4f", currency_symbol, cost_per_ml);
    ESP_LOGI(TAG, "  Max ml: %d", max_ml);
    ESP_LOGI(TAG, "  Currency: %s", currency_symbol);
}

void pouring_screen_start_pour(const char* unique_id, float cost_per_ml_param, int max_ml_param, const char* currency) {
    // Reset flow meter first
    flow_meter_reset_volume();
    
    // Set parameters
    pouring_screen_set_params(unique_id, cost_per_ml_param, max_ml_param, currency);
    
    ESP_LOGI(TAG, "[Pouring Screen] Starting pour:");
    ESP_LOGI(TAG, "  ID: %s", pour_unique_id);
    ESP_LOGI(TAG, "  Cost per ml: %s%.4f", currency_symbol, cost_per_ml);
    ESP_LOGI(TAG, "  Max ml: %d", max_ml);
    ESP_LOGI(TAG, "  Currency: %s", currency_symbol);
}

bool pouring_screen_is_max_reached() {
    if (!pour_active) {
        return false;
    }
    
    float volume_liters = flow_meter_get_total_volume_liters();
    float volume_ml = volume_liters * 1000.0;
    return volume_ml >= max_ml;
}

void pouring_screen_set_switch_callback(void (*callback)(void)) {
    screen_switch_callback = callback;
}

float pouring_screen_get_cost_per_ml() {
    return cost_per_ml;
}

void pouring_screen_cleanup() {
    // Set inactive first to prevent updates during cleanup
    pouring_screen_active = false;
    
    // Clean up labels
    if (flow_rate_label != NULL) {
        lv_obj_del(flow_rate_label);
        flow_rate_label = NULL;
    }
    
    if (flow_rate_value != NULL) {
        lv_obj_del(flow_rate_value);
        flow_rate_value = NULL;
    }
    
    if (volume_label != NULL) {
        lv_obj_del(volume_label);
        volume_label = NULL;
    }
    
    if (volume_value != NULL) {
        lv_obj_del(volume_value);
        volume_value = NULL;
    }
    
    if (cost_per_unit_label != NULL) {
        lv_obj_del(cost_per_unit_label);
        cost_per_unit_label = NULL;
    }
    
    if (cost_per_unit_value != NULL) {
        lv_obj_del(cost_per_unit_value);
        cost_per_unit_value = NULL;
    }
    
    if (total_cost_label != NULL) {
        lv_obj_del(total_cost_label);
        total_cost_label = NULL;
    }
    
    if (total_cost_value != NULL) {
        lv_obj_del(total_cost_value);
        total_cost_value = NULL;
    }
    
    // Clean up base screen (content area only, shared components persist)
    base_screen_cleanup();
    
    pouring_screen_active = false;
    
    ESP_LOGI(TAG, "[Pouring Screen] Pouring Screen cleaned up");
}
