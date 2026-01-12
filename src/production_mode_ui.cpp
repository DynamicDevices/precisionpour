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
#include "mqtt_client.h"
// Use the logo image (extracted from splashscreen)
#include "images/precision_pour_logo.h"

// UI objects
static lv_obj_t *logo_container = NULL;
static lv_obj_t *qr_code = NULL;
static lv_obj_t *label_qr_text = NULL;
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
    
    // Create WiFi status icon in bottom left corner (vertical signal bars)
    Serial.println("[Production UI] Creating WiFi status icon...");
    wifi_status_container = lv_obj_create(lv_scr_act());
    if (wifi_status_container != NULL) {
        // Container for WiFi icon (24x20 pixels to fit signal bars)
        lv_obj_set_size(wifi_status_container, 24, 20);
        lv_obj_align(wifi_status_container, LV_ALIGN_BOTTOM_LEFT, 5, -5);
        lv_obj_set_style_bg_opa(wifi_status_container, LV_OPA_TRANSP, 0);  // Transparent background
        lv_obj_set_style_border_width(wifi_status_container, 0, 0);
        lv_obj_set_style_pad_all(wifi_status_container, 0, 0);
        lv_obj_clear_flag(wifi_status_container, LV_OBJ_FLAG_SCROLLABLE);
        
        // Create WiFi signal bars (4 bars of increasing height)
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
        
        Serial.println("[Production UI] WiFi status icon created");
    }
    lv_timer_handler();
    
    // Create communication activity icon in bottom right corner (electric pulse/spark)
    Serial.println("[Production UI] Creating communication activity icon...");
    comm_status_container = lv_obj_create(lv_scr_act());
    if (comm_status_container != NULL) {
        // Container for communication icon (20x20 pixels)
        lv_obj_set_size(comm_status_container, 20, 20);
        lv_obj_align(comm_status_container, LV_ALIGN_BOTTOM_RIGHT, -5, -5);
        lv_obj_set_style_bg_opa(comm_status_container, LV_OPA_TRANSP, 0);  // Transparent background
        lv_obj_set_style_border_width(comm_status_container, 0, 0);
        lv_obj_set_style_pad_all(comm_status_container, 0, 0);
        lv_obj_clear_flag(comm_status_container, LV_OBJ_FLAG_SCROLLABLE);
        
        // Create spark/pulse elements (zigzag pattern like electric pulse)
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
        
        Serial.println("[Production UI] Communication activity icon created");
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
        // RSSI ranges: Excellent: >-50, Good: -50 to -60, Fair: -60 to -70, Weak: -70 to -80, Very Weak: <-80
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
        // When disconnected (bars_to_show = 0), show all bars at 40% opacity so they're still visible
        // When connected, show bars based on signal strength
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
    
    // Update communication activity icon (electric pulse/spark)
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
    
    // Update other UI elements as needed
    // - Update flow meter readings
    // - Update RFID tag status
    // - Update pour status
    
    // Note: QR code is static and doesn't need updating
    // If you need dynamic QR codes, call lv_qrcode_update() here with new data
}
