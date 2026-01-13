/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * Shared Data Icon Component Implementation
 */

// Project headers
#include "config.h"
#include "ui_data_icon.h"

// System/Standard library headers
#include <lvgl.h>

// ESP-IDF framework headers
#include <esp_log.h>
#define TAG "ui_data"

// Project compatibility headers
#include "esp_idf_compat.h"

// Static data icon objects (shared across all screens)
static lv_obj_t* data_container = NULL;
static lv_obj_t* data_spark1 = NULL;
static lv_obj_t* data_spark2 = NULL;
static lv_obj_t* data_spark3 = NULL;

// Data icon state
static bool data_active = false;
static unsigned long last_activity_time = 0;
static const unsigned long ACTIVITY_TIMEOUT_MS = 500;  // Show activity for 500ms after last activity

// Flash animation state (for TCP/IP activity indication)
static bool flash_state = false;  // Current flash state (true = visible, false = hidden)
static unsigned long last_flash_toggle = 0;
static const unsigned long FLASH_INTERVAL_MS = 200;  // Flash every 200ms (5 times per second)

lv_obj_t* ui_data_icon_create(lv_obj_t* parent) {
    if (data_container != NULL) {
        ESP_LOGW(TAG, "[Data Icon] Data icon already exists, returning existing container");
        return data_container;
    }
    
    if (parent == NULL) {
        ESP_LOGE(TAG, "[Data Icon] ERROR: Parent object is NULL!");
        return NULL;
    }
    
    ESP_LOGI(TAG, "[Data Icon] Creating shared data icon component...");
    
    // Create container for communication icon (20x20 pixels)
    data_container = lv_obj_create(parent);
    if (data_container == NULL) {
        ESP_LOGE(TAG, "[Data Icon] ERROR: Failed to create data container!");
        return NULL;
    }
    
    lv_obj_set_size(data_container, 20, 20);
    lv_obj_align(data_container, LV_ALIGN_BOTTOM_RIGHT, -5, -5);
    lv_obj_set_style_bg_opa(data_container, LV_OPA_TRANSP, 0);  // Transparent background
    lv_obj_set_style_border_width(data_container, 0, 0);
    lv_obj_set_style_pad_all(data_container, 0, 0);
    lv_obj_clear_flag(data_container, LV_OBJ_FLAG_SCROLLABLE);
    
    // Create spark/pulse elements (zigzag pattern like electric pulse)
    // Spark 1 (left, small)
    data_spark1 = lv_obj_create(data_container);
    lv_obj_set_size(data_spark1, 2, 6);
    lv_obj_set_style_bg_opa(data_spark1, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(data_spark1, lv_color_hex(0x808080), 0);  // Gray initially (no activity)
    lv_obj_set_style_border_width(data_spark1, 0, 0);
    lv_obj_set_style_radius(data_spark1, 1, 0);
    lv_obj_align(data_spark1, LV_ALIGN_LEFT_MID, 4, 0);
    
    // Spark 2 (center, medium) - main pulse
    data_spark2 = lv_obj_create(data_container);
    lv_obj_set_size(data_spark2, 3, 10);
    lv_obj_set_style_bg_opa(data_spark2, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(data_spark2, lv_color_hex(0x808080), 0);  // Gray initially (no activity)
    lv_obj_set_style_border_width(data_spark2, 0, 0);
    lv_obj_set_style_radius(data_spark2, 1, 0);
    lv_obj_align(data_spark2, LV_ALIGN_CENTER, 0, 0);
    
    // Spark 3 (right, small)
    data_spark3 = lv_obj_create(data_container);
    lv_obj_set_size(data_spark3, 2, 6);
    lv_obj_set_style_bg_opa(data_spark3, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(data_spark3, lv_color_hex(0x808080), 0);  // Gray initially (no activity)
    lv_obj_set_style_border_width(data_spark3, 0, 0);
    lv_obj_set_style_radius(data_spark3, 1, 0);
    lv_obj_align(data_spark3, LV_ALIGN_RIGHT_MID, -4, 0);
    
    lv_timer_handler();
    
    ESP_LOGI(TAG, "[Data Icon] Shared data icon component created successfully");
    return data_container;
}

lv_obj_t* ui_data_icon_get_container() {
    return data_container;
}

void ui_data_icon_set_active(bool active) {
    data_active = active;
    if (active) {
        last_activity_time = millis();
    }
}

void ui_data_icon_update(bool connected, bool active) {
    if (data_container == NULL || data_spark1 == NULL || 
        data_spark2 == NULL || data_spark3 == NULL) {
        return;  // Icon not created yet
    }
    
    unsigned long now = millis();
    
    // Update activity state
    if (active) {
        data_active = true;
        last_activity_time = now;
    } else {
        // Check if activity timeout has passed
        if (now - last_activity_time > ACTIVITY_TIMEOUT_MS) {
            data_active = false;
            flash_state = false;  // Reset flash state when activity stops
        }
    }
    
    // Flash animation when there's TCP/IP activity
    if (data_active && connected) {
        // Toggle flash state at regular intervals
        if (now - last_flash_toggle >= FLASH_INTERVAL_MS) {
            flash_state = !flash_state;
            last_flash_toggle = now;
        }
    } else {
        // No flashing when inactive or disconnected
        flash_state = true;  // Always visible when not flashing
    }
    
    // Determine color based on connection and activity
    lv_color_t icon_color;
    if (connected) {
        if (data_active) {
            icon_color = lv_color_hex(0x00FF00);  // Green when active
        } else {
            icon_color = lv_color_hex(0x808080);  // Gray when connected but idle
        }
    } else {
        icon_color = lv_color_hex(0xFF0000);  // Red when disconnected
    }
    
    // Update spark colors
    lv_obj_set_style_bg_color(data_spark1, icon_color, 0);
    lv_obj_set_style_bg_color(data_spark2, icon_color, 0);
    lv_obj_set_style_bg_color(data_spark3, icon_color, 0);
    
    // Set opacity based on flash state (flash on/off when active)
    lv_opa_t opacity;
    if (data_active && connected) {
        // Flash animation: toggle between visible and hidden
        opacity = flash_state ? LV_OPA_COVER : LV_OPA_TRANSP;
    } else if (connected) {
        // Connected but idle: show at reduced opacity
        opacity = LV_OPA_60;
    } else {
        // Disconnected: show at full opacity (red)
        opacity = LV_OPA_COVER;
    }
    
    lv_obj_set_style_opa(data_spark1, opacity, 0);
    lv_obj_set_style_opa(data_spark2, opacity, 0);
    lv_obj_set_style_opa(data_spark3, opacity, 0);
    
    // Invalidate to trigger redraw
    lv_obj_invalidate(data_container);
}

void ui_data_icon_cleanup() {
    // Note: We don't delete the data icon here because it's shared
    // The icon should persist across screen transitions
    // Only cleanup if we're completely shutting down the UI
    if (data_container != NULL) {
        ESP_LOGI(TAG, "[Data Icon] Data icon cleanup called (icon persists for reuse)");
        // Icon will be cleaned up when parent screen is deleted
    }
}
