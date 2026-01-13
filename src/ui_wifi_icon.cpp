/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * Shared WiFi Icon Component Implementation
 */

// Project headers
#include "config.h"
#include "ui_wifi_icon.h"

// System/Standard library headers
#include <lvgl.h>

// ESP-IDF framework headers
#include <esp_log.h>
#define TAG "ui_wifi"

// Project compatibility headers
#include "esp_idf_compat.h"

// Static WiFi icon objects (shared across all screens)
static lv_obj_t* wifi_container = NULL;
static lv_obj_t* wifi_bar1 = NULL;
static lv_obj_t* wifi_bar2 = NULL;
static lv_obj_t* wifi_bar3 = NULL;
static lv_obj_t* wifi_bar4 = NULL;

// WiFi icon state
static bool wifi_flashing = false;
static unsigned long last_wifi_flash_toggle = 0;
static const unsigned long WIFI_FLASH_INTERVAL_MS = 2500;  // 2500ms on, 2500ms off = 5 second cycle
static bool wifi_flash_state = false;  // Current flash state (true = visible, false = hidden)

lv_obj_t* ui_wifi_icon_create(lv_obj_t* parent) {
    if (wifi_container != NULL) {
        ESP_LOGW(TAG, "[WiFi Icon] WiFi icon already exists, returning existing container");
        return wifi_container;
    }
    
    if (parent == NULL) {
        ESP_LOGE(TAG, "[WiFi Icon] ERROR: Parent object is NULL!");
        return NULL;
    }
    
    ESP_LOGI(TAG, "[WiFi Icon] Creating shared WiFi icon component...");
    
    // Create container for WiFi icon (24x20 pixels to fit signal bars)
    wifi_container = lv_obj_create(parent);
    if (wifi_container == NULL) {
        ESP_LOGE(TAG, "[WiFi Icon] ERROR: Failed to create WiFi container!");
        return NULL;
    }
    
    lv_obj_set_size(wifi_container, 24, 20);
    lv_obj_align(wifi_container, LV_ALIGN_BOTTOM_LEFT, 5, -5);
    lv_obj_set_style_bg_opa(wifi_container, LV_OPA_TRANSP, 0);  // Transparent background
    lv_obj_set_style_border_width(wifi_container, 0, 0);
    lv_obj_set_style_pad_all(wifi_container, 0, 0);
    lv_obj_clear_flag(wifi_container, LV_OBJ_FLAG_SCROLLABLE);
    
    // Create WiFi signal bars (4 bars of increasing height)
    // Bar 1 (shortest) - 4px tall
    wifi_bar1 = lv_obj_create(wifi_container);
    lv_obj_set_size(wifi_bar1, 3, 4);
    lv_obj_set_style_bg_opa(wifi_bar1, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(wifi_bar1, lv_color_hex(0xFF0000), 0);  // Red initially
    lv_obj_set_style_border_width(wifi_bar1, 0, 0);
    lv_obj_set_style_radius(wifi_bar1, 1, 0);
    lv_obj_align(wifi_bar1, LV_ALIGN_BOTTOM_LEFT, 5, -1);
    
    // Bar 2 - 7px tall
    wifi_bar2 = lv_obj_create(wifi_container);
    lv_obj_set_size(wifi_bar2, 3, 7);
    lv_obj_set_style_bg_opa(wifi_bar2, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(wifi_bar2, lv_color_hex(0xFF0000), 0);  // Red initially
    lv_obj_set_style_border_width(wifi_bar2, 0, 0);
    lv_obj_set_style_radius(wifi_bar2, 1, 0);
    lv_obj_align(wifi_bar2, LV_ALIGN_BOTTOM_LEFT, 9, -1);
    
    // Bar 3 - 10px tall
    wifi_bar3 = lv_obj_create(wifi_container);
    lv_obj_set_size(wifi_bar3, 3, 10);
    lv_obj_set_style_bg_opa(wifi_bar3, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(wifi_bar3, lv_color_hex(0xFF0000), 0);  // Red initially
    lv_obj_set_style_border_width(wifi_bar3, 0, 0);
    lv_obj_set_style_radius(wifi_bar3, 1, 0);
    lv_obj_align(wifi_bar3, LV_ALIGN_BOTTOM_LEFT, 13, -1);
    
    // Bar 4 (tallest) - 13px tall
    wifi_bar4 = lv_obj_create(wifi_container);
    lv_obj_set_size(wifi_bar4, 3, 13);
    lv_obj_set_style_bg_opa(wifi_bar4, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(wifi_bar4, lv_color_hex(0xFF0000), 0);  // Red initially
    lv_obj_set_style_border_width(wifi_bar4, 0, 0);
    lv_obj_set_style_radius(wifi_bar4, 1, 0);
    lv_obj_align(wifi_bar4, LV_ALIGN_BOTTOM_LEFT, 17, -1);
    
    lv_timer_handler();
    
    ESP_LOGI(TAG, "[WiFi Icon] Shared WiFi icon component created successfully");
    return wifi_container;
}

lv_obj_t* ui_wifi_icon_get_container() {
    return wifi_container;
}

void ui_wifi_icon_set_flashing(bool flashing) {
    wifi_flashing = flashing;
    if (!flashing) {
        // Reset flash state when disabling
        wifi_flash_state = true;
        last_wifi_flash_toggle = 0;
    }
}

void ui_wifi_icon_update(bool connected, int rssi, bool flashing) {
    if (wifi_container == NULL || wifi_bar1 == NULL || wifi_bar2 == NULL || 
        wifi_bar3 == NULL || wifi_bar4 == NULL) {
        return;  // Icon not created yet
    }
    
    // Update flashing state
    wifi_flashing = flashing;
    
    // Determine color: Green when connected, Red when disconnected
    lv_color_t icon_color = connected ? lv_color_hex(0x00FF00) : lv_color_hex(0xFF0000);
    
    // Determine number of bars to show based on signal strength
    // RSSI ranges: Excellent: >-50, Good: -50 to -60, Fair: -60 to -70, Weak: -70 to -80, Very Weak: <-80
    int bars_to_show = 0;
    if (connected) {
        if (rssi > -50) {
            bars_to_show = 4;  // Excellent signal - show all 4 bars
        } else if (rssi > -60) {
            bars_to_show = 3;  // Good signal - show 3 bars
        } else if (rssi > -70) {
            bars_to_show = 2;  // Fair signal - show 2 bars
        } else if (rssi > -80) {
            bars_to_show = 1;  // Weak signal - show 1 bar
        } else {
            bars_to_show = 1;  // Very weak, but show at least 1 bar
        }
    } else {
        bars_to_show = 0;  // No bars when disconnected (but still show them dimmed)
    }
    
    // Update bar colors
    lv_obj_set_style_bg_color(wifi_bar1, icon_color, 0);
    lv_obj_set_style_bg_color(wifi_bar2, icon_color, 0);
    lv_obj_set_style_bg_color(wifi_bar3, icon_color, 0);
    lv_obj_set_style_bg_color(wifi_bar4, icon_color, 0);
    
    // Handle flashing animation
    unsigned long now = millis();
    lv_opa_t base_opacity = LV_OPA_COVER;
    
    if (wifi_flashing) {
        // Toggle flash state every 2500ms (5 second cycle: 2.5s on, 2.5s off)
        if (now - last_wifi_flash_toggle >= WIFI_FLASH_INTERVAL_MS) {
            wifi_flash_state = !wifi_flash_state;
            last_wifi_flash_toggle = now;
        }
        // Use flash state to control visibility (true = visible, false = hidden)
        base_opacity = wifi_flash_state ? LV_OPA_COVER : LV_OPA_TRANSP;
    } else {
        // Reset flash state when not flashing
        wifi_flash_state = true;
        last_wifi_flash_toggle = 0;
    }
    
    // Show/hide bars based on signal strength and flash state
    // When disconnected (bars_to_show = 0), show all bars at 40% opacity so they're still visible
    // When connected, show bars based on signal strength
    if (bars_to_show == 0) {
        // Disconnected - show all bars dimmed but visible
        lv_opa_t disconnected_opacity = wifi_flashing ? base_opacity : (lv_opa_t)LV_OPA_40;
        lv_obj_set_style_opa(wifi_bar1, disconnected_opacity, 0);
        lv_obj_set_style_opa(wifi_bar2, disconnected_opacity, 0);
        lv_obj_set_style_opa(wifi_bar3, disconnected_opacity, 0);
        lv_obj_set_style_opa(wifi_bar4, disconnected_opacity, 0);
    } else {
        // Connected - show bars based on signal strength, with flash effect if needed
        lv_opa_t bar1_opacity = (bars_to_show >= 1) ? base_opacity : (base_opacity * LV_OPA_20 / LV_OPA_COVER);
        lv_opa_t bar2_opacity = (bars_to_show >= 2) ? base_opacity : (base_opacity * LV_OPA_20 / LV_OPA_COVER);
        lv_opa_t bar3_opacity = (bars_to_show >= 3) ? base_opacity : (base_opacity * LV_OPA_20 / LV_OPA_COVER);
        lv_opa_t bar4_opacity = (bars_to_show >= 4) ? base_opacity : (base_opacity * LV_OPA_20 / LV_OPA_COVER);
        
        lv_obj_set_style_opa(wifi_bar1, bar1_opacity, 0);
        lv_obj_set_style_opa(wifi_bar2, bar2_opacity, 0);
        lv_obj_set_style_opa(wifi_bar3, bar3_opacity, 0);
        lv_obj_set_style_opa(wifi_bar4, bar4_opacity, 0);
    }
    
    // Invalidate to trigger redraw
    lv_obj_invalidate(wifi_container);
}

void ui_wifi_icon_cleanup() {
    // Note: We don't delete the WiFi icon here because it's shared
    // The icon should persist across screen transitions
    // Only cleanup if we're completely shutting down the UI
    if (wifi_container != NULL) {
        ESP_LOGI(TAG, "[WiFi Icon] WiFi icon cleanup called (icon persists for reuse)");
        // Icon will be cleaned up when parent screen is deleted
    }
}
