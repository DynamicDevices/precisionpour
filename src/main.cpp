/**
 * ESP32 Touchscreen Display Firmware with LVGL
 * 
 * Main entry point for the firmware.
 * Uses LVGL for modern UI development.
 */

#include <Arduino.h>
#include "config.h"
#include "lvgl_display.h"
#include "lvgl_touch.h"
#include "splashscreen.h"
#include "wifi_manager.h"
#include "mqtt_client.h"
#include <SPI.h>

// Include mode-specific UI based on configuration
#if TEST_MODE
    #include "test_mode_ui.h"
#else
    #include "production_mode_ui.h"
#endif

// LVGL tick timer
hw_timer_t *lvgl_timer = NULL;
portMUX_TYPE lvgl_timer_mux = portMUX_INITIALIZER_UNLOCKED;

// LVGL tick handler (called by timer)
void IRAM_ATTR lvgl_tick_handler() {
    portENTER_CRITICAL_ISR(&lvgl_timer_mux);
    lv_tick_inc(1);  // 1ms tick
    portEXIT_CRITICAL_ISR(&lvgl_timer_mux);
}

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
    
    // Hardware initialization complete (50%)
    splashscreen_set_progress(50);
    splashscreen_set_status("Hardware ready");
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
    
    // Update UI based on mode
    #if TEST_MODE
        test_mode_update();
    #else
        production_mode_update();
    #endif
    
    // Continuous IRQ pin monitoring (every 100ms) to catch ANY state changes
    static uint32_t last_irq_check = 0;
    static int last_irq_state = -1;
    uint32_t now = millis();
    if (now - last_irq_check > 100) {
        if (TOUCH_IRQ >= 0) {
            int current_irq_state = digitalRead(TOUCH_IRQ);
            if (current_irq_state != last_irq_state) {
                Serial.printf("\n[Main Loop] *** IRQ PIN STATE CHANGED: %s -> %s (pin %d) ***\r\n",
                            last_irq_state == HIGH ? "HIGH" : (last_irq_state == LOW ? "LOW" : "UNKNOWN"),
                            current_irq_state == HIGH ? "HIGH" : "LOW",
                            TOUCH_IRQ);
                Serial.flush();
                last_irq_state = current_irq_state;
            }
        }
        last_irq_check = now;
    }
    
    // Periodic touch controller test (every 3 seconds)
    static uint32_t last_touch_test = 0;
    if (now - last_touch_test > 3000) {
        Serial.println("\n[Main Loop] Testing touch controller...");
        
        // Test IRQ pin
        if (TOUCH_IRQ >= 0) {
            int irq_state = digitalRead(TOUCH_IRQ);
            Serial.printf("[Main Loop] IRQ pin state: %s (pin %d)\r\n", 
                        irq_state == LOW ? "LOW" : "HIGH", TOUCH_IRQ);
        }
        
        // Test SPI communication by reading raw values
        // Note: This will conflict with LVGL's touch reading, but it's just for debugging
        Serial.println("[Main Loop] Reading raw touch values via SPI...");
        // We'll do a quick read to see if SPI is working
        // (This is a simplified test - actual reading is in lvgl_touch.cpp)
        
        last_touch_test = now;
    }
    
    // WiFi connection maintenance
    wifi_manager_loop();
    
    // MQTT connection maintenance (only if WiFi is connected)
    if (wifi_manager_is_connected()) {
        mqtt_client_loop();
    }
    
    // Small delay to prevent watchdog issues
    delay(5);
}
