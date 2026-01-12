/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * Splashscreen Implementation
 * 
 * Displays the Precision Pour splashscreen image during startup with animated progress bar.
 * Only shown in production mode (TEST_MODE = 0).
 */

#include "splashscreen.h"
#include "config.h"
#include <Arduino.h>

#if !TEST_MODE
    // Include the Precision Pour logo image (used for both splashscreen and main page)
    #include "images/precision_pour_logo.h"
#endif

static lv_obj_t *splashscreen_img = NULL;
static lv_obj_t *progress_bar = NULL;
static lv_obj_t *status_label = NULL;
static bool splashscreen_active = false;

// Progress bar dimensions (positioned at bottom of screen to match image design)
#define PROGRESS_BAR_HEIGHT 6
#define PROGRESS_BAR_MARGIN 20
#define PROGRESS_BAR_Y_OFFSET -25  // Distance from bottom (negative = up from bottom)

void splashscreen_init() {
    #if TEST_MODE
        // In test mode, use simple placeholder
        splashscreen_img = lv_obj_create(lv_scr_act());
        lv_obj_set_size(splashscreen_img, DISPLAY_WIDTH, DISPLAY_HEIGHT);
        lv_obj_set_style_bg_color(splashscreen_img, lv_color_hex(0x000000), 0);
        lv_obj_align(splashscreen_img, LV_ALIGN_CENTER, 0, 0);
        
        // Simple text label
        lv_obj_t *label = lv_label_create(splashscreen_img);
        lv_label_set_text(label, "Test Mode");
        lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    #else
        // Production mode: Use Precision Pour image
        Serial.println("[Splashscreen] Creating image object...");
        
        // Set background to pure black (RGB 0,0,0)
        lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);
        lv_timer_handler();
        delay(5);
        
        // Clear screen to ensure clean background
        lv_obj_clean(lv_scr_act());
        
        // Ensure background stays pure black (RGB 0,0,0)
        lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);
        lv_timer_handler();
        delay(5);
        
        // Create image object
        splashscreen_img = lv_img_create(lv_scr_act());
        
        // Load the embedded Precision Pour logo image
        Serial.println("[Splashscreen] Setting logo image source...");
        Serial.printf("[Splashscreen] Logo pointer: %p, data pointer: %p, data_size: %d\r\n",
                     &precision_pour_logo, precision_pour_logo.data, precision_pour_logo.data_size);
        
        if (precision_pour_logo.data == NULL) {
            Serial.println("[Splashscreen] ERROR: Logo image data is NULL!");
        } else {
            Serial.println("[Splashscreen] Logo image data is valid, setting source...");
            lv_img_set_src(splashscreen_img, &precision_pour_logo);
        }
        
        // Center the logo on the screen (logo is 280x80, display is 320x240)
        lv_obj_align(splashscreen_img, LV_ALIGN_CENTER, 0, 0);
        
        // Remove any padding
        lv_obj_set_style_pad_all(splashscreen_img, 0, 0);
        
        // Force refresh
        lv_obj_invalidate(splashscreen_img);
        lv_refr_now(NULL);
        
        Serial.println("[Splashscreen] Image source set, processing LVGL...");
        for (int i = 0; i < 10; i++) {
            lv_timer_handler();
            delay(5);
        }
        
        Serial.println("[Splashscreen] Precision Pour logo should be visible");
        Serial.printf("[Splashscreen] Logo data pointer: %p, size: %d bytes\r\n", 
                     precision_pour_logo.data, precision_pour_logo.data_size);
        
        // The image already contains the branding, so we just add the progress bar overlay
    #endif
    
    // Create progress bar at the bottom
    // Position it below the logo, centered horizontally
    progress_bar = lv_bar_create(lv_scr_act());
    lv_obj_set_size(progress_bar, DISPLAY_WIDTH - (PROGRESS_BAR_MARGIN * 2), PROGRESS_BAR_HEIGHT);
    // Position progress bar at bottom with some margin
    lv_obj_align(progress_bar, LV_ALIGN_BOTTOM_MID, 0, -20);
    
    // Style the progress bar to match the Precision Pour design
    // Background: dark gray (matches image)
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x2A2A2A), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(progress_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(progress_bar, 3, LV_PART_MAIN);
    lv_obj_set_style_border_width(progress_bar, 0, LV_PART_MAIN);
    
    // Progress indicator: golden-yellow (matches image branding color)
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0xFFD700), LV_PART_INDICATOR);  // Golden yellow
    lv_obj_set_style_bg_opa(progress_bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(progress_bar, 3, LV_PART_INDICATOR);
    lv_obj_set_style_border_width(progress_bar, 0, LV_PART_INDICATOR);
    
    // Set initial progress to 0
    lv_bar_set_value(progress_bar, 0, LV_ANIM_OFF);
    lv_bar_set_range(progress_bar, 0, 100);
    
    // Status label above progress bar
    // Note: The Precision Pour image already has "INITIALIZING SENSORS..." text,
    // so this label can be hidden or used for additional status updates
    status_label = lv_label_create(splashscreen_img);
    lv_label_set_text(status_label, "");
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_opa(status_label, LV_OPA_COVER, 0);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, PROGRESS_BAR_Y_OFFSET - 18);
    
    // Hide status label initially since image has its own text
    // Uncomment to show status updates:
    // lv_obj_clear_flag(status_label, LV_OBJ_FLAG_HIDDEN);
    
    splashscreen_active = true;
    
    // Process LVGL multiple times to ensure image is rendered
    for (int i = 0; i < 5; i++) {
        lv_timer_handler();
        delay(5);
    }
    
    #if TEST_MODE
        Serial.println("Splashscreen displayed (test mode - simple)");
    #else
        Serial.println("Splashscreen displayed (production mode - Precision Pour image)");
        Serial.printf("Image pointer: %p, Size: %dx%d\r\n", 
                     &precision_pour_logo, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    #endif
}

void splashscreen_set_progress(uint8_t percent) {
    if (progress_bar != NULL && splashscreen_active) {
        // Clamp to 0-100
        if (percent > 100) percent = 100;
        
        // Update progress bar with smooth animation
        lv_bar_set_value(progress_bar, percent, LV_ANIM_ON);
        
        // Process LVGL to update display
        lv_timer_handler();
        
        Serial.printf("Progress: %d%%\r\n", percent);
        Serial.flush();
    } else {
        Serial.printf("[Splashscreen] WARNING: Cannot set progress - bar=%p active=%d\r\n", 
                     progress_bar, splashscreen_active);
        Serial.flush();
    }
}

void splashscreen_set_status(const char *text) {
    if (status_label != NULL && splashscreen_active) {
        #if !TEST_MODE
            // In production mode, the Precision Pour image already has status text
            // We keep the label hidden since the image shows "INITIALIZING SENSORS..."
            // Uncomment to show additional status updates:
            // lv_label_set_text(status_label, text);
            // lv_obj_clear_flag(status_label, LV_OBJ_FLAG_HIDDEN);
        #else
            // In test mode, show the status text
            lv_label_set_text(status_label, text);
            lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, PROGRESS_BAR_Y_OFFSET - 18);
        #endif
        
        // Process LVGL to update display
        lv_timer_handler();
        
        Serial.printf("Status: %s\r\n", text);
    }
}

void splashscreen_remove() {
    Serial.println("[Splashscreen] Removing splashscreen elements...");
    
    // Remove progress bar FIRST (before logo) so it disappears first
    if (progress_bar != NULL) {
        Serial.println("[Splashscreen] Removing progress bar...");
        lv_obj_del(progress_bar);
        progress_bar = NULL;
        lv_timer_handler();
        delay(10);  // Small delay to ensure progress bar is removed
    }
    
    // Remove status label
    if (status_label != NULL) {
        lv_obj_del(status_label);
        status_label = NULL;
    }
    
    // Remove logo/image LAST (or at the same time)
    if (splashscreen_img != NULL) {
        Serial.println("[Splashscreen] Removing logo...");
        lv_obj_del(splashscreen_img);
        splashscreen_img = NULL;
    }
    
    splashscreen_active = false;
    
    // Process LVGL to update display
    lv_timer_handler();
    delay(10);
    lv_timer_handler();
    
    Serial.println("[Splashscreen] Splashscreen removed");
}

bool splashscreen_is_active() {
    return splashscreen_active;
}
