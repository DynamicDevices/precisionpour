/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * UI Implementation
 * 
 * Creates a simple example UI with LVGL.
 * Customize this for your application needs.
 */

#include "ui.h"
#include "system/esp_idf_compat.h"
#include "esp_log.h"
#define TAG "ui"

// UI objects
static lv_obj_t *label_title;
static lv_obj_t *label_status;
static lv_obj_t *btn_test;
static lv_obj_t *label_counter;
static int32_t counter = 0;

// Button event callback
static void btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_CLICKED) {
        counter++;
        lv_label_set_text_fmt(label_counter, "Clicks: %ld", counter);
        ESP_LOGI(TAG, "Button clicked! Count: %ld", counter);
    }
}

void ui_init() {
    // Create a simple UI
    
    // Title label
    label_title = lv_label_create(lv_scr_act());
    lv_label_set_text(label_title, "ESP32 Touchscreen");
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_14, 0);
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 20);
    
    // Status label
    label_status = lv_label_create(lv_scr_act());
    lv_label_set_text(label_status, "System Ready");
    lv_obj_align(label_status, LV_ALIGN_TOP_MID, 0, 50);
    
    // Test button
    btn_test = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn_test, 150, 50);
    lv_obj_align(btn_test, LV_ALIGN_CENTER, 0, -20);
    lv_obj_add_event_cb(btn_test, btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Button label
    lv_obj_t *btn_label = lv_label_create(btn_test);
    lv_label_set_text(btn_label, "Click Me!");
    lv_obj_center(btn_label);
    
    // Counter label
    label_counter = lv_label_create(lv_scr_act());
    lv_label_set_text_fmt(label_counter, "Clicks: %ld", counter);
    lv_obj_align(label_counter, LV_ALIGN_CENTER, 0, 40);
    
    ESP_LOGI(TAG, "UI initialized");
}

void ui_update() {
    // Update UI elements if needed
    // This is called periodically from the main loop
}
