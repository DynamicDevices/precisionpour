/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * Test Mode UI Implementation
 * 
 * Comprehensive hardware testing interface
 */

// Project headers
#include "config.h"
#include "lvgl_touch.h"
#include "test_mode_ui.h"

// System/Standard library headers
#ifdef ESP_PLATFORM
    // ESP-IDF framework headers
    #include <esp_log.h>
    #define TAG "test_ui"
    
    // Project compatibility headers
    #include "esp_idf_compat.h"
#else
    // Arduino framework headers
    #include <Arduino.h>
#endif

// UI objects
static lv_obj_t *tabview = NULL;
static lv_obj_t *tab_display = NULL;
static lv_obj_t *tab_touch = NULL;
static lv_obj_t *tab_flow = NULL;
static lv_obj_t *tab_rfid = NULL;

// Display test objects
static lv_obj_t *label_display_title;
static lv_obj_t *label_display_info;
static lv_obj_t *btn_color_test;
static lv_obj_t *label_color_status;

// Touch test objects
static lv_obj_t *label_touch_title;
static lv_obj_t *label_touch_coords;
static lv_obj_t *label_touch_status;
static lv_obj_t *canvas_touch_area;
static int16_t last_touch_x = -1;
static int16_t last_touch_y = -1;

// Flow meter test objects
static lv_obj_t *label_flow_title;
static lv_obj_t *label_flow_status;
static lv_obj_t *label_flow_count;
static uint32_t flow_count = 0;

// RFID/NFC test objects
static lv_obj_t *label_rfid_title;
static lv_obj_t *label_rfid_status;
static lv_obj_t *label_rfid_id;
static lv_obj_t *btn_rfid_scan;

// Color test callback
static void color_test_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    
    // Log all events to see what's happening
    Serial.printf("[Button] Event received: code=%d (CLICKED=%d)\r\n", code, LV_EVENT_CLICKED);
    
    if (code == LV_EVENT_CLICKED) {
        Serial.println("[Button] *** COLOR TEST BUTTON CLICKED ***");
        static uint8_t color_index = 0;
        const lv_color_t colors[] = {
            LV_COLOR_MAKE(255, 0, 0),    // Red
            LV_COLOR_MAKE(0, 255, 0),    // Green
            LV_COLOR_MAKE(0, 0, 255),    // Blue
            LV_COLOR_MAKE(255, 255, 0),  // Yellow
            LV_COLOR_MAKE(255, 0, 255),  // Magenta
            LV_COLOR_MAKE(0, 255, 255),  // Cyan
            LV_COLOR_MAKE(255, 255, 255), // White
            LV_COLOR_MAKE(0, 0, 0),      // Black
        };
        const char* color_names[] = {
            "Red", "Green", "Blue", "Yellow", "Magenta", "Cyan", "White", "Black"
        };
        
        color_index = (color_index + 1) % 8;
        lv_obj_set_style_bg_color(lv_scr_act(), colors[color_index], 0);
        lv_label_set_text_fmt(label_color_status, "Color: %s", color_names[color_index]);
        Serial.printf("[Button] Display test: Color changed to %s\r\n", color_names[color_index]);
    } else {
        Serial.printf("[Button] Event code %d is not CLICKED, ignoring\r\n", code);
    }
}

// RFID scan callback
static void rfid_scan_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        lv_label_set_text(label_rfid_status, "Scanning...");
        lv_label_set_text(label_rfid_id, "No tag detected");
        Serial.println("RFID: Scan initiated (not implemented yet)");
        // TODO: Implement RFID scanning
    }
}

void test_mode_init() {
    Serial.println("\n=== Initializing Test Mode UI ===");
    
    // Clear screen
    lv_obj_clean(lv_scr_act());
    
    // Create tab view for different test sections
    tabview = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 30);
    
    // Create tabs
    tab_display = lv_tabview_add_tab(tabview, "Display");
    tab_touch = lv_tabview_add_tab(tabview, "Touch");
    tab_flow = lv_tabview_add_tab(tabview, "Flow");
    tab_rfid = lv_tabview_add_tab(tabview, "RFID");
    
    // === DISPLAY TAB ===
    label_display_title = lv_label_create(tab_display);
    lv_label_set_text(label_display_title, "Display Test");
    lv_obj_set_style_text_font(label_display_title, &lv_font_montserrat_14, 0);
    lv_obj_align(label_display_title, LV_ALIGN_TOP_MID, 0, 10);
    
    label_display_info = lv_label_create(tab_display);
    lv_label_set_text_fmt(label_display_info, 
        "Resolution: %dx%d\n"
        "Rotation: %d\n"
        "Colors: 16-bit RGB565",
        DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_ROTATION);
    lv_obj_align(label_display_info, LV_ALIGN_TOP_MID, 0, 40);
    
    btn_color_test = lv_btn_create(tab_display);
    lv_obj_set_size(btn_color_test, 200, 40);
    lv_obj_align(btn_color_test, LV_ALIGN_CENTER, 0, 0);
    
    // Add multiple event callbacks to catch all events
    lv_obj_add_event_cb(btn_color_test, color_test_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_color_test, color_test_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(btn_color_test, color_test_cb, LV_EVENT_RELEASED, NULL);
    
    Serial.println("[Test UI] Color test button created and event handlers registered");
    
    lv_obj_t *btn_label = lv_label_create(btn_color_test);
    lv_label_set_text(btn_label, "Test Colors");
    lv_obj_center(btn_label);
    
    label_color_status = lv_label_create(tab_display);
    lv_label_set_text(label_color_status, "Tap button to cycle colors");
    lv_obj_align(label_color_status, LV_ALIGN_BOTTOM_MID, 0, -20);
    
    // === TOUCH TAB ===
    label_touch_title = lv_label_create(tab_touch);
    lv_label_set_text(label_touch_title, "Touchscreen Test");
    lv_obj_set_style_text_font(label_touch_title, &lv_font_montserrat_14, 0);
    lv_obj_align(label_touch_title, LV_ALIGN_TOP_MID, 0, 10);
    
    label_touch_coords = lv_label_create(tab_touch);
    lv_label_set_text(label_touch_coords, "Touch coordinates:\nX: --\nY: --");
    lv_obj_align(label_touch_coords, LV_ALIGN_TOP_MID, 0, 40);
    
    label_touch_status = lv_label_create(tab_touch);
    lv_label_set_text(label_touch_status, "Touch the screen to test");
    lv_obj_align(label_touch_status, LV_ALIGN_BOTTOM_MID, 0, -20);
    
    // Create a canvas for touch visualization
    canvas_touch_area = lv_canvas_create(tab_touch);
    lv_obj_set_size(canvas_touch_area, DISPLAY_WIDTH - 40, 100);
    lv_obj_align(canvas_touch_area, LV_ALIGN_CENTER, 0, 20);
    lv_canvas_set_buffer(canvas_touch_area, 
        (lv_color_t*)malloc(sizeof(lv_color_t) * (DISPLAY_WIDTH - 40) * 100),
        DISPLAY_WIDTH - 40, 100, LV_IMG_CF_TRUE_COLOR);
    // Clear canvas
    lv_canvas_fill_bg(canvas_touch_area, lv_color_hex(0x000000), LV_OPA_COVER);
    
    // === FLOW METER TAB ===
    label_flow_title = lv_label_create(tab_flow);
    lv_label_set_text(label_flow_title, "Flow Meter Test");
    lv_obj_set_style_text_font(label_flow_title, &lv_font_montserrat_14, 0);
    lv_obj_align(label_flow_title, LV_ALIGN_TOP_MID, 0, 10);
    
    label_flow_status = lv_label_create(tab_flow);
    lv_label_set_text_fmt(label_flow_status, 
        "Pin: GPIO%d\n"
        "Status: Not connected",
        FLOW_METER_PIN);
    lv_obj_align(label_flow_status, LV_ALIGN_TOP_MID, 0, 50);
    
    label_flow_count = lv_label_create(tab_flow);
    lv_label_set_text_fmt(label_flow_count, "Pulses: %lu", flow_count);
    lv_obj_set_style_text_font(label_flow_count, &lv_font_montserrat_14, 0);
    lv_obj_align(label_flow_count, LV_ALIGN_CENTER, 0, 0);
    
    // TODO: Initialize flow meter interrupt
    
    // === RFID/NFC TAB ===
    label_rfid_title = lv_label_create(tab_rfid);
    lv_label_set_text(label_rfid_title, "RFID/NFC Test");
    lv_obj_set_style_text_font(label_rfid_title, &lv_font_montserrat_14, 0);
    lv_obj_align(label_rfid_title, LV_ALIGN_TOP_MID, 0, 10);
    
    label_rfid_status = lv_label_create(tab_rfid);
    lv_label_set_text_fmt(label_rfid_status,
        "CS Pin: GPIO%d\n"
        "RST Pin: GPIO%d\n"
        "Status: Not initialized",
        RFID_CS_PIN, RFID_RST_PIN);
    lv_obj_align(label_rfid_status, LV_ALIGN_TOP_MID, 0, 50);
    
    btn_rfid_scan = lv_btn_create(tab_rfid);
    lv_obj_set_size(btn_rfid_scan, 200, 40);
    lv_obj_align(btn_rfid_scan, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn_rfid_scan, rfid_scan_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *btn_rfid_label = lv_label_create(btn_rfid_scan);
    lv_label_set_text(btn_rfid_label, "Scan Tag");
    lv_obj_center(btn_rfid_label);
    
    label_rfid_id = lv_label_create(tab_rfid);
    lv_label_set_text(label_rfid_id, "No tag detected");
    lv_obj_align(label_rfid_id, LV_ALIGN_BOTTOM_MID, 0, -20);
    
    Serial.println("Test Mode UI initialized");
}

void test_mode_update() {
    static uint32_t last_update = 0;
    uint32_t now = millis();
    
    // Update touch coordinates every 50ms to avoid too frequent updates
    if (now - last_update > 50) {
        int16_t x, y;
        bool pressed = get_touch_state(&x, &y);
        
        if (pressed && (x != last_touch_x || y != last_touch_y)) {
            // Update touch coordinate display
            lv_label_set_text_fmt(label_touch_coords, 
                "Touch coordinates:\nX: %d\nY: %d", x, y);
            
            // Update touch status
            lv_label_set_text(label_touch_status, "Touch detected!");
            
            // Note: Canvas drawing is complex in LVGL v8, so we just show coordinates
            // The canvas is there for visual feedback, but we'll update it later if needed
            
            last_touch_x = x;
            last_touch_y = y;
        } else if (!pressed && last_touch_x >= 0) {
            // Touch released
            lv_label_set_text(label_touch_status, "Touch released");
            last_touch_x = -1;
            last_touch_y = -1;
        }
        
        last_update = now;
    }
    
    // Update flow meter count (if interrupt fires)
    // TODO: Read from interrupt counter
    
    // Update RFID status
    // TODO: Check for new tag
}
