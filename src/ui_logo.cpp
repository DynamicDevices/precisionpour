/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * Shared Logo Component Implementation
 */

// Project headers
#include "config.h"
#include "ui_logo.h"
#include "images/precision_pour_logo.h"
#include "rle_decompress.h"

// System/Standard library headers
#include <lvgl.h>

// ESP-IDF framework headers
#include <esp_log.h>
#define TAG "ui_logo"

// Static logo object (shared across all screens)
static lv_obj_t* logo_obj = NULL;
static lv_obj_t* logo_container = NULL;

lv_obj_t* ui_logo_create(lv_obj_t* parent) {
    if (logo_obj != NULL) {
        ESP_LOGW(TAG, "[Logo] Logo already exists, returning existing object");
        return logo_obj;
    }
    
    if (parent == NULL) {
        ESP_LOGE(TAG, "[Logo] ERROR: Parent object is NULL!");
        return NULL;
    }
    
    ESP_LOGI(TAG, "[Logo] Creating shared logo component...");
    
    // Create container for logo (allows positioning)
    logo_container = lv_obj_create(parent);
    if (logo_container == NULL) {
        ESP_LOGE(TAG, "[Logo] ERROR: Failed to create logo container!");
        return NULL;
    }
    
    // Set container size to fit logo (259x40 logo, container 60px high for spacing)
    // Reduced from 70px to 60px to give more space for content area
    lv_obj_set_size(logo_container, DISPLAY_WIDTH, 60);
    lv_obj_align(logo_container, LV_ALIGN_TOP_MID, 0, 10);  // Move down 10px from top
    lv_obj_set_style_bg_opa(logo_container, LV_OPA_TRANSP, 0);  // Transparent background
    lv_obj_set_style_border_width(logo_container, 0, 0);
    lv_obj_set_style_pad_all(logo_container, 0, 0);
    lv_obj_clear_flag(logo_container, LV_OBJ_FLAG_SCROLLABLE);  // Disable scrolling
    
    // Create logo image object
    logo_obj = lv_img_create(logo_container);
    if (logo_obj == NULL) {
        ESP_LOGE(TAG, "[Logo] ERROR: Failed to create logo image object!");
        lv_obj_del(logo_container);
        logo_container = NULL;
        return NULL;
    }
    
    // Get decompressed image (handles RLE compression if enabled)
    const lv_img_dsc_t* logo_img = rle_get_image(
        &precision_pour_logo,
        PRECISION_POUR_LOGO_IS_COMPRESSED,
        PRECISION_POUR_LOGO_IS_COMPRESSED ? PRECISION_POUR_LOGO_UNCOMPRESSED_SIZE : precision_pour_logo.data_size
    );
    
    if (logo_img == NULL) {
        ESP_LOGE(TAG, "[Logo] ERROR: Failed to get logo image!");
        lv_obj_del(logo_container);
        logo_container = NULL;
        return NULL;
    }
    
    // Set logo image source
    lv_img_set_src(logo_obj, logo_img);
    
    // Center the logo in the container
    lv_obj_align(logo_obj, LV_ALIGN_CENTER, 0, 0);
    
    // Force refresh
    lv_obj_invalidate(logo_obj);
    lv_timer_handler();
    
    ESP_LOGI(TAG, "[Logo] Shared logo component created successfully");
    return logo_obj;
}

lv_obj_t* ui_logo_get_object() {
    return logo_obj;
}

void ui_logo_cleanup() {
    // Note: We don't delete the logo object here because it's shared
    // The logo should persist across screen transitions
    // Only cleanup if we're completely shutting down the UI
    if (logo_obj != NULL) {
        ESP_LOGI(TAG, "[Logo] Logo cleanup called (logo persists for reuse)");
        // Logo will be cleaned up when parent screen is deleted
    }
}
