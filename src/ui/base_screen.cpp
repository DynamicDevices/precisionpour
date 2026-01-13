/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * Base Screen Layout Component Implementation
 */

// Project headers
#include "config.h"
#include "ui/base_screen.h"
#include "ui/ui_logo.h"
#include "ui/ui_wifi_icon.h"
#include "ui/ui_data_icon.h"
#include "mqtt/mqtt_manager.h"
#include "wifi/wifi_manager.h"

// System/Standard library headers
#include <lvgl.h>

// ESP-IDF framework headers
#include <esp_log.h>
#define TAG "base_screen"

// Project compatibility headers
#include "system/esp_idf_compat.h"

// Static content area container
static lv_obj_t* content_area = NULL;

// WiFi update throttling
static unsigned long last_wifi_rssi_update = 0;
static const unsigned long WIFI_RSSI_UPDATE_INTERVAL_MS = 10000;  // Update every 10 seconds
static int cached_rssi = 0;
static bool cached_wifi_connected = false;

lv_obj_t* base_screen_create(lv_obj_t* parent) {
    if (parent == NULL) {
        ESP_LOGE(TAG, "[Base Screen] ERROR: Parent object is NULL!");
        return NULL;
    }
    
    ESP_LOGI(TAG, "[Base Screen] Creating base screen layout...");
    
    // Set background color (pure black)
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    
    // Clear screen - this deletes all children, so shared components will be recreated
    lv_obj_clean(parent);
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    
    // Wait for LVGL to finish processing the cleanup
    // Process multiple times to ensure all deletions are fully processed
    // This prevents "modifying dirty areas in render" errors
    for (int i = 0; i < 5; i++) {
        lv_timer_handler();
        delay(5);
    }
    
    // Create shared logo (top center) - create first so it's behind other elements
    if (ui_logo_create(parent) == NULL) {
        ESP_LOGE(TAG, "[Base Screen] ERROR: Failed to create logo!");
        return NULL;
    }
    
    // Force logo container to be behind other elements (lower z-order)
    // The logo is created first, so it's already behind, but we ensure it explicitly
    lv_obj_t* logo_obj = ui_logo_get_object();
    if (logo_obj != NULL) {
        // Get the parent container and move it to background
        lv_obj_t* logo_parent = lv_obj_get_parent(logo_obj);
        if (logo_parent != NULL) {
            lv_obj_move_background(logo_parent);
        }
    }
    
    // Create content area (middle section, below logo)
    content_area = lv_obj_create(parent);
    if (content_area == NULL) {
        ESP_LOGE(TAG, "[Base Screen] ERROR: Failed to create content area!");
        return NULL;
    }
    
    // Content area spans from below logo to above icons
    // Logo container is 60px high (with 10px top margin, ends at y=70), icons are 20px + 5px margin = 25px from bottom
    // So content area: y=70, height = DISPLAY_HEIGHT - 70 - 25 = DISPLAY_HEIGHT - 95
    lv_obj_set_size(content_area, DISPLAY_WIDTH, DISPLAY_HEIGHT - BASE_SCREEN_CONTENT_Y - 25);
    lv_obj_align(content_area, LV_ALIGN_TOP_LEFT, 0, BASE_SCREEN_CONTENT_Y);
    lv_obj_set_style_bg_opa(content_area, LV_OPA_TRANSP, 0);  // Transparent background
    lv_obj_set_style_border_width(content_area, 0, 0);
    lv_obj_set_style_pad_all(content_area, 0, 0);
    lv_obj_clear_flag(content_area, LV_OBJ_FLAG_SCROLLABLE);  // Disable scrolling
    
    // Create shared WiFi icon (bottom left)
    if (ui_wifi_icon_create(parent) == NULL) {
        ESP_LOGE(TAG, "[Base Screen] ERROR: Failed to create WiFi icon!");
        return NULL;
    }
    
    // Create shared data icon (bottom right)
    if (ui_data_icon_create(parent) == NULL) {
        ESP_LOGE(TAG, "[Base Screen] ERROR: Failed to create data icon!");
        return NULL;
    }
    
    // Process all pending LVGL operations after creating all objects
    // Process multiple times to ensure all creations are fully processed
    for (int i = 0; i < 3; i++) {
        lv_timer_handler();
        delay(5);
    }
    
    ESP_LOGI(TAG, "[Base Screen] Base screen layout created successfully");
    return content_area;
}

lv_obj_t* base_screen_get_content_area() {
    return content_area;
}

void base_screen_update() {
    // Update WiFi icon
    unsigned long now = millis();
    
    // Throttle WiFi RSSI updates
    if (now - last_wifi_rssi_update >= WIFI_RSSI_UPDATE_INTERVAL_MS || last_wifi_rssi_update == 0) {
        cached_wifi_connected = wifi_manager_is_connected();
        cached_rssi = wifi_manager_get_rssi();
        last_wifi_rssi_update = now;
    }
    
    // Check if WiFi is connected but MQTT is not (for flashing)
    bool mqtt_connected = mqtt_client_is_connected();
    bool should_flash = cached_wifi_connected && !mqtt_connected;
    
    // Update WiFi icon
    ui_wifi_icon_update(cached_wifi_connected, cached_rssi, should_flash);
    
    // Update data icon - use WiFi RX/TX bytes activity (actual TCP/IP traffic)
    bool wifi_active = wifi_manager_has_activity();
    ui_data_icon_update(mqtt_connected, wifi_active);
}

void base_screen_cleanup() {
    // Note: We don't delete shared components (logo, WiFi icon, data icon)
    // They persist across screen transitions
    
    // Only clean up the content area
    if (content_area != NULL) {
        lv_obj_del(content_area);
        content_area = NULL;
        ESP_LOGI(TAG, "[Base Screen] Content area cleaned up");
    }
}
