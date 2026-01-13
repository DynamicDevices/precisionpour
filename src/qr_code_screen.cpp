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
#include "qr_code_screen.h"
#include "base_screen.h"

// System/Standard library headers
#include <lvgl.h>
#include <string.h>

// ESP-IDF framework headers
#include <esp_chip_info.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <esp_system.h>
#define TAG "qr_screen"

// Project compatibility headers
#include "esp_idf_compat.h"

// UI objects
static lv_obj_t* qr_code = NULL;
static lv_obj_t* label_qr_text = NULL;

// QR code base URL
#define QR_CODE_BASE_URL "https://precisionpour.co.uk/pay"

// Buffer for QR code URL
static char qr_code_url[128] = {0};

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
    ESP_LOGI(TAG, "\n=== Initializing QR Code Screen ===");
    
    // Create base screen layout (logo, WiFi icon, data icon)
    lv_obj_t* content_area = base_screen_create(lv_scr_act());
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
    delay(10);
    lv_timer_handler();
    
    ESP_LOGI(TAG, "[QR Screen] QR Code Screen initialized");
}

void qr_code_screen_update() {
    // Update base screen (WiFi and data icons)
    base_screen_update();
    
    // QR code is static and doesn't need updating
    // If you need dynamic QR codes, call lv_qrcode_update() here with new data
}

void qr_code_screen_cleanup() {
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
    
    ESP_LOGI(TAG, "[QR Screen] QR Code Screen cleaned up");
}
