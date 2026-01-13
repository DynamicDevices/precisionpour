/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * QR Code Screen Implementation
 * 
 * Displays QR code for payment using base_screen layout
 */

// Project headers
#include "config.h"
#include "ui/qr_code_screen.h"
#include "ui/ui_logo.h"
#include "ui/base_screen.h"
#include "ui/screen_manager.h"

// System/Standard library headers
#include <lvgl.h>
#include <string.h>

// ESP-IDF framework headers
#include <esp_chip_info.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <esp_system.h>
#define TAG "qr_screen"

// ESP-IDF framework headers
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// UI objects
static lv_obj_t* qr_code = NULL;
static lv_obj_t* label_qr_text = NULL;
static lv_obj_t* content_area = NULL;  // Store content area for event handling
static bool qr_screen_active = false;

// QR code base URL
#define QR_CODE_BASE_URL "https://precisionpour.co.uk/pay"

// Buffer for QR code URL
static char qr_code_url[128] = {0};

// Forward declaration for touch event handler
static void qr_code_touch_event_handler(lv_event_t *e);

/**
 * Read ESP32 unique chip identifier (MAC address)
 * Returns a string representation of the MAC address
 */
static void get_chip_id_string(char *buffer, size_t buffer_size) {
    uint8_t mac[6];
    
    // Read the default MAC address (unique per ESP32 chip)
    esp_err_t ret = esp_read_mac(mac, ESP_MAC_WIFI_STA);
    
    if (ret == ESP_OK) {
        // Format MAC address as hex string (e.g., "AABBCCDDEEFF")
        snprintf(buffer, buffer_size, "%02X%02X%02X%02X%02X%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        
        ESP_LOGI(TAG, "[QR Screen] ESP32 MAC Address: %02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        ESP_LOGI(TAG, "[QR Screen] Chip ID String: %s", buffer);
    } else {
        // Fallback: use chip revision or other identifier
        uint32_t chip_id = 0;
        esp_chip_info_t chip_info;
        esp_chip_info(&chip_info);
        chip_id = chip_info.revision;
        snprintf(buffer, buffer_size, "%08lX", (unsigned long)chip_id);
        ESP_LOGW(TAG, "[QR Screen] Using fallback chip ID: %s", buffer);
    }
}

void qr_code_screen_init() {
    ESP_LOGI(TAG, "=== Initializing QR Code Screen ===");
    
    // Log debug option status
    #ifdef DEBUG_QR_TAP_TO_POUR
    ESP_LOGI(TAG, "[QR Screen] DEBUG_QR_TAP_TO_POUR is defined, value: %d", DEBUG_QR_TAP_TO_POUR);
    #else
    ESP_LOGI(TAG, "[QR Screen] DEBUG_QR_TAP_TO_POUR is NOT defined");
    #endif
    
    qr_screen_active = true;
    
    // Create base screen layout (logo, WiFi icon, data icon)
    content_area = base_screen_create(lv_scr_act());
    if (content_area == NULL) {
        ESP_LOGE(TAG, "[QR Screen] ERROR: Failed to create base screen!");
        return;
    }
    
    // Read ESP32 unique chip ID (MAC address)
    char chip_id[32] = {0};
    get_chip_id_string(chip_id, sizeof(chip_id));
    
    // Build QR code URL with device ID
    snprintf(qr_code_url, sizeof(qr_code_url), "%s?id=%s", QR_CODE_BASE_URL, chip_id);
    ESP_LOGI(TAG, "[QR Screen] QR Code URL: %s", qr_code_url);
    
    // Create QR code in the content area
    ESP_LOGI(TAG, "[QR Screen] Creating QR code...");
    #if LV_USE_QRCODE
        // Calculate QR code size - maximize it to occupy most of the central screen
        // Content area: DISPLAY_HEIGHT - BASE_SCREEN_CONTENT_Y (70) - 25 (for icons) = available height
        // With content area moved up: 240 - 70 - 25 = 145px available
        int content_height = DISPLAY_HEIGHT - BASE_SCREEN_CONTENT_Y - 25;  // ~145px available
        int content_width = DISPLAY_WIDTH;  // 320px
        
        // QR code should be square - maximize size while leaving space for text
        // Use the smaller of: (width - margins) or (height - text space)
        int qr_size = content_width - 20;  // Use almost full width (300px) with small margins
        
        // But don't exceed available height minus space for text at bottom
        int max_height_for_qr = content_height - 25;  // Leave 25px for text at bottom
        if (qr_size > max_height_for_qr) {
            qr_size = max_height_for_qr;  // Limit to available height
        }
        
        ESP_LOGI(TAG, "[QR Screen] QR code size: %d (content area: %dx%d, available height: %d)", 
                 qr_size, content_width, content_height, content_height);
        
        qr_code = lv_qrcode_create(content_area, qr_size, lv_color_hex(0x000000), lv_color_hex(0xFFFFFF));
        if (qr_code != NULL) {
            // Update QR code with URL
            lv_qrcode_update(qr_code, qr_code_url, strlen(qr_code_url));
            
            // Position QR code at the top of the content area (minimal margin)
            // Content area starts at y=75, with 5px gap from logo (which ends at y=80)
            lv_obj_align(qr_code, LV_ALIGN_TOP_MID, 0, 0);  // At top of content area
            
            // Ensure QR code is on top (higher z-order than logo)
            lv_obj_move_foreground(qr_code);
            
            // Add touch event handler for debug mode (tap to pour)
            // Make QR code clickable and add event handler
            #ifdef DEBUG_QR_TAP_TO_POUR
            if (DEBUG_QR_TAP_TO_POUR) {
                // Enable clickable on QR code object
                lv_obj_add_flag(qr_code, LV_OBJ_FLAG_CLICKABLE);
                // Add event handlers to QR code (both PRESSED and CLICKED for reliability)
                lv_obj_add_event_cb(qr_code, qr_code_touch_event_handler, LV_EVENT_PRESSED, NULL);
                lv_obj_add_event_cb(qr_code, qr_code_touch_event_handler, LV_EVENT_CLICKED, NULL);
                
                // Make content area clickable and add handlers as fallback
                lv_obj_add_flag(content_area, LV_OBJ_FLAG_CLICKABLE);
                lv_obj_add_event_cb(content_area, qr_code_touch_event_handler, LV_EVENT_PRESSED, NULL);
                lv_obj_add_event_cb(content_area, qr_code_touch_event_handler, LV_EVENT_CLICKED, NULL);
                
                // Also add handler to logo container (catches touches in logo area above content area)
                lv_obj_t* logo_obj = ui_logo_get_object();
                if (logo_obj != NULL) {
                    lv_obj_t* logo_container = lv_obj_get_parent(logo_obj);
                    if (logo_container != NULL) {
                        lv_obj_add_flag(logo_container, LV_OBJ_FLAG_CLICKABLE);
                        lv_obj_add_event_cb(logo_container, qr_code_touch_event_handler, LV_EVENT_PRESSED, NULL);
                        ESP_LOGI(TAG, "[QR Screen] Debug: Added handler to logo container");
                    }
                }
                
                // Also add handler to screen itself as ultimate fallback
                // Make screen clickable to ensure it receives touch events
                lv_obj_add_flag(lv_scr_act(), LV_OBJ_FLAG_CLICKABLE);
                lv_obj_add_event_cb(lv_scr_act(), qr_code_touch_event_handler, LV_EVENT_PRESSED, NULL);
                lv_obj_add_event_cb(lv_scr_act(), qr_code_touch_event_handler, LV_EVENT_CLICKED, NULL);
                
                ESP_LOGI(TAG, "[QR Screen] Debug mode: QR code tap to pour enabled (on QR code, content area, logo, and screen)");
                ESP_LOGI(TAG, "[QR Screen] Debug: QR code bounds: x=100-220, y=70-190 (120x120), center=(160,130)");
            }
            #endif
            
            ESP_LOGI(TAG, "[QR Screen] QR code created successfully (size: %d)", qr_size);
        } else {
            ESP_LOGE(TAG, "[QR Screen] ERROR: Failed to create QR code!");
        }
    #else
        ESP_LOGW(TAG, "[QR Screen] LVGL QR code support not enabled!");
        // Fallback: show URL as text
        label_qr_text = lv_label_create(content_area);
        if (label_qr_text != NULL) {
            lv_label_set_text(label_qr_text, qr_code_url);
            lv_obj_set_style_text_color(label_qr_text, lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_text_font(label_qr_text, &lv_font_montserrat_14, 0);
            lv_obj_align(label_qr_text, LV_ALIGN_CENTER, 0, 0);
        }
    #endif
    
    // Create text label at bottom of content area (well below QR code)
    if (qr_code != NULL) {
        label_qr_text = lv_label_create(content_area);
        if (label_qr_text != NULL) {
            lv_label_set_text(label_qr_text, "Scan here to pay and pour");
            lv_obj_set_style_text_font(label_qr_text, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(label_qr_text, lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_text_align(label_qr_text, LV_TEXT_ALIGN_CENTER, 0);
            // Position text at very bottom of content area, well separated from QR code
            lv_obj_align(label_qr_text, LV_ALIGN_BOTTOM_MID, 0, 0);
        }
    }
    
    // Force refresh
    lv_timer_handler();
    vTaskDelay(pdMS_TO_TICKS(10));
    lv_timer_handler();
    
    ESP_LOGI(TAG, "[QR Screen] QR Code Screen initialized");
}

/**
 * Touch event handler for QR code (debug mode only)
 * Transitions to pouring screen with test parameters when QR code is tapped
 */
static void qr_code_touch_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* target = lv_event_get_target(e);
    
    // Log ALL events to see what we're receiving (helps debug why some touches aren't detected)
    ESP_LOGI(TAG, "[QR Screen] Debug: Event received: code=%d (%s), target=%p", 
             code, 
             (code == LV_EVENT_PRESSED) ? "PRESSED" : 
             (code == LV_EVENT_CLICKED) ? "CLICKED" : 
             (code == LV_EVENT_RELEASED) ? "RELEASED" : "OTHER",
             target);
    
    // Process PRESSED and CLICKED events
    // PRESSED is more reliable (fires immediately), CLICKED requires full press-release cycle
    if (code != LV_EVENT_PRESSED && code != LV_EVENT_CLICKED) {
        return;  // Ignore other events
    }
    
    // Get touch point immediately to check if it's in QR code region
    // This allows us to accept touches even if they hit non-clickable objects
    lv_point_t point;
    lv_indev_t* indev = lv_indev_get_act();
    bool has_touch_point = false;
    if (indev != NULL) {
        lv_indev_get_point(indev, &point);
        has_touch_point = true;
        ESP_LOGI(TAG, "[QR Screen] Debug: Touch point: (%d, %d)", point.x, point.y);
    }
    
    ESP_LOGI(TAG, "[QR Screen] Debug: Processing event: code=%d, target=%p, qr_code=%p", 
             code, target, qr_code);
    
    // If we have a touch point, check if it's in the QR code region first
    // This allows us to accept touches even if they hit objects that don't propagate events
    if (has_touch_point && qr_code != NULL) {
        // Calculate QR code bounds
        int qr_size = 120;
        int qr_center_x = DISPLAY_WIDTH / 2;  // 160
        int qr_x1 = qr_center_x - qr_size / 2;  // 100
        int qr_x2 = qr_center_x + qr_size / 2;  // 220
        int qr_y1 = BASE_SCREEN_CONTENT_Y;  // 70
        int qr_y2 = qr_y1 + qr_size;  // 190
        
        // Accept touches in QR code region with margin (for easier tapping)
        int margin = 30;
        bool in_qr_region = (point.x >= (qr_x1 - margin)) && (point.x <= (qr_x2 + margin)) &&
                           (point.y >= (qr_y1 - margin)) && (point.y <= (qr_y2 + margin));
        
        if (in_qr_region) {
            ESP_LOGI(TAG, "[QR Screen] Debug: Touch in QR code region (x=%d-%d, y=%d-%d), accepting regardless of target",
                     qr_x1 - margin, qr_x2 + margin, qr_y1 - margin, qr_y2 + margin);
            // Accept this touch and transition to pouring screen
            const char* test_unique_id = "debug_tap_order_001";
            float test_cost_per_ml = 0.005f;
            int test_max_ml = 500;
            const char* test_currency = CURRENCY_SYMBOL;
            screen_manager_show_pouring(test_unique_id, test_cost_per_ml, test_max_ml, test_currency);
            return;
        }
    }
    
    // Process the touch event based on target object
    {
        // Check if the click was on the QR code, content area, logo, or screen
        // If clicked on content area, check if it's within QR code bounds
        bool is_qr_code_click = (target == qr_code);
        bool is_content_area_click = (target == content_area);
        
        // Check if it's the logo container
        lv_obj_t* logo_obj = ui_logo_get_object();
        lv_obj_t* logo_container = (logo_obj != NULL) ? lv_obj_get_parent(logo_obj) : NULL;
        bool is_logo_click = (logo_container != NULL && target == logo_container);
        
        if (is_qr_code_click) {
            ESP_LOGI(TAG, "[QR Screen] Debug: QR code directly clicked");
        } else if (is_logo_click) {
            // Logo area clicked - for debug mode, accept if it's in the QR code region
            lv_point_t point;
            lv_indev_t* indev = lv_indev_get_act();
            if (indev != NULL) {
                lv_indev_get_point(indev, &point);
                
                // Check if touch is in QR code region (with margin)
                int qr_size = 120;
                int qr_center_x = DISPLAY_WIDTH / 2;
                int qr_x1 = qr_center_x - qr_size / 2;
                int qr_x2 = qr_center_x + qr_size / 2;
                int qr_y1 = BASE_SCREEN_CONTENT_Y;
                int margin = 30;
                
                if (point.x >= (qr_x1 - margin) && point.x <= (qr_x2 + margin) && 
                    point.y >= (qr_y1 - margin)) {
                    ESP_LOGI(TAG, "[QR Screen] Debug: Logo area clicked in QR code region, accepting");
                    is_qr_code_click = true;
                } else {
                    ESP_LOGI(TAG, "[QR Screen] Debug: Logo area clicked outside QR code region, ignoring");
                    return;
                }
            } else {
                // Can't determine, accept logo clicks for debug mode
                is_qr_code_click = true;
            }
        } else if (is_content_area_click && qr_code != NULL) {
            // Get touch point
            lv_point_t point;
            lv_indev_t* indev = lv_indev_get_act();
            if (indev != NULL) {
                lv_indev_get_point(indev, &point);
                
                // Get QR code position and size
                lv_area_t qr_area;
                lv_obj_get_coords(qr_code, &qr_area);
                
                // Log bounds for debugging
                ESP_LOGI(TAG, "[QR Screen] Debug: Touch at (%d, %d), QR code bounds: x1=%d, y1=%d, x2=%d, y2=%d (size: %dx%d)",
                         point.x, point.y, qr_area.x1, qr_area.y1, qr_area.x2, qr_area.y2,
                         qr_area.x2 - qr_area.x1 + 1, qr_area.y2 - qr_area.y1 + 1);
                
                // Use LVGL's hit test function for accurate bounds checking
                bool hit = lv_obj_hit_test(qr_code, &point);
                
                if (hit) {
                    ESP_LOGI(TAG, "[QR Screen] Debug: Content area clicked within QR code bounds (hit test passed)");
                    is_qr_code_click = true;
                } else {
                    // For debug mode, also accept clicks in the upper portion of content area
                    // (where QR code is located) even if hit test fails
                    // This handles cases where the QR code object might not cover the full visual area
                    lv_area_t content_area_coords;
                    lv_obj_get_coords(content_area, &content_area_coords);
                    
                    // Check if click is in upper 80% of content area (where QR code is)
                    int content_height = content_area_coords.y2 - content_area_coords.y1 + 1;
                    int upper_region_y = content_area_coords.y1 + (content_height * 80 / 100);
                    
                    if (point.y <= upper_region_y) {
                        ESP_LOGI(TAG, "[QR Screen] Debug: Content area clicked in upper region (QR code area), accepting");
                        is_qr_code_click = true;
                    } else {
                        ESP_LOGI(TAG, "[QR Screen] Debug: Content area clicked in lower region (text area), ignoring");
                        return;  // Not on QR code, ignore
                    }
                }
            } else {
                ESP_LOGW(TAG, "[QR Screen] Debug: Could not get touch point, assuming QR code click");
                is_qr_code_click = true;  // Assume it's a QR code click if we can't determine
            }
        } else {
            // Check if it's a screen-level touch (fallback for touches outside content area)
            // This handles touches on logo, screen background, or other non-clickable objects
            if (target == lv_scr_act()) {
                // Get touch point to check if it's in QR code area
                lv_point_t point;
                lv_indev_t* indev = lv_indev_get_act();
                if (indev != NULL) {
                    lv_indev_get_point(indev, &point);
                    
                    ESP_LOGI(TAG, "[QR Screen] Debug: Screen-level touch at (%d, %d)", point.x, point.y);
                    
                    // Calculate actual QR code bounds based on its size and position
                    // QR code: 120x120, centered horizontally, at top of content area (y=70)
                    int qr_size = 120;  // Actual QR code size (calculated in init)
                    int qr_center_x = DISPLAY_WIDTH / 2;  // 160
                    int qr_x1 = qr_center_x - qr_size / 2;  // 100
                    int qr_x2 = qr_center_x + qr_size / 2;  // 220
                    int qr_y1 = BASE_SCREEN_CONTENT_Y;  // 70
                    int qr_y2 = qr_y1 + qr_size;  // 190
                    
                    // For debug mode, accept touches in a generous region around the QR code
                    // This includes the QR code itself plus a margin for easier tapping
                    int margin = 30;  // 30px margin around QR code for easier tapping
                    int region_x1 = qr_x1 - margin;  // 70
                    int region_x2 = qr_x2 + margin;  // 250
                    int region_y1 = qr_y1 - margin;  // 40 (includes area above content area)
                    int region_y2 = qr_y2 + margin;  // 220
                    
                    // Check if touch is in the QR code region (with margin)
                    bool in_x_range = (point.x >= region_x1) && (point.x <= region_x2);
                    bool in_y_range = (point.y >= region_y1) && (point.y <= region_y2);
                    
                    if (in_x_range && in_y_range) {
                        ESP_LOGI(TAG, "[QR Screen] Debug: Screen-level touch in QR code region (x=%d-%d, y=%d-%d), accepting",
                                 region_x1, region_x2, region_y1, region_y2);
                        is_qr_code_click = true;
                    } else {
                        ESP_LOGI(TAG, "[QR Screen] Debug: Screen-level touch outside QR code region (QR: x=%d-%d, y=%d-%d), ignoring",
                                 qr_x1, qr_x2, qr_y1, qr_y2);
                        return;
                    }
                } else {
                    // Can't determine touch point, but for debug mode accept screen-level touches
                    ESP_LOGW(TAG, "[QR Screen] Debug: Could not get touch point, accepting screen-level touch");
                    is_qr_code_click = true;
                }
            } else {
                ESP_LOGI(TAG, "[QR Screen] Debug: Click on unknown target (%p), ignoring", target);
                return;
            }
        }
        
        if (is_qr_code_click) {
            ESP_LOGI(TAG, "[QR Screen] Debug: QR code tapped, transitioning to pouring screen");
            
            // Transition to pouring screen with test parameters
            const char* test_unique_id = "debug_tap_order_001";
            float test_cost_per_ml = 0.005f;  // £0.005 per ml = £5.00 per liter
            int test_max_ml = 500;  // 500ml max pour
            const char* test_currency = CURRENCY_SYMBOL;
            
            screen_manager_show_pouring(test_unique_id, test_cost_per_ml, test_max_ml, test_currency);
        }
    }
}

void qr_code_screen_update() {
    // Only update if screen is active
    if (!qr_screen_active) {
        return;
    }
    
    // Update base screen (WiFi and data icons)
    base_screen_update();
    
    // QR code is static and doesn't need updating
    // If you need dynamic QR codes, call lv_qrcode_update() here with new data
}

void qr_code_screen_cleanup() {
    // Set inactive first to prevent updates during cleanup
    qr_screen_active = false;
    
    // Clean up QR code
    if (qr_code != NULL) {
        lv_obj_del(qr_code);
        qr_code = NULL;
    }
    
    if (label_qr_text != NULL) {
        lv_obj_del(label_qr_text);
        label_qr_text = NULL;
    }
    
    // Clean up base screen (content area only, shared components persist)
    base_screen_cleanup();
    
    // Reset content area reference
    content_area = NULL;
    
    ESP_LOGI(TAG, "[QR Screen] QR Code Screen cleaned up");
}
