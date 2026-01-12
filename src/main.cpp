/**
 * ESP32 Touchscreen Display Firmware with LVGL
 * 
 * Main entry point for the firmware.
 * Uses LVGL for modern UI development.
 */

/**
 * PrecisionPour ESP32 Firmware
 * 
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

#include <Arduino.h>
#include "config.h"
#include "lvgl_display.h"
#include "lvgl_touch.h"
#include "splashscreen.h"
#include "wifi_manager.h"
#include "mqtt_client.h"
#include "flow_meter.h"
#include <SPI.h>
#include <ArduinoJson.h>

// Include mode-specific UI based on configuration
#if TEST_MODE
    #include "test_mode_ui.h"
#else
    #include "production_mode_ui.h"
    #include "pouring_mode_ui.h"
#endif

// LVGL tick timer
hw_timer_t *lvgl_timer = NULL;
portMUX_TYPE lvgl_timer_mux = portMUX_INITIALIZER_UNLOCKED;

// Screen state (production mode only)
#if !TEST_MODE
enum ScreenState {
    SCREEN_PRODUCTION,  // Default: QR code and payment screen
    SCREEN_POURING      // Active pouring: flow rate and cost screen
};
static ScreenState current_screen = SCREEN_PRODUCTION;
#endif

// LVGL tick handler (called by timer)
void IRAM_ATTR lvgl_tick_handler() {
    portENTER_CRITICAL_ISR(&lvgl_timer_mux);
    lv_tick_inc(1);  // 1ms tick
    portEXIT_CRITICAL_ISR(&lvgl_timer_mux);
}

#if !TEST_MODE
// Forward declaration
static void switch_to_production_screen();

// MQTT message handler for screen switching and commands
void on_mqtt_message(char* topic, byte* payload, unsigned int length) {
    // Null-terminate the payload
    char message[512] = {0};
    if (length < sizeof(message)) {
        memcpy(message, payload, length);
        message[length] = '\0';
    } else {
        memcpy(message, payload, sizeof(message) - 1);
    }
    
    Serial.printf("[MQTT] Received message on topic: %s\r\n", topic);
    Serial.printf("[MQTT] Payload: %s\r\n", message);
    
    // Check if this is the "paid" command topic
    if (strstr(topic, "/commands/paid") != NULL) {
        // Parse JSON payload for "paid" command
        // Expected format: {"id":"unique_id","cost_per_ml":0.005,"max_ml":500}
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, message);
        
        if (error) {
            Serial.printf("[MQTT] JSON parse error: %s\r\n", error.c_str());
            return;
        }
        
        // Extract fields
        const char* unique_id = doc["id"] | "";
        float cost_per_ml = doc["cost_per_ml"] | 0.0;
        int max_ml = doc["max_ml"] | 0;
        const char* currency = doc["currency"] | "";  // Optional: Standard ISO code "GBP" or "USD"
        
        if (strlen(unique_id) > 0 && cost_per_ml > 0 && max_ml > 0) {
            Serial.printf("[MQTT] Paid command received:\r\n");
            Serial.printf("  ID: %s\r\n", unique_id);
            Serial.printf("  Cost per ml: %.4f\r\n", cost_per_ml);
            Serial.printf("  Max ml: %d\r\n", max_ml);
            if (strlen(currency) > 0) {
                Serial.printf("  Currency: %s\r\n", currency);
            }
            
            // Start pouring with these parameters
            if (current_screen != SCREEN_POURING) {
                current_screen = SCREEN_POURING;
                pouring_mode_start_pour(unique_id, cost_per_ml, max_ml, currency);
            } else {
                // Update existing pour parameters
                pouring_mode_update_pour_params(unique_id, cost_per_ml, max_ml, currency);
            }
        } else {
            Serial.println("[MQTT] Invalid paid command - missing required fields");
        }
        return;
    }
    
    // Handle other commands on the general commands topic
    // Parse JSON-like commands (simple string matching for now)
    // Expected commands:
    // - "start_pour" or "{\"action\":\"start_pour\"}" - Switch to pouring screen
    // - "stop_pour" or "{\"action\":\"stop_pour\"}" - Switch back to production screen
    // - "cost:X.XX" or "{\"cost\":X.XX}" - Update cost per unit
    
    if (strstr(message, "start_pour") != NULL || strstr(message, "\"start_pour\"") != NULL) {
        Serial.println("[MQTT] Switching to pouring screen...");
        if (current_screen != SCREEN_POURING) {
            current_screen = SCREEN_POURING;
            pouring_mode_reset();  // Reset volume counter for new pour
            pouring_mode_init();    // Initialize pouring screen
        }
    } else if (strstr(message, "stop_pour") != NULL || strstr(message, "\"stop_pour\"") != NULL) {
        Serial.println("[MQTT] Switching to production screen...");
        if (current_screen != SCREEN_PRODUCTION) {
            current_screen = SCREEN_PRODUCTION;
            production_mode_init();  // Re-initialize production screen
        }
    } else if (strstr(message, "cost:") != NULL) {
        // Extract cost value (format: "cost:5.50")
        float cost = atof(strstr(message, "cost:") + 5);
        if (cost > 0) {
            Serial.printf("[MQTT] Updating cost per unit to: £%.2f\r\n", cost);
            pouring_mode_set_cost_per_unit(cost);
        }
    } else if (strstr(message, "\"cost\"") != NULL) {
        // Extract cost from JSON (format: {"cost":5.50})
        // Simple extraction - look for number after "cost"
        char* cost_start = strstr(message, "\"cost\"");
        if (cost_start != NULL) {
            char* colon = strchr(cost_start, ':');
            if (colon != NULL) {
                float cost = atof(colon + 1);
                if (cost > 0) {
                    Serial.printf("[MQTT] Updating cost per unit to: £%.2f\r\n", cost);
                    pouring_mode_set_cost_per_unit(cost);
                }
            }
        }
    }
}

// Callback function to switch back to production screen (called from pouring screen touch)
static void switch_to_production_screen() {
    if (current_screen != SCREEN_PRODUCTION) {
        current_screen = SCREEN_PRODUCTION;
        production_mode_init();  // Re-initialize production screen
        Serial.println("[Main] Switched back to production screen");
    }
}
#endif

void setup() {
    // Initialize serial communication
    Serial.begin(SERIAL_BAUD);
    delay(100);
    
    Serial.println("\n\nESP32 Touchscreen Display Firmware");
    Serial.println("=====================================");
    Serial.printf("Chip model: %s\r\n", ESP.getChipModel());
    Serial.printf("Chip revision: %d\r\n", ESP.getChipRevision());
    Serial.printf("CPU frequency: %d MHz\r\n", ESP.getCpuFreqMHz());
    Serial.printf("Flash size: %d bytes\r\n", ESP.getFlashChipSize());
    Serial.printf("Free heap: %d bytes\r\n", ESP.getFreeHeap());
    
    // Initialize backlight early so we can see the display
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    
    // Initialize LVGL
    lv_init();
    Serial.println("LVGL initialized");
    
    // Initialize display driver
    lvgl_display_init();
    
    // Set up LVGL tick timer (1ms)
    lvgl_timer = timerBegin(0, 80, true);  // 80MHz / 80 = 1MHz, 1 tick = 1us
    timerAttachInterrupt(lvgl_timer, &lvgl_tick_handler, true);
    timerAlarmWrite(lvgl_timer, 1000, true);  // 1000us = 1ms
    timerAlarmEnable(lvgl_timer);
    
    // Show splashscreen early (must be after display and timer are ready)
    splashscreen_init();
    splashscreen_set_progress(10);
    splashscreen_set_status("Starting up...");
    delay(200);
    
    // Initialize touch driver (30%)
    lvgl_touch_init();
    splashscreen_set_progress(30);
    splashscreen_set_status("Touch initialized");
    delay(150);
    
    // System information gathering (40%)
    splashscreen_set_progress(40);
    splashscreen_set_status("System ready");
    delay(150);
    
    // Initialize flow meter (50%)
    flow_meter_init();
    splashscreen_set_progress(50);
    splashscreen_set_status("Flow meter ready");
    delay(200);
    
    // Prepare main UI (70%)
    splashscreen_set_progress(70);
    #if TEST_MODE
        splashscreen_set_status("Loading test mode...");
    #else
        splashscreen_set_status("Loading PrecisionPour...");
    #endif
    delay(200);
    
    // Set progress to 90% BEFORE initializing UI (UI init will clear screen)
    Serial.println("[Setup] Setting progress to 90%...");
    Serial.flush();
    splashscreen_set_progress(90);
    splashscreen_set_status("Loading UI...");
    Serial.println("[Setup] Progress set to 90%");
    Serial.flush();
    delay(200);
    
    // Set background to pure black (RGB 0,0,0) BEFORE removing splashscreen
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);
    lv_timer_handler();
    delay(5);
    
    // Remove splashscreen FIRST, then initialize UI
    // (UI init clears the screen, so we need to remove splashscreen first)
    Serial.println("[Setup] Removing splashscreen before UI init...");
    Serial.flush();
    splashscreen_remove();
    Serial.println("[Setup] Splashscreen removed");
    Serial.flush();
    
    // Ensure background stays pure black (RGB 0,0,0) during transition
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);
    lv_timer_handler();
    delay(5);
    
    // Now initialize main UI (this will create the UI on clean screen)
    Serial.println("\n[Setup] About to initialize UI...");
    #if TEST_MODE
        Serial.println("[Setup] Initializing test mode UI...");
        test_mode_init();
        Serial.println("[Setup] Test mode UI initialized - DONE");
    #else
        Serial.println("[Setup] Initializing production mode UI...");
        production_mode_init();
        Serial.println("[Setup] Production mode UI initialized - DONE");
        
        // Set up callback for pouring screen to switch back to production screen
        pouring_mode_set_screen_switch_callback(switch_to_production_screen);
    #endif
    Serial.flush();
    
    // Finalize (100% - just for logging, splashscreen is already gone)
    Serial.println("[Setup] Setup sequence complete!");
    Serial.flush();
    
    Serial.println("\n========================================");
    Serial.println("SETUP COMPLETE!");
    Serial.println("========================================");
    Serial.flush();
    
    // Initialize WiFi connection
    Serial.println("\n[Setup] Initializing WiFi...");
    bool wifi_ok = wifi_manager_init();
    if (wifi_ok) {
        Serial.println("[Setup] WiFi initialized successfully");
    } else {
        Serial.println("[Setup] WiFi initialization failed, will retry in loop");
    }
    
    // Get chip ID for MQTT (same method as production UI uses)
    char chip_id[32] = {0};
    uint8_t mac[6];
    esp_err_t ret = esp_efuse_mac_get_default(mac);
    if (ret == ESP_OK) {
        snprintf(chip_id, sizeof(chip_id), "%02X%02X%02X%02X%02X%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        Serial.printf("[Setup] Chip ID: %s\r\n", chip_id);
    } else {
        Serial.println("[Setup] Failed to get chip ID");
    }
    
    // Initialize MQTT client (only if WiFi is connected)
    if (wifi_ok && strlen(chip_id) > 0) {
        Serial.println("\n[Setup] Initializing MQTT...");
        #if !TEST_MODE
            // Set MQTT message callback for screen switching
            mqtt_client_set_callback(on_mqtt_message);
        #endif
        bool mqtt_ok = mqtt_client_init(chip_id);
        if (mqtt_ok) {
            Serial.println("[Setup] MQTT initialized successfully");
        } else {
            Serial.println("[Setup] MQTT initialization failed, will retry in loop");
        }
    } else {
        Serial.println("[Setup] Skipping MQTT initialization (WiFi not connected or chip ID unavailable)");
    }
    #if TEST_MODE
        Serial.println("Running in TEST MODE");
    #else
        Serial.println("Running in PRODUCTION MODE");
    #endif
    Serial.printf("Free heap after setup: %d bytes\r\n", ESP.getFreeHeap());
}

void loop() {
    // Handle LVGL tasks (should be called every few milliseconds)
    lv_timer_handler();
    
    // WiFi connection maintenance
    wifi_manager_loop();
    
    // MQTT connection maintenance (only if WiFi is connected)
    if (wifi_manager_is_connected()) {
        mqtt_client_loop();
    }
    
    // Update flow meter (calculates flow rate and volume)
    flow_meter_update();
    
    // Update UI based on mode and current screen
    #if TEST_MODE
        test_mode_update();
    #else
        // Update the currently active screen
        if (current_screen == SCREEN_PRODUCTION) {
            production_mode_update();
        } else if (current_screen == SCREEN_POURING) {
            pouring_mode_update();
        }
    #endif
    
    // Touch controller is handled by LVGL touch driver
    // No need for continuous monitoring here
    
    // WiFi connection maintenance
    wifi_manager_loop();
    
    // MQTT connection maintenance (only if WiFi is connected)
    if (wifi_manager_is_connected()) {
        mqtt_client_loop();
    }
    
    // Small delay to prevent watchdog issues
    delay(5);
}
