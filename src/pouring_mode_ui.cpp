/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * Pouring Mode UI Implementation
 * 
 * Screen displayed during active pouring, showing flow rate, volume, and cost
 */

// Project headers
#include "config.h"
#include "flow/flow_meter.h"
#include "images/precision_pour_logo.h"
#include "mqtt/mqtt_manager.h"
#include "pouring_mode_ui.h"
#include "utils/rle_decompress.h"
#include "wifi/wifi_manager.h"

// System/Standard library headers
#include <lvgl.h>
#include <string.h>

// ESP-IDF framework headers
#include <esp_log.h>
#define TAG "pouring_ui"

// Project compatibility headers
#include "system/esp_idf_compat.h"

// UI objects
static lv_obj_t *logo_container = NULL;
static lv_obj_t *flow_rate_label = NULL;
static lv_obj_t *flow_rate_value = NULL;
static lv_obj_t *volume_label = NULL;
static lv_obj_t *volume_value = NULL;
static lv_obj_t *cost_per_unit_label = NULL;
static lv_obj_t *cost_per_unit_value = NULL;
static lv_obj_t *total_cost_label = NULL;
static lv_obj_t *total_cost_value = NULL;
static lv_obj_t *wifi_status_container = NULL;  // WiFi status icon container
static lv_obj_t *wifi_bar1 = NULL;  // WiFi signal bar 1 (shortest)
static lv_obj_t *wifi_bar2 = NULL;  // WiFi signal bar 2
static lv_obj_t *wifi_bar3 = NULL;  // WiFi signal bar 3
static lv_obj_t *wifi_bar4 = NULL;  // WiFi signal bar 4 (tallest)
static lv_obj_t *comm_status_container = NULL;  // Communication activity icon container
static lv_obj_t *comm_spark1 = NULL;  // Spark/pulse element 1
static lv_obj_t *comm_spark2 = NULL;  // Spark/pulse element 2
static lv_obj_t *comm_spark3 = NULL;  // Spark/pulse element 3

// WiFi signal strength update throttling
static unsigned long last_wifi_rssi_update = 0;
static const unsigned long WIFI_RSSI_UPDATE_INTERVAL_MS = 10000;  // Update every 10 seconds
static int cached_rssi = 0;  // Cached RSSI value
static bool cached_wifi_connected = false;  // Cached connection status

// Brand colors (matching PrecisionPour branding)
#define COLOR_BACKGROUND lv_color_hex(0x000000) // Pure black background (RGB 0,0,0)
#define COLOR_TEXT lv_color_hex(0xFFFFFF) // White
#define COLOR_GOLDEN lv_color_hex(0xFFD700) // Golden yellow (branding color)

// Pouring parameters (from "paid" MQTT command)
static char pour_unique_id[64] = {0};  // Unique ID for this pour
static float cost_per_ml = 0.0;  // Cost per milliliter (from MQTT)
static int max_ml = 0;  // Maximum milliliters allowed (from MQTT)
static bool pour_active = false;  // Whether a paid pour is active
static char currency_symbol[8] = {0};  // Currency symbol (from MQTT, defaults to CURRENCY_SYMBOL)

// Callback function to switch back to main screen (set from main.cpp)
static void (*screen_switch_callback)(void) = NULL;

// Forward declaration
static void pouring_screen_touch_cb(lv_event_t *e);

void pouring_mode_init() {
    ESP_LOGI(TAG, "\n=== Initializing Pouring Mode UI ===");
    
    // Ensure LVGL is ready
    if (lv_scr_act() == NULL) {
        ESP_LOGE(TAG, "[Pouring UI] ERROR: No active screen!");
        return;
    }
    
    // Set background color FIRST (before clearing) to prevent white flash
    lv_obj_set_style_bg_color(lv_scr_act(), COLOR_BACKGROUND, 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);
    lv_timer_handler();
    delay(5);
    
    // Now clear screen (background is already set)
    lv_obj_clean(lv_scr_act());
    // Re-apply background color
    lv_obj_set_style_bg_color(lv_scr_act(), COLOR_BACKGROUND, 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);
    lv_timer_handler();
    delay(5);
    
    // Create logo area at the top (same as production mode)
    ESP_LOGI(TAG, "[Pouring UI] Creating logo from image...");
    logo_container = lv_obj_create(lv_scr_act());
    if (logo_container == NULL) {
        ESP_LOGE(TAG, "[Pouring UI] ERROR: Failed to create logo container!");
        return;
    }
    
    // Set container size to fit logo
    lv_obj_set_size(logo_container, DISPLAY_WIDTH, 90);
    lv_obj_align(logo_container, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(logo_container, LV_OPA_TRANSP, 0);  // Transparent background
    lv_obj_set_style_border_width(logo_container, 0, 0);
    lv_obj_set_style_pad_all(logo_container, 0, 0);
    lv_obj_clear_flag(logo_container, LV_OBJ_FLAG_SCROLLABLE);  // Disable scrolling
    lv_timer_handler();
    
    // Create logo image object
    lv_obj_t *logo_img = lv_img_create(logo_container);
    if (logo_img == NULL) {
        ESP_LOGE(TAG, "[Pouring UI] ERROR: Failed to create logo image object!");
        return;
    }
    
    // Get decompressed image (handles RLE compression if enabled)
    const lv_img_dsc_t* logo_img_data = rle_get_image(
        &precision_pour_logo,
        PRECISION_POUR_LOGO_IS_COMPRESSED,
        PRECISION_POUR_LOGO_IS_COMPRESSED ? PRECISION_POUR_LOGO_UNCOMPRESSED_SIZE : precision_pour_logo.data_size
    );
    
    if (logo_img_data == NULL) {
        ESP_LOGE(TAG, "[Pouring UI] ERROR: Failed to get logo image!");
    } else {
        // Set logo image source
        lv_img_set_src(logo_img, logo_img_data);
    }
    
    // Center the logo in the container
    lv_obj_align(logo_img, LV_ALIGN_CENTER, 0, 0);
    
    // Force refresh
    lv_obj_invalidate(logo_img);
    lv_refr_now(NULL);
    lv_timer_handler();
    delay(10);
    lv_timer_handler();
    
    ESP_LOGI(TAG, "[Pouring UI] Logo created from image");
    
    // Create pouring information labels (below logo)
    // Flow Rate
    flow_rate_label = lv_label_create(lv_scr_act());
    if (flow_rate_label != NULL) {
        lv_label_set_text(flow_rate_label, "Flow Rate:");
        lv_obj_set_style_text_color(flow_rate_label, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(flow_rate_label, &lv_font_montserrat_14, 0);
        lv_obj_align(flow_rate_label, LV_ALIGN_TOP_LEFT, 10, 100);
    }
    
    flow_rate_value = lv_label_create(lv_scr_act());
    if (flow_rate_value != NULL) {
        lv_label_set_text(flow_rate_value, "0.00 mL/min");
        lv_obj_set_style_text_color(flow_rate_value, COLOR_GOLDEN, 0);
        lv_obj_set_style_text_font(flow_rate_value, &lv_font_montserrat_14, 0);
        lv_obj_align(flow_rate_value, LV_ALIGN_TOP_LEFT, 10, 120);
    }
    
    // Volume
    volume_label = lv_label_create(lv_scr_act());
    if (volume_label != NULL) {
        lv_label_set_text(volume_label, "Volume:");
        lv_obj_set_style_text_color(volume_label, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(volume_label, &lv_font_montserrat_14, 0);
        lv_obj_align(volume_label, LV_ALIGN_TOP_LEFT, 10, 150);
    }
    
    volume_value = lv_label_create(lv_scr_act());
    if (volume_value != NULL) {
        lv_label_set_text(volume_value, "0 ml");
        lv_obj_set_style_text_color(volume_value, COLOR_GOLDEN, 0);
        lv_obj_set_style_text_font(volume_value, &lv_font_montserrat_14, 0);
        lv_obj_align(volume_value, LV_ALIGN_TOP_LEFT, 10, 170);
    }
    
    // Cost per Unit (now per ml)
    cost_per_unit_label = lv_label_create(lv_scr_act());
    if (cost_per_unit_label != NULL) {
        lv_label_set_text(cost_per_unit_label, "Cost per ml:");
        lv_obj_set_style_text_color(cost_per_unit_label, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(cost_per_unit_label, &lv_font_montserrat_14, 0);
        lv_obj_align(cost_per_unit_label, LV_ALIGN_TOP_RIGHT, -10, 100);
    }
    
    cost_per_unit_value = lv_label_create(lv_scr_act());
    if (cost_per_unit_value != NULL) {
        char init_str[32];
        const char* symbol = (strlen(currency_symbol) > 0) ? currency_symbol : CURRENCY_SYMBOL;
        snprintf(init_str, sizeof(init_str), "%s0.0000", symbol);
        lv_label_set_text(cost_per_unit_value, init_str);
        lv_obj_set_style_text_color(cost_per_unit_value, COLOR_GOLDEN, 0);
        lv_obj_set_style_text_font(cost_per_unit_value, &lv_font_montserrat_14, 0);
        lv_obj_align(cost_per_unit_value, LV_ALIGN_TOP_RIGHT, -10, 120);
    }
    
    // Total Cost
    total_cost_label = lv_label_create(lv_scr_act());
    if (total_cost_label != NULL) {
        lv_label_set_text(total_cost_label, "Total Cost:");
        lv_obj_set_style_text_color(total_cost_label, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(total_cost_label, &lv_font_montserrat_14, 0);
        lv_obj_align(total_cost_label, LV_ALIGN_TOP_RIGHT, -10, 150);
    }
    
    total_cost_value = lv_label_create(lv_scr_act());
    if (total_cost_value != NULL) {
        char init_str[32];
        const char* symbol = (strlen(currency_symbol) > 0) ? currency_symbol : CURRENCY_SYMBOL;
        snprintf(init_str, sizeof(init_str), "%s0.00", symbol);
        lv_label_set_text(total_cost_value, init_str);
        lv_obj_set_style_text_color(total_cost_value, COLOR_GOLDEN, 0);
        lv_obj_set_style_text_font(total_cost_value, &lv_font_montserrat_14, 0);
        lv_obj_align(total_cost_value, LV_ALIGN_TOP_RIGHT, -10, 170);
    }
    
    // Create WiFi status icon in bottom left corner (same as production mode)
    ESP_LOGI(TAG, "[Pouring UI] Creating WiFi status icon...");
    wifi_status_container = lv_obj_create(lv_scr_act());
    if (wifi_status_container != NULL) {
        // Container for WiFi icon (20x20 pixels)
        lv_obj_set_size(wifi_status_container, 20, 20);
        lv_obj_align(wifi_status_container, LV_ALIGN_BOTTOM_LEFT, 5, -5);
        lv_obj_set_style_bg_opa(wifi_status_container, LV_OPA_TRANSP, 0);  // Transparent background
        lv_obj_set_style_border_width(wifi_status_container, 0, 0);
        lv_obj_set_style_pad_all(wifi_status_container, 0, 0);
        lv_obj_clear_flag(wifi_status_container, LV_OBJ_FLAG_SCROLLABLE);
        
        // Bar 1 (shortest) - 4px tall
        wifi_bar1 = lv_obj_create(wifi_status_container);
        lv_obj_set_size(wifi_bar1, 3, 4);
        lv_obj_set_style_bg_opa(wifi_bar1, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(wifi_bar1, lv_color_hex(0xFF0000), 0);  // Red initially
        lv_obj_set_style_border_width(wifi_bar1, 0, 0);
        lv_obj_set_style_radius(wifi_bar1, 1, 0);
        lv_obj_align(wifi_bar1, LV_ALIGN_BOTTOM_LEFT, 5, -1);
        
        // Bar 2 - 7px tall
        wifi_bar2 = lv_obj_create(wifi_status_container);
        lv_obj_set_size(wifi_bar2, 3, 7);
        lv_obj_set_style_bg_opa(wifi_bar2, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(wifi_bar2, lv_color_hex(0xFF0000), 0);  // Red initially
        lv_obj_set_style_border_width(wifi_bar2, 0, 0);
        lv_obj_set_style_radius(wifi_bar2, 1, 0);
        lv_obj_align(wifi_bar2, LV_ALIGN_BOTTOM_LEFT, 9, -1);
        
        // Bar 3 - 10px tall
        wifi_bar3 = lv_obj_create(wifi_status_container);
        lv_obj_set_size(wifi_bar3, 3, 10);
        lv_obj_set_style_bg_opa(wifi_bar3, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(wifi_bar3, lv_color_hex(0xFF0000), 0);  // Red initially
        lv_obj_set_style_border_width(wifi_bar3, 0, 0);
        lv_obj_set_style_radius(wifi_bar3, 1, 0);
        lv_obj_align(wifi_bar3, LV_ALIGN_BOTTOM_LEFT, 13, -1);
        
        // Bar 4 (tallest) - 13px tall
        wifi_bar4 = lv_obj_create(wifi_status_container);
        lv_obj_set_size(wifi_bar4, 3, 13);
        lv_obj_set_style_bg_opa(wifi_bar4, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(wifi_bar4, lv_color_hex(0xFF0000), 0);  // Red initially
        lv_obj_set_style_border_width(wifi_bar4, 0, 0);
        lv_obj_set_style_radius(wifi_bar4, 1, 0);
        lv_obj_align(wifi_bar4, LV_ALIGN_BOTTOM_LEFT, 17, -1);
        
        ESP_LOGI(TAG, "[Pouring UI] WiFi status icon created");
    }
    lv_timer_handler();
    
    // Create communication activity icon in bottom right corner (same as production mode)
    ESP_LOGI(TAG, "[Pouring UI] Creating communication activity icon...");
    comm_status_container = lv_obj_create(lv_scr_act());
    if (comm_status_container != NULL) {
        // Container for communication icon (20x20 pixels)
        lv_obj_set_size(comm_status_container, 20, 20);
        lv_obj_align(comm_status_container, LV_ALIGN_BOTTOM_RIGHT, -5, -5);
        lv_obj_set_style_bg_opa(comm_status_container, LV_OPA_TRANSP, 0);  // Transparent background
        lv_obj_set_style_border_width(comm_status_container, 0, 0);
        lv_obj_set_style_pad_all(comm_status_container, 0, 0);
        lv_obj_clear_flag(comm_status_container, LV_OBJ_FLAG_SCROLLABLE);
        
        // Create spark/pulse elements
        // Spark 1 (left, small)
        comm_spark1 = lv_obj_create(comm_status_container);
        lv_obj_set_size(comm_spark1, 2, 6);
        lv_obj_set_style_bg_opa(comm_spark1, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(comm_spark1, lv_color_hex(0x808080), 0);  // Gray initially (no activity)
        lv_obj_set_style_border_width(comm_spark1, 0, 0);
        lv_obj_set_style_radius(comm_spark1, 1, 0);
        lv_obj_align(comm_spark1, LV_ALIGN_LEFT_MID, 4, 0);
        
        // Spark 2 (center, medium) - main pulse
        comm_spark2 = lv_obj_create(comm_status_container);
        lv_obj_set_size(comm_spark2, 3, 10);
        lv_obj_set_style_bg_opa(comm_spark2, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(comm_spark2, lv_color_hex(0x808080), 0);  // Gray initially (no activity)
        lv_obj_set_style_border_width(comm_spark2, 0, 0);
        lv_obj_set_style_radius(comm_spark2, 1, 0);
        lv_obj_align(comm_spark2, LV_ALIGN_CENTER, 0, 0);
        
        // Spark 3 (right, small)
        comm_spark3 = lv_obj_create(comm_status_container);
        lv_obj_set_size(comm_spark3, 2, 6);
        lv_obj_set_style_bg_opa(comm_spark3, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(comm_spark3, lv_color_hex(0x808080), 0);  // Gray initially (no activity)
        lv_obj_set_style_border_width(comm_spark3, 0, 0);
        lv_obj_set_style_radius(comm_spark3, 1, 0);
        lv_obj_align(comm_spark3, LV_ALIGN_RIGHT_MID, -4, 0);
        
        ESP_LOGI(TAG, "[Pouring UI] Communication activity icon created");
    }
    lv_timer_handler();
    
    // Add touch event handler to screen - tap anywhere to return to main screen
    lv_obj_add_event_cb(lv_scr_act(), pouring_screen_touch_cb, LV_EVENT_CLICKED, NULL);
    
    // Force refresh to show everything
    for (int i = 0; i < 5; i++) {
        lv_timer_handler();
        delay(5);
    }
    
    ESP_LOGI(TAG, "Pouring Mode UI initialized");
}

// Touch event callback for pouring screen - switch back to main screen on tap
static void pouring_screen_touch_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "[Pouring UI] Screen tapped - switching to main screen");
        
        // Call the callback to switch back to main screen
        if (screen_switch_callback != NULL) {
            screen_switch_callback();
        } else {
            ESP_LOGE(TAG, "[Pouring UI] ERROR: screen_switch_callback is NULL!");
        }
    }
}

void pouring_mode_update() {
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
            // Max reached - could trigger stop here or show warning
            ESP_LOGW(TAG, "[Pouring] Maximum volume reached!");
        }
    }
    
    // Update WiFi status icon (same logic as production mode)
    if (wifi_status_container != NULL && 
        wifi_bar1 != NULL && wifi_bar2 != NULL && wifi_bar3 != NULL && wifi_bar4 != NULL) {
        unsigned long now = millis();
        
        // Only update RSSI every 10 seconds
        if (now - last_wifi_rssi_update >= WIFI_RSSI_UPDATE_INTERVAL_MS || last_wifi_rssi_update == 0) {
            cached_wifi_connected = wifi_manager_is_connected();
            cached_rssi = wifi_manager_get_rssi();
            last_wifi_rssi_update = now;
        }
        
        bool wifi_connected = cached_wifi_connected;
        int rssi = cached_rssi;
        
        // Determine color: Green when connected, Red when disconnected
        lv_color_t icon_color = wifi_connected ? lv_color_hex(0x00FF00) : lv_color_hex(0xFF0000);
        
        // Determine number of bars to show based on signal strength
        int bars_to_show = 0;
        if (wifi_connected) {
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
        
        // Show/hide bars based on signal strength
        if (bars_to_show == 0) {
            // Disconnected - show all bars dimmed but visible
            lv_obj_set_style_opa(wifi_bar1, LV_OPA_40, 0);
            lv_obj_set_style_opa(wifi_bar2, LV_OPA_40, 0);
            lv_obj_set_style_opa(wifi_bar3, LV_OPA_40, 0);
            lv_obj_set_style_opa(wifi_bar4, LV_OPA_40, 0);
        } else {
            // Connected - show bars based on signal strength
            lv_obj_set_style_opa(wifi_bar1, (bars_to_show >= 1) ? LV_OPA_COVER : LV_OPA_20, 0);
            lv_obj_set_style_opa(wifi_bar2, (bars_to_show >= 2) ? LV_OPA_COVER : LV_OPA_20, 0);
            lv_obj_set_style_opa(wifi_bar3, (bars_to_show >= 3) ? LV_OPA_COVER : LV_OPA_20, 0);
            lv_obj_set_style_opa(wifi_bar4, (bars_to_show >= 4) ? LV_OPA_COVER : LV_OPA_20, 0);
        }
        
        // Force redraw
        lv_obj_invalidate(wifi_bar1);
        lv_obj_invalidate(wifi_bar2);
        lv_obj_invalidate(wifi_bar3);
        lv_obj_invalidate(wifi_bar4);
    }
    
    // Update communication activity icon (same logic as production mode)
    if (comm_status_container != NULL && comm_spark1 != NULL && comm_spark2 != NULL && comm_spark3 != NULL) {
        bool has_activity = mqtt_client_has_activity();
        
        // Update icon color: Green when active, Gray when idle
        lv_color_t comm_color = has_activity ? lv_color_hex(0x00FF00) : lv_color_hex(0x808080);
        
        // Update all spark elements
        lv_obj_set_style_bg_color(comm_spark1, comm_color, 0);
        lv_obj_set_style_bg_color(comm_spark2, comm_color, 0);
        lv_obj_set_style_bg_color(comm_spark3, comm_color, 0);
        
        // When active, make the center spark brighter/more prominent
        if (has_activity) {
            lv_obj_set_style_opa(comm_spark1, LV_OPA_80, 0);  // Slightly dimmed
            lv_obj_set_style_opa(comm_spark2, LV_OPA_COVER, 0);  // Full brightness (main pulse)
            lv_obj_set_style_opa(comm_spark3, LV_OPA_80, 0);  // Slightly dimmed
        } else {
            lv_obj_set_style_opa(comm_spark1, LV_OPA_30, 0);  // Dimmed when idle
            lv_obj_set_style_opa(comm_spark2, LV_OPA_30, 0);  // Dimmed when idle
            lv_obj_set_style_opa(comm_spark3, LV_OPA_30, 0);  // Dimmed when idle
        }
        
        // Force redraw
        lv_obj_invalidate(comm_spark1);
        lv_obj_invalidate(comm_spark2);
        lv_obj_invalidate(comm_spark3);
    }
}

void pouring_mode_reset() {
    // Reset flow meter volume counter
    flow_meter_reset_volume();
    
    // Reset pour parameters
    pour_unique_id[0] = '\0';
    cost_per_ml = 0.0;
    max_ml = 0;
    pour_active = false;
    currency_symbol[0] = '\0';  // Reset currency symbol
    
    // Update displays to show zero
    if (flow_rate_value != NULL) {
        lv_label_set_text(flow_rate_value, "0.00 mL/min");
    }
    if (volume_value != NULL) {
        lv_label_set_text(volume_value, "0 ml");
    }
    if (cost_per_unit_value != NULL) {
        char init_str[32];
        const char* symbol = (strlen(currency_symbol) > 0) ? currency_symbol : CURRENCY_SYMBOL;
        snprintf(init_str, sizeof(init_str), "%s0.0000", symbol);
        lv_label_set_text(cost_per_unit_value, init_str);
    }
    if (total_cost_value != NULL) {
        char init_str[32];
        const char* symbol = (strlen(currency_symbol) > 0) ? currency_symbol : CURRENCY_SYMBOL;
        snprintf(init_str, sizeof(init_str), "%s0.00", symbol);
        lv_label_set_text(total_cost_value, init_str);
    }
}

// Function to update cost per unit (legacy - kept for backward compatibility)
void pouring_mode_set_cost_per_unit(float cost) {
    // Convert cost per liter to cost per ml
    cost_per_ml = cost / 1000.0;
    ESP_LOGI(TAG, "[Pouring UI] Cost per unit updated to: £%.2f/L (%.4f/ml)", cost, cost_per_ml);
}

// Helper function to parse currency string and return symbol
// Accepts standard ISO currency codes: "GBP" or "USD" (case-insensitive)
// Note: Uses "GBP " text prefix for pounds since £ symbol (U+00A3) is not in the font
static const char* parse_currency(const char* currency_str) {
    if (currency_str == NULL || strlen(currency_str) == 0) {
        // Default: use "GBP " for pounds (since £ symbol not in font)
        return "GBP ";
    }
    
    // Convert to uppercase for comparison (ISO codes are typically uppercase)
    char upper[8] = {0};
    strncpy(upper, currency_str, sizeof(upper) - 1);
    for (int i = 0; upper[i]; i++) {
        upper[i] = toupper(upper[i]);
    }
    
    // Check for standard ISO currency codes
    if (strcmp(upper, "GBP") == 0) {
        return "GBP ";  // Use text prefix instead of £ symbol (not in font)
    } else if (strcmp(upper, "USD") == 0) {
        return "$";  // $ symbol is in ASCII, should work
    }
    
    // If not recognized, use default (GBP for pounds)
    ESP_LOGW(TAG, "[Pouring UI] Warning: Unrecognized currency code '%s', defaulting to GBP", currency_str);
    return "GBP ";
}

// Start pouring with paid parameters
void pouring_mode_start_pour(const char* unique_id, float cost_per_ml_param, int max_ml_param, const char* currency) {
    // Store parameters
    strncpy(pour_unique_id, unique_id, sizeof(pour_unique_id) - 1);
    pour_unique_id[sizeof(pour_unique_id) - 1] = '\0';
    cost_per_ml = cost_per_ml_param;
    max_ml = max_ml_param;
    pour_active = true;
    
    // Parse and store currency symbol
    const char* currency_sym = parse_currency(currency);
    strncpy(currency_symbol, currency_sym, sizeof(currency_symbol) - 1);
    currency_symbol[sizeof(currency_symbol) - 1] = '\0';
    
    ESP_LOGI(TAG, "[Pouring UI] Starting pour:");
    ESP_LOGI(TAG, "  ID: %s", pour_unique_id);
    ESP_LOGI(TAG, "  Cost per ml: %s%.4f", currency_symbol, cost_per_ml);
    ESP_LOGI(TAG, "  Max ml: %d", max_ml);
    ESP_LOGI(TAG, "  Currency: %s", currency_symbol);
    
    // Reset volume counter
    flow_meter_reset_volume();
    
    // Initialize pouring screen
    pouring_mode_init();
}

// Update pour parameters (if pour is already active)
void pouring_mode_update_pour_params(const char* unique_id, float cost_per_ml_param, int max_ml_param, const char* currency) {
    strncpy(pour_unique_id, unique_id, sizeof(pour_unique_id) - 1);
    pour_unique_id[sizeof(pour_unique_id) - 1] = '\0';
    cost_per_ml = cost_per_ml_param;
    max_ml = max_ml_param;
    
    // Parse and store currency symbol
    const char* currency_sym = parse_currency(currency);
    strncpy(currency_symbol, currency_sym, sizeof(currency_symbol) - 1);
    currency_symbol[sizeof(currency_symbol) - 1] = '\0';
    
    ESP_LOGI(TAG, "[Pouring UI] Updated pour parameters:");
    ESP_LOGI(TAG, "  ID: %s", pour_unique_id);
    ESP_LOGI(TAG, "  Cost per ml: %s%.4f", currency_symbol, cost_per_ml);
    ESP_LOGI(TAG, "  Max ml: %d", max_ml);
    ESP_LOGI(TAG, "  Currency: %s", currency_symbol);
}

// Check if maximum ml has been reached
bool pouring_mode_is_max_reached() {
    if (!pour_active || max_ml <= 0) {
        return false;
    }
    
    float volume_liters = flow_meter_get_total_volume_liters();
    float volume_ml = volume_liters * 1000.0;
    
    return (volume_ml >= max_ml);
}

// Set callback function to switch back to main screen
void pouring_mode_set_screen_switch_callback(void (*callback)(void)) {
    screen_switch_callback = callback;
}
