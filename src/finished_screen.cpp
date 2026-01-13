/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * Finished Screen Implementation
 * 
 * Displays completion message with final volume and cost
 * Uses base_screen for standard layout
 */

// Project headers
#include "config.h"
#include "finished_screen.h"
#include "base_screen.h"

// System/Standard library headers
#include <lvgl.h>
#include <string.h>

// ESP-IDF framework headers
#include <esp_log.h>
#define TAG "finished"

// Project compatibility headers
#include "esp_idf_compat.h"

// Brand colors
#define COLOR_TEXT lv_color_hex(0xFFFFFF) // White
#define COLOR_GOLDEN lv_color_hex(0xFFD700) // Golden yellow

// UI objects
static lv_obj_t* message_label = NULL;
static lv_obj_t* volume_label = NULL;
static lv_obj_t* volume_value = NULL;
static lv_obj_t* cost_label = NULL;
static lv_obj_t* cost_value = NULL;
static lv_obj_t* timeout_label = NULL;

// Timeout configuration
#define FINISHED_SCREEN_TIMEOUT_MS 5000  // 5 seconds timeout

// State
static unsigned long finished_screen_start_time = 0;
static bool finished_screen_active = false;

void finished_screen_init(float final_volume_ml, float final_cost, const char* currency) {
    ESP_LOGI(TAG, "\n=== Initializing Finished Screen ===");
    
    // Create base screen layout (logo, WiFi icon, data icon)
    lv_obj_t* content_area = base_screen_create(lv_scr_act());
    if (content_area == NULL) {
        ESP_LOGE(TAG, "[Finished Screen] ERROR: Failed to create base screen!");
        return;
    }
    
    // Create completion message
    message_label = lv_label_create(content_area);
    if (message_label != NULL) {
        lv_label_set_text(message_label, "Pour Complete!");
        lv_obj_set_style_text_color(message_label, COLOR_GOLDEN, 0);
        lv_obj_set_style_text_font(message_label, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_align(message_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(message_label, LV_ALIGN_TOP_MID, 0, 20);
    }
    
    // Create volume display
    volume_label = lv_label_create(content_area);
    if (volume_label != NULL) {
        lv_label_set_text(volume_label, "Volume:");
        lv_obj_set_style_text_color(volume_label, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(volume_label, &lv_font_montserrat_14, 0);
        lv_obj_align(volume_label, LV_ALIGN_CENTER, 0, -30);
    }
    
    volume_value = lv_label_create(content_area);
    if (volume_value != NULL) {
        char volume_str[32];
        snprintf(volume_str, sizeof(volume_str), "%.0f ml", final_volume_ml);
        lv_label_set_text(volume_value, volume_str);
        lv_obj_set_style_text_color(volume_value, COLOR_GOLDEN, 0);
        lv_obj_set_style_text_font(volume_value, &lv_font_montserrat_14, 0);
        lv_obj_align(volume_value, LV_ALIGN_CENTER, 0, -10);
    }
    
    // Create cost display
    cost_label = lv_label_create(content_area);
    if (cost_label != NULL) {
        lv_label_set_text(cost_label, "Total Cost:");
        lv_obj_set_style_text_color(cost_label, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(cost_label, &lv_font_montserrat_14, 0);
        lv_obj_align(cost_label, LV_ALIGN_CENTER, 0, 20);
    }
    
    cost_value = lv_label_create(content_area);
    if (cost_value != NULL) {
        char cost_str[32];
        const char* symbol = (currency != NULL && strlen(currency) > 0) ? currency : CURRENCY_SYMBOL;
        snprintf(cost_str, sizeof(cost_str), "%s%.2f", symbol, final_cost);
        lv_label_set_text(cost_value, cost_str);
        lv_obj_set_style_text_color(cost_value, COLOR_GOLDEN, 0);
        lv_obj_set_style_text_font(cost_value, &lv_font_montserrat_14, 0);
        lv_obj_align(cost_value, LV_ALIGN_CENTER, 0, 40);
    }
    
    // Create timeout countdown label (optional, can be removed if not needed)
    timeout_label = lv_label_create(content_area);
    if (timeout_label != NULL) {
        lv_label_set_text(timeout_label, "Returning to payment...");
        lv_obj_set_style_text_color(timeout_label, lv_color_hex(0x808080), 0);  // Gray
        lv_obj_set_style_text_font(timeout_label, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_align(timeout_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(timeout_label, LV_ALIGN_BOTTOM_MID, 0, -10);
    }
    
    // Record start time for timeout
    finished_screen_start_time = millis();
    finished_screen_active = true;
    
    // Force refresh
    lv_timer_handler();
    delay(10);
    lv_timer_handler();
    
    ESP_LOGI(TAG, "[Finished Screen] Finished Screen initialized");
    ESP_LOGI(TAG, "  Final Volume: %.0f ml", final_volume_ml);
    ESP_LOGI(TAG, "  Final Cost: %s%.2f", (currency != NULL ? currency : CURRENCY_SYMBOL), final_cost);
}

bool finished_screen_update() {
    if (!finished_screen_active) {
        return false;
    }
    
    // Update base screen (WiFi and data icons)
    base_screen_update();
    
    // Check if timeout has elapsed
    unsigned long now = millis();
    unsigned long elapsed = now - finished_screen_start_time;
    
    if (elapsed >= FINISHED_SCREEN_TIMEOUT_MS) {
        ESP_LOGI(TAG, "[Finished Screen] Timeout elapsed, ready to return to QR code screen");
        finished_screen_active = false;
        return true;  // Signal that we should transition to QR code screen
    }
    
    // Update timeout countdown (optional)
    if (timeout_label != NULL) {
        unsigned long remaining = (FINISHED_SCREEN_TIMEOUT_MS - elapsed) / 1000;
        char timeout_str[64];
        if (remaining > 0) {
            snprintf(timeout_str, sizeof(timeout_str), "Returning in %lu...", remaining);
        } else {
            snprintf(timeout_str, sizeof(timeout_str), "Returning...");
        }
        lv_label_set_text(timeout_label, timeout_str);
    }
    
    return false;  // Timeout not yet elapsed
}

void finished_screen_cleanup() {
    // Set inactive first to prevent updates during cleanup
    finished_screen_active = false;
    
    // Clean up labels
    if (message_label != NULL) {
        lv_obj_del(message_label);
        message_label = NULL;
    }
    
    if (volume_label != NULL) {
        lv_obj_del(volume_label);
        volume_label = NULL;
    }
    
    if (volume_value != NULL) {
        lv_obj_del(volume_value);
        volume_value = NULL;
    }
    
    if (cost_label != NULL) {
        lv_obj_del(cost_label);
        cost_label = NULL;
    }
    
    if (cost_value != NULL) {
        lv_obj_del(cost_value);
        cost_value = NULL;
    }
    
    if (timeout_label != NULL) {
        lv_obj_del(timeout_label);
        timeout_label = NULL;
    }
    
    // Clean up base screen (content area only, shared components persist)
    base_screen_cleanup();
    
    ESP_LOGI(TAG, "[Finished Screen] Finished Screen cleaned up");
}
