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
#include "wifi_manager.h"
// Use the logo image (extracted from splashscreen)
#include "images/precision_pour_logo.h"

// UI objects
static lv_obj_t *logo_container = NULL;
static lv_obj_t *qr_code = NULL;
static lv_obj_t *label_qr_text = NULL;
static lv_obj_t *wifi_status_container = NULL;  // WiFi status icon container
static lv_obj_t *wifi_arc1 = NULL;  // WiFi signal arc 1 (outermost)
static lv_obj_t *wifi_arc2 = NULL;  // WiFi signal arc 2
static lv_obj_t *wifi_arc3 = NULL;  // WiFi signal arc 3 (innermost)

// Brand colors (matching PrecisionPour branding)
#define COLOR_BACKGROUND lv_color_hex(0x000000) // Pure black background (RGB 0,0,0)
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
    // Use background color from logo image (#161716) to match perfectly
    lv_obj_set_style_bg_color(lv_scr_act(), COLOR_BACKGROUND, 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);
    lv_timer_handler();
    delay(5);
    
    // Now clear screen (background is already set)
    lv_obj_clean(lv_scr_act());
    // Re-apply background color to match logo
    lv_obj_set_style_bg_color(lv_scr_act(), COLOR_BACKGROUND, 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);
    lv_timer_handler();
    delay(5);
    
    // Create logo area at the top using the splashscreen logo image
    Serial.println("[Production UI] Creating logo from image...");
    Serial.flush();
    logo_container = lv_obj_create(lv_scr_act());
    if (logo_container == NULL) {
        Serial.println("[Production UI] ERROR: Failed to create logo container!");
        Serial.flush();
        return;
    }
    
    // Set container size to fit logo (adjust based on logo height)
    // Logo is 80px high, we'll make container 90px to move it up and add spacing
    lv_obj_set_size(logo_container, DISPLAY_WIDTH, 90);
    lv_obj_align(logo_container, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(logo_container, LV_OPA_TRANSP, 0);  // Transparent background
    lv_obj_set_style_border_width(logo_container, 0, 0);
    lv_obj_set_style_pad_all(logo_container, 0, 0);
    // Enable clipping so only the top 100px of the image is visible
    lv_obj_set_style_clip_corner(logo_container, false, 0);
    lv_obj_clear_flag(logo_container, LV_OBJ_FLAG_SCROLLABLE);  // Disable scrolling
    lv_timer_handler();
    
    // Create logo image object
    lv_obj_t *logo_img = lv_img_create(logo_container);
    if (logo_img == NULL) {
        Serial.println("[Production UI] ERROR: Failed to create logo image object!");
        Serial.flush();
        return;
    }
    
    // Set logo image source - use the logo image (properly sized)
    Serial.printf("[Production UI] Setting logo image source, data pointer: %p\r\n", precision_pour_logo.data);
    Serial.printf("[Production UI] Logo dimensions: %dx%d\r\n", 
                  precision_pour_logo.header.w, precision_pour_logo.header.h);
    
    // Set image source
    lv_img_set_src(logo_img, &precision_pour_logo);
    
    // Center the logo in the container
    lv_obj_align(logo_img, LV_ALIGN_CENTER, 0, 0);
    
    // Force refresh
    lv_obj_invalidate(logo_img);
    lv_refr_now(NULL);
    lv_timer_handler();
    delay(10);
    lv_timer_handler();
    
    Serial.println("[Production UI] Logo created from image");
    Serial.flush();
    
    // Create QR code in the center
    Serial.println("[Production UI] Creating QR code...");
    Serial.flush();
    #if LV_USE_QRCODE
        // Calculate QR code size (larger and better centered)
        lv_coord_t qr_size = 130;  // QR code size in pixels (increased for better visibility)
        
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
            
            // Position QR code 50px above center
            // Display is 240px high, center is 120px
            // Position at 70px from top (120 - 50 = 70px)
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
        
        // Position text at the bottom of the screen
        #if LV_USE_QRCODE
            if (qr_code != NULL) {
                // Position at bottom with some margin
                lv_obj_align(label_qr_text, LV_ALIGN_BOTTOM_MID, 0, -10);
            } else {
                lv_obj_align(label_qr_text, LV_ALIGN_BOTTOM_MID, 0, -30);
            }
        #else
            lv_obj_align(label_qr_text, LV_ALIGN_BOTTOM_MID, 0, -30);
        #endif
    }
    
    // Create WiFi status icon in bottom left corner
    Serial.println("[Production UI] Creating WiFi status icon...");
    wifi_status_container = lv_obj_create(lv_scr_act());
    if (wifi_status_container != NULL) {
        // Container for WiFi icon (30x30 pixels to fit the arcs)
        lv_obj_set_size(wifi_status_container, 30, 30);
        lv_obj_align(wifi_status_container, LV_ALIGN_BOTTOM_LEFT, 5, -5);
        lv_obj_set_style_bg_opa(wifi_status_container, LV_OPA_TRANSP, 0);  // Transparent background
        lv_obj_set_style_border_width(wifi_status_container, 0, 0);
        lv_obj_set_style_pad_all(wifi_status_container, 0, 0);
        lv_obj_clear_flag(wifi_status_container, LV_OBJ_FLAG_SCROLLABLE);
        
        // Create WiFi signal arcs (typical WiFi icon: 3 arcs)
        // Arc 1 (outermost) - 120 degrees, from 210 to 330
        wifi_arc1 = lv_arc_create(wifi_status_container);
        lv_obj_set_size(wifi_arc1, 24, 24);
        lv_arc_set_bg_angles(wifi_arc1, 210, 330);
        lv_arc_set_angles(wifi_arc1, 210, 330);
        lv_obj_remove_style(wifi_arc1, NULL, LV_PART_KNOB);
        lv_obj_set_style_arc_width(wifi_arc1, 2, 0);
        lv_obj_set_style_arc_color(wifi_arc1, lv_color_hex(0xFF0000), 0);  // Red initially
        lv_obj_align(wifi_arc1, LV_ALIGN_CENTER, 0, 0);
        
        // Arc 2 (middle) - 120 degrees, from 210 to 330
        wifi_arc2 = lv_arc_create(wifi_status_container);
        lv_obj_set_size(wifi_arc2, 18, 18);
        lv_arc_set_bg_angles(wifi_arc2, 210, 330);
        lv_arc_set_angles(wifi_arc2, 210, 330);
        lv_obj_remove_style(wifi_arc2, NULL, LV_PART_KNOB);
        lv_obj_set_style_arc_width(wifi_arc2, 2, 0);
        lv_obj_set_style_arc_color(wifi_arc2, lv_color_hex(0xFF0000), 0);  // Red initially
        lv_obj_align(wifi_arc2, LV_ALIGN_CENTER, 0, 0);
        
        // Arc 3 (innermost) - 120 degrees, from 210 to 330
        wifi_arc3 = lv_arc_create(wifi_status_container);
        lv_obj_set_size(wifi_arc3, 12, 12);
        lv_arc_set_bg_angles(wifi_arc3, 210, 330);
        lv_arc_set_angles(wifi_arc3, 210, 330);
        lv_obj_remove_style(wifi_arc3, NULL, LV_PART_KNOB);
        lv_obj_set_style_arc_width(wifi_arc3, 2, 0);
        lv_obj_set_style_arc_color(wifi_arc3, lv_color_hex(0xFF0000), 0);  // Red initially
        lv_obj_align(wifi_arc3, LV_ALIGN_CENTER, 0, 0);
        
        Serial.println("[Production UI] WiFi status icon created");
    }
    lv_timer_handler();
    
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
    // Update WiFi status icon
    if (wifi_status_container != NULL && wifi_arc1 != NULL && wifi_arc2 != NULL && wifi_arc3 != NULL) {
        bool wifi_connected = wifi_manager_is_connected();
        
        // Update icon color based on connection status
        // Green = connected, Red = disconnected/problem
        static bool last_wifi_state = false;
        if (wifi_connected != last_wifi_state) {
            lv_color_t icon_color = wifi_connected ? lv_color_hex(0x00FF00) : lv_color_hex(0xFF0000);
            
            // Update all three arcs
            lv_obj_set_style_arc_color(wifi_arc1, icon_color, 0);
            lv_obj_set_style_arc_color(wifi_arc2, icon_color, 0);
            lv_obj_set_style_arc_color(wifi_arc3, icon_color, 0);
            
            last_wifi_state = wifi_connected;
            
            // Force redraw
            lv_obj_invalidate(wifi_arc1);
            lv_obj_invalidate(wifi_arc2);
            lv_obj_invalidate(wifi_arc3);
        }
    }
    
    // Update other UI elements as needed
    // - Update flow meter readings
    // - Update RFID tag status
    // - Update pour status
    
    // Note: QR code is static and doesn't need updating
    // If you need dynamic QR codes, call lv_qrcode_update() here with new data
}
