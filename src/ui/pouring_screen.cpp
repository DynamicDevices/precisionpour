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
#include "images/pouring_illustration.h"
#include "utils/rle_decompress.h"

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
static lv_obj_t* root = NULL;
static lv_obj_t* title_label = NULL;
static lv_obj_t* illustration_img_obj = NULL;
static lv_obj_t* budget_left_value = NULL;
static lv_obj_t* progress_bar = NULL;
static lv_obj_t* progress_label = NULL;
static lv_obj_t* flow_rate_value = NULL;
static lv_obj_t* unit_cost_value = NULL;
static lv_obj_t* bottom_summary_label = NULL;
static lv_obj_t* action_hint_label = NULL;

// Pouring parameters (from MQTT "paid" command)
static char pour_unique_id[64] = {0};
static float cost_per_ml = 0.0;
static int max_ml = 0;
static bool pour_active = false;
static char currency_symbol[8] = {0};
static float max_budget_cost = 0.0f;  // Derived from cost_per_ml * max_ml

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
    
    // Root container for pouring screen (fits inside base_screen content area)
    root = lv_obj_create(content_area);
    if (root == NULL) {
        ESP_LOGE(TAG, "[Pouring Screen] ERROR: Failed to create root container!");
        return;
    }
    lv_obj_set_size(root, lv_obj_get_width(content_area), lv_obj_get_height(content_area));
    lv_obj_align(root, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(root, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(root, pouring_screen_touch_cb, LV_EVENT_CLICKED, NULL);

    // Bottom summary: poured + cost so far (place on the main screen so it's truly at the bottom)
    // Put it between the WiFi/data icons.
    bottom_summary_label = lv_label_create(lv_scr_act());
    {
        char init_str[48];
        const char* symbol = (strlen(currency_symbol) > 0) ? currency_symbol : CURRENCY_SYMBOL;
        snprintf(init_str, sizeof(init_str), "0 ml • %s0.00", symbol);
        lv_label_set_text(bottom_summary_label, init_str);
    }
    lv_obj_set_style_text_color(bottom_summary_label, COLOR_GOLDEN, 0);
    lv_obj_set_style_text_font(bottom_summary_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(bottom_summary_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_clear_flag(bottom_summary_label, LV_OBJ_FLAG_CLICKABLE);
    lv_label_set_long_mode(bottom_summary_label, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(bottom_summary_label, DISPLAY_WIDTH - 2 * (BASE_SCREEN_ICON_SIZE + BASE_SCREEN_ICON_MARGIN + 8));
    lv_obj_align(bottom_summary_label, LV_ALIGN_BOTTOM_MID, 0, -2);

    // Illustration (beer icon) centered in remaining space above summary
    illustration_img_obj = lv_img_create(root);
    if (illustration_img_obj != NULL) {
        const lv_img_dsc_t* img = rle_get_image(
            &pouring_illustration,
            POURING_ILLUSTRATION_IS_COMPRESSED,
            POURING_ILLUSTRATION_IS_COMPRESSED ? POURING_ILLUSTRATION_UNCOMPRESSED_SIZE : pouring_illustration.data_size
        );
        if (img != NULL) {
            lv_img_set_src(illustration_img_obj, img);
            // Centered in the content area (no manual offset)
            lv_obj_align(illustration_img_obj, LV_ALIGN_CENTER, DEBUG_POURING_ICON_OFFSET_X, 0);
        } else {
            lv_obj_add_flag(illustration_img_obj, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // Keep screen interactive (tap handling stays the same), but don't show extra helper text for now
    action_hint_label = NULL;
    
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

    // Read sensors once per update
    const float volume_liters = flow_meter_get_total_volume_liters();
    const float volume_ml = volume_liters * 1000.0f;

    // Bottom summary (ml poured + cost)
    if (bottom_summary_label != NULL) {
        char summary_str[64];
        const char* symbol = (strlen(currency_symbol) > 0) ? currency_symbol : CURRENCY_SYMBOL;
        float total_cost = (pour_active && cost_per_ml > 0.0f) ? (volume_ml * cost_per_ml) : 0.0f;
        snprintf(summary_str, sizeof(summary_str), "%.0f ml • %s%.2f", volume_ml, symbol, total_cost);
        lv_label_set_text(bottom_summary_label, summary_str);
    }
}

void pouring_screen_reset() {
    pour_active = false;
    cost_per_ml = 0.0;
    max_ml = 0;
    pour_unique_id[0] = '\0';
    currency_symbol[0] = '\0';
    max_budget_cost = 0.0f;
    
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
    max_budget_cost = (max_ml > 0 && cost_per_ml > 0.0f) ? (cost_per_ml * (float)max_ml) : 0.0f;

    // If UI is already constructed, refresh the bottom summary immediately
    if (bottom_summary_label != NULL) {
        char init_str[64];
        const char* symbol = (strlen(currency_symbol) > 0) ? currency_symbol : CURRENCY_SYMBOL;
        snprintf(init_str, sizeof(init_str), "0 ml • %s0.00", symbol);
        lv_label_set_text(bottom_summary_label, init_str);
    }
    
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

    // If we created a label on lv_scr_act(), delete it explicitly (base_screen_cleanup won't touch it)
    if (bottom_summary_label != NULL) {
        lv_obj_del(bottom_summary_label);
        bottom_summary_label = NULL;
    }

    // Content is owned by base_screen content_area; deleting it cleans up children.
    // We just NULL out our pointers.
    root = NULL;
    title_label = NULL;
    illustration_img_obj = NULL;
    budget_left_value = NULL;
    progress_bar = NULL;
    progress_label = NULL;
    flow_rate_value = NULL;
    unit_cost_value = NULL;
    action_hint_label = NULL;
    
    // Clean up base screen (content area only, shared components persist)
    base_screen_cleanup();
    
    pouring_screen_active = false;
    
    ESP_LOGI(TAG, "[Pouring Screen] Pouring Screen cleaned up");
}
