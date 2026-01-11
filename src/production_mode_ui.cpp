/**
 * Production Mode UI Implementation
 * 
 * PrecisionPour branded interface with logo, QR code, and payment instructions
 */

#include "production_mode_ui.h"
#include "config.h"
#include <lvgl.h>
#include <Arduino.h>
#include <string.h>
#include "esp_efuse.h"
#include "esp_system.h"

// UI objects
static lv_obj_t *logo_container = NULL;
static lv_obj_t *qr_code = NULL;
static lv_obj_t *label_qr_text = NULL;

// Brand colors (matching PrecisionPour branding)
#define COLOR_BACKGROUND lv_color_hex(0x1A1A1A) // Dark gray/black
#define COLOR_TEXT lv_color_hex(0xFFFFFF) // White
#define COLOR_GOLDEN lv_color_hex(0xFFD700) // Golden yellow (branding color)

// QR code base URL - includes device MAC address for unique per-device URLs
// The QR code is generated once at initialization (not dynamically updated)
#define QR_CODE_BASE_URL "https://precisionpour.co.uk/pay"  // Base payment URL

// Buffer for QR code URL (base URL + optional device ID)
static char qr_code_url[128] = {0};

/**
 * Read ESP32 unique chip identifier (MAC address)
 * Returns a string representation of the MAC address
 */
static void get_chip_id_string(char *buffer, size_t buffer_size) {
    uint8_t mac[6];
    
    // Read the default MAC address (unique per ESP32 chip)
    esp_err_t ret = esp_efuse_mac_get_default(mac);
    
    if (ret == ESP_OK) {
        // Format MAC address as hex string (e.g., "AABBCCDDEEFF")
        snprintf(buffer, buffer_size, "%02X%02X%02X%02X%02X%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        
        Serial.printf("[Production UI] ESP32 MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        Serial.printf("[Production UI] Chip ID String: %s\r\n", buffer);
    } else {
        // Fallback: use chip revision or other identifier
        uint32_t chip_id = 0;
        for (int i = 0; i < 17; i += 8) {
            chip_id |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
        }
        snprintf(buffer, buffer_size, "%08X", chip_id);
        Serial.printf("[Production UI] Using fallback chip ID: %s\r\n", buffer);
    }
    Serial.flush();
}

void production_mode_init() {
    Serial.println("\n=== Initializing Production Mode UI ===");
    Serial.flush();
    
    // Read ESP32 unique chip ID (MAC address)
    char chip_id[32] = {0};
    get_chip_id_string(chip_id, sizeof(chip_id));
    
    // Build QR code URL with device ID: https://precisionpour.co.uk?id=XXX
    snprintf(qr_code_url, sizeof(qr_code_url), "%s?id=%s", QR_CODE_BASE_URL, chip_id);
    
    Serial.printf("[Production UI] QR Code URL: %s\r\n", qr_code_url);
    Serial.flush();
    
    // Ensure LVGL is ready
    if (lv_scr_act() == NULL) {
        Serial.println("[Production UI] ERROR: No active screen!");
        Serial.flush();
        return;
    }
    
    // Set background color FIRST (before clearing) to prevent white flash
    lv_obj_set_style_bg_color(lv_scr_act(), COLOR_BACKGROUND, 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);
    lv_timer_handler();
    delay(5);
    
    // Now clear screen (background is already set to black)
    lv_obj_clean(lv_scr_act());
    lv_timer_handler();
    delay(5);
    
    // Create logo area at the top using text labels (simpler and more reliable)
    Serial.println("[Production UI] Creating logo...");
    Serial.flush();
    logo_container = lv_obj_create(lv_scr_act());
    if (logo_container == NULL) {
        Serial.println("[Production UI] ERROR: Failed to create logo container!");
        Serial.flush();
        return;
    }
    
    lv_obj_set_size(logo_container, DISPLAY_WIDTH, 80);
    lv_obj_align(logo_container, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(logo_container, LV_OPA_TRANSP, 0);  // Transparent background
    lv_obj_set_style_border_width(logo_container, 0, 0);
    lv_obj_set_style_pad_all(logo_container, 0, 0);
    lv_timer_handler();
    
    // Create "PRECISION POUR" title text (matching splashscreen style)
    lv_obj_t *title_label = lv_label_create(logo_container);
    if (title_label != NULL) {
        lv_label_set_text(title_label, "PRECISION POUR");
        lv_obj_set_style_text_font(title_label, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(title_label, COLOR_TEXT, 0);
        lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 10);
    }
    
    // Create decorative line below title (golden yellow to match branding)
    lv_obj_t *line = lv_obj_create(logo_container);
    if (line != NULL) {
        lv_obj_set_size(line, 200, 2);
        lv_obj_align(line, LV_ALIGN_TOP_MID, 0, 35);
        lv_obj_set_style_bg_color(line, COLOR_GOLDEN, 0);
        lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(line, 0, 0);
        lv_obj_set_style_radius(line, 0, 0);
    }
    
    lv_timer_handler();
    Serial.println("[Production UI] Logo created (text-based)");
    Serial.flush();
    
    // Create QR code in the center
    Serial.println("[Production UI] Creating QR code...");
    Serial.flush();
    #if LV_USE_QRCODE
        // Calculate QR code size (fit in remaining space)
        lv_coord_t qr_size = 120;  // QR code size in pixels
        
        // Create QR code with size and colors (LVGL v8 API)
        // QR code is generated once at initialization - it's not dynamically updated
        qr_code = lv_qrcode_create(lv_scr_act(), qr_size, lv_color_black(), lv_color_white());
        
        if (qr_code == NULL) {
            Serial.println("[Production UI] ERROR: Failed to create QR code!");
            Serial.flush();
        } else {
            // Set QR code data (generated once, not dynamic)
            // URL can include device ID - see get_chip_id_string() above
            lv_qrcode_update(qr_code, qr_code_url, strlen(qr_code_url));
            
            // Position QR code below logo, with proper spacing
            // Logo is ~80px high, QR code is 120px
            // Position QR code at y=70 (80px logo - 10px overlap for tighter spacing)
            lv_obj_align(qr_code, LV_ALIGN_TOP_MID, 0, 70);
            lv_timer_handler();
            
            Serial.println("[Production UI] QR code created");
            Serial.printf("[Production UI] QR code URL: %s (static, generated at init)\r\n", qr_code_url);
            Serial.flush();
        }
    #else
        Serial.println("[Production UI] ERROR: QR code support not enabled in lv_conf.h!");
        Serial.flush();
        // Create placeholder if QR code not available
        lv_obj_t *placeholder = lv_label_create(lv_scr_act());
        if (placeholder != NULL) {
            lv_label_set_text(placeholder, "QR Code\n(Enable LV_USE_QRCODE)");
            lv_obj_set_style_text_font(placeholder, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(placeholder, COLOR_TEXT, 0);
            lv_obj_align(placeholder, LV_ALIGN_CENTER, 0, 30);
        }
    #endif
    
    // Create text label below QR code
    Serial.println("[Production UI] Creating QR text label...");
    Serial.flush();
    label_qr_text = lv_label_create(lv_scr_act());
    if (label_qr_text != NULL) {
        lv_label_set_text(label_qr_text, "Scan here to pay and pour");
        lv_obj_set_style_text_font(label_qr_text, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(label_qr_text, COLOR_TEXT, 0);
        lv_obj_set_style_text_align(label_qr_text, LV_TEXT_ALIGN_CENTER, 0);
        
        // Position text below QR code with proper spacing
        // QR code is at y=70, height=120, so bottom is at y=190
        // Position text at y=200 (10px spacing below QR code)
        #if LV_USE_QRCODE
            if (qr_code != NULL) {
                // Position below QR code with spacing (QR code bottom is at ~190px)
                lv_obj_align(label_qr_text, LV_ALIGN_TOP_MID, 0, 200);
            } else {
                lv_obj_align(label_qr_text, LV_ALIGN_BOTTOM_MID, 0, -30);
            }
        #else
            lv_obj_align(label_qr_text, LV_ALIGN_BOTTOM_MID, 0, -30);
        #endif
    }
    
    // Force refresh to show everything
    for (int i = 0; i < 5; i++) {
        lv_timer_handler();
        delay(5);
    }
    
    Serial.println("Production Mode UI initialized");
    Serial.println("Note: QR code is generated once at initialization (not dynamically updated)");
    Serial.flush();
}

void production_mode_update() {
    // Update UI elements as needed
    // - Update flow meter readings
    // - Update RFID tag status
    // - Update pour status
    
    // Note: QR code is static and doesn't need updating
    // If you need dynamic QR codes, call lv_qrcode_update() here with new data
}
