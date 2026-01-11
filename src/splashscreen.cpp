/**
 * Splashscreen Implementation
 * 
 * Displays the splashscreen image during startup with animated progress bar.
 * The progress bar is positioned at the bottom to match the image design.
 */

#include "splashscreen.h"
#include "config.h"
#include <Arduino.h>

static lv_obj_t *splashscreen_img = NULL;
static lv_obj_t *progress_bar = NULL;
static lv_obj_t *status_label = NULL;
static bool splashscreen_active = false;

// Progress bar dimensions (positioned at bottom of screen)
#define PROGRESS_BAR_HEIGHT 8
#define PROGRESS_BAR_MARGIN 20
#define PROGRESS_BAR_Y_OFFSET -30  // Distance from bottom (negative = up from bottom)

void splashscreen_init() {
    // Create a full-screen image object
    splashscreen_img = lv_img_create(lv_scr_act());
    
    // Try to load from filesystem first (if using LittleFS/SPIFFS)
    // Uncomment if you've uploaded the image to filesystem:
    // lv_img_set_src(splashscreen_img, "S:/splashscreen.png");
    
    // Alternative: Use embedded image (convert PNG to C array first)
    // extern const lv_img_dsc_t splashscreen_img_dsc;
    // lv_img_set_src(splashscreen_img, &splashscreen_img_dsc);
    
    // For now, create a colored background with text as placeholder
    // Replace this with actual image loading once converted
    lv_obj_set_size(splashscreen_img, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_obj_set_style_bg_color(splashscreen_img, lv_color_hex(0x000000), 0);
    lv_obj_align(splashscreen_img, LV_ALIGN_CENTER, 0, 0);
    
    // Add title text as placeholder
    lv_obj_t *label = lv_label_create(splashscreen_img);
    lv_label_set_text(label, "Precision Pour");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -50);
    
    // Create progress bar at the bottom (matching image design)
    progress_bar = lv_bar_create(splashscreen_img);
    lv_obj_set_size(progress_bar, DISPLAY_WIDTH - (PROGRESS_BAR_MARGIN * 2), PROGRESS_BAR_HEIGHT);
    lv_obj_align(progress_bar, LV_ALIGN_BOTTOM_MID, 0, PROGRESS_BAR_Y_OFFSET);
    
    // Style the progress bar to match the image
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x333333), LV_PART_MAIN);  // Background (dark gray)
    lv_obj_set_style_bg_opa(progress_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(progress_bar, 2, LV_PART_MAIN);
    
    // Progress indicator color (bright color to match typical progress bars)
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x00FF00), LV_PART_INDICATOR);  // Green
    lv_obj_set_style_bg_opa(progress_bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(progress_bar, 2, LV_PART_INDICATOR);
    
    // Set initial progress to 0
    lv_bar_set_value(progress_bar, 0, LV_ANIM_OFF);
    lv_bar_set_range(progress_bar, 0, 100);
    
    // Optional: Status label above progress bar
    status_label = lv_label_create(splashscreen_img);
    lv_label_set_text(status_label, "Initializing...");
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xCCCCCC), 0);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, PROGRESS_BAR_Y_OFFSET - 20);
    
    splashscreen_active = true;
    
    // Process LVGL to show initial state
    lv_timer_handler();
    
    Serial.println("Splashscreen displayed with progress bar");
}

void splashscreen_set_progress(uint8_t percent) {
    if (progress_bar != NULL && splashscreen_active) {
        // Clamp to 0-100
        if (percent > 100) percent = 100;
        
        // Update progress bar with smooth animation
        lv_bar_set_value(progress_bar, percent, LV_ANIM_ON);
        
        // Process LVGL to update display
        lv_timer_handler();
        
        Serial.printf("Progress: %d%%\n", percent);
    }
}

void splashscreen_set_status(const char *text) {
    if (status_label != NULL && splashscreen_active) {
        lv_label_set_text(status_label, text);
        lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, PROGRESS_BAR_Y_OFFSET - 20);
        
        // Process LVGL to update display
        lv_timer_handler();
        
        Serial.printf("Status: %s\n", text);
    }
}

void splashscreen_remove() {
    if (splashscreen_img != NULL) {
        lv_obj_del(splashscreen_img);
        splashscreen_img = NULL;
        progress_bar = NULL;
        status_label = NULL;
        splashscreen_active = false;
        Serial.println("Splashscreen removed");
    }
}

bool splashscreen_is_active() {
    return splashscreen_active;
}
