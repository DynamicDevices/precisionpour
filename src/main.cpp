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

// Project headers (include config.h first for ENABLE_WATCHDOG definition)
#include "config.h"

// System/Standard library headers
// ESP-IDF framework headers
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#if ENABLE_WATCHDOG
#include <esp_task_wdt.h>
#endif
#define TAG_MAIN "main"

// Third-party library headers
#include <ArduinoJson.h>

// Project headers (continued)
#include "esp_idf_compat.h"
#include "esp_system_compat.h"
#include "flow_meter.h"
#include "lvgl_display.h"
#include "lvgl_touch.h"
#include "mqtt_manager.h"
#include "splashscreen.h"
#include "wifi_manager.h"

// Include mode-specific UI based on configuration
#if TEST_MODE
    #include "test_mode_ui.h"
#else
    #include "production_mode_ui.h"
    #include "pouring_mode_ui.h"
#endif

// LVGL tick timer
// hw_timer_t is defined in esp_idf_compat.h
#ifdef ESP_PLATFORM
    static hw_timer_t* lvgl_timer = NULL;
    static portMUX_TYPE lvgl_timer_mux = portMUX_INITIALIZER_UNLOCKED;
#else
    hw_timer_t *lvgl_timer = NULL;
    portMUX_TYPE lvgl_timer_mux = portMUX_INITIALIZER_UNLOCKED;
#endif

// Error recovery tracking
static unsigned int consecutive_errors = 0;
static unsigned long last_error_time = 0;

// Screen state (production mode only)
#if !TEST_MODE
enum ScreenState {
    SCREEN_PRODUCTION,  // Default: QR code and payment screen
    SCREEN_POURING      // Active pouring: flow rate and cost screen
};
static ScreenState current_screen = SCREEN_PRODUCTION;
#endif

// LVGL tick handler (called by timer)
#ifdef ESP_PLATFORM
// ESP-IDF: Timer callback runs in task context (not ISR), so use portENTER_CRITICAL
void lvgl_tick_handler(void* arg) {
    portENTER_CRITICAL(&lvgl_timer_mux);
    lv_tick_inc(1);  // 1ms tick
    portEXIT_CRITICAL(&lvgl_timer_mux);
}
#else
void IRAM_ATTR lvgl_tick_handler() {
    portENTER_CRITICAL_ISR(&lvgl_timer_mux);
    lv_tick_inc(1);  // 1ms tick
    portEXIT_CRITICAL_ISR(&lvgl_timer_mux);
}
#endif

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
    
    #ifdef ESP_PLATFORM
        ESP_LOGI(TAG_MQTT, "[MQTT] Received message on topic: %s", topic);
        ESP_LOGI(TAG_MQTT, "[MQTT] Payload: %s", message);
    #else
        Serial.printf("[MQTT] Received message on topic: %s\r\n", topic);
        Serial.printf("[MQTT] Payload: %s\r\n", message);
    #endif
    
    // Check if this is the "paid" command topic
    if (strstr(topic, "/commands/paid") != NULL) {
        // Parse JSON payload for "paid" command
        // Expected format: {"id":"unique_id","cost_per_ml":0.005,"max_ml":500,"currency":"GBP"}
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, message);
        
        if (error) {
            #ifdef ESP_PLATFORM
                ESP_LOGE(TAG_MQTT, "[MQTT] JSON parse error: %s", error.c_str());
            #else
                Serial.printf("[MQTT] JSON parse error: %s\r\n", error.c_str());
            #endif
            consecutive_errors++;
            last_error_time = millis();
            return;
        }
        
        // Validate JSON structure and extract fields with bounds checking
        const char* unique_id = doc["id"] | "";
        float cost_per_ml = doc["cost_per_ml"] | 0.0;
        int max_ml = doc["max_ml"] | 0;
        const char* currency = doc["currency"] | "";  // Optional: Standard ISO code "GBP" or "USD"
        
        // Validate fields with reasonable bounds
        bool valid = true;
        if (strlen(unique_id) == 0 || strlen(unique_id) > 128) {
            #ifdef ESP_PLATFORM
                ESP_LOGW(TAG_MQTT, "[MQTT] Invalid unique_id: empty or too long");
            #else
                Serial.println("[MQTT] Invalid unique_id: empty or too long");
            #endif
            valid = false;
        }
        if (cost_per_ml <= 0.0 || cost_per_ml > 1000.0) {
            #ifdef ESP_PLATFORM
                ESP_LOGW(TAG_MQTT, "[MQTT] Invalid cost_per_ml: %.4f (must be 0 < cost <= 1000)", cost_per_ml);
            #else
                Serial.printf("[MQTT] Invalid cost_per_ml: %.4f (must be 0 < cost <= 1000)\r\n", cost_per_ml);
            #endif
            valid = false;
        }
        if (max_ml <= 0 || max_ml > 100000) {
            #ifdef ESP_PLATFORM
                ESP_LOGW(TAG_MQTT, "[MQTT] Invalid max_ml: %d (must be 0 < max <= 100000)", max_ml);
            #else
                Serial.printf("[MQTT] Invalid max_ml: %d (must be 0 < max <= 100000)\r\n", max_ml);
            #endif
            valid = false;
        }
        if (strlen(currency) > 0 && strcmp(currency, "GBP") != 0 && strcmp(currency, "USD") != 0) {
            #ifdef ESP_PLATFORM
                ESP_LOGW(TAG_MQTT, "[MQTT] Invalid currency: %s (must be GBP or USD)", currency);
            #else
                Serial.printf("[MQTT] Invalid currency: %s (must be GBP or USD)\r\n", currency);
            #endif
            valid = false;
        }
        
        if (valid && strlen(unique_id) > 0 && cost_per_ml > 0 && max_ml > 0) {
            // Reset error counter on successful parse
            consecutive_errors = 0;
            #ifdef ESP_PLATFORM
                ESP_LOGI(TAG_MQTT, "[MQTT] Paid command received:");
                ESP_LOGI(TAG_MQTT, "  ID: %s", unique_id);
                ESP_LOGI(TAG_MQTT, "  Cost per ml: %.4f", cost_per_ml);
                ESP_LOGI(TAG_MQTT, "  Max ml: %d", max_ml);
                if (strlen(currency) > 0) {
                    ESP_LOGI(TAG_MQTT, "  Currency: %s", currency);
                }
            #else
                Serial.printf("[MQTT] Paid command received:\r\n");
                Serial.printf("  ID: %s\r\n", unique_id);
                Serial.printf("  Cost per ml: %.4f\r\n", cost_per_ml);
                Serial.printf("  Max ml: %d\r\n", max_ml);
                if (strlen(currency) > 0) {
                    Serial.printf("  Currency: %s\r\n", currency);
                }
            #endif
            
            // Start pouring with these parameters
            if (current_screen != SCREEN_POURING) {
                current_screen = SCREEN_POURING;
                pouring_mode_start_pour(unique_id, cost_per_ml, max_ml, currency);
            } else {
                // Update existing pour parameters
                pouring_mode_update_pour_params(unique_id, cost_per_ml, max_ml, currency);
            }
        } else {
            #ifdef ESP_PLATFORM
                ESP_LOGW(TAG_MQTT, "[MQTT] Invalid paid command - validation failed");
            #else
                Serial.println("[MQTT] Invalid paid command - validation failed");
            #endif
            consecutive_errors++;
            last_error_time = millis();
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
        #ifdef ESP_PLATFORM
            ESP_LOGI(TAG_MQTT, "[MQTT] Switching to pouring screen...");
        #else
            Serial.println("[MQTT] Switching to pouring screen...");
        #endif
        if (current_screen != SCREEN_POURING) {
            current_screen = SCREEN_POURING;
            pouring_mode_reset();  // Reset volume counter for new pour
            pouring_mode_init();    // Initialize pouring screen
        }
    } else if (strstr(message, "stop_pour") != NULL || strstr(message, "\"stop_pour\"") != NULL) {
        #ifdef ESP_PLATFORM
            ESP_LOGI(TAG_MQTT, "[MQTT] Switching to production screen...");
        #else
            Serial.println("[MQTT] Switching to production screen...");
        #endif
        if (current_screen != SCREEN_PRODUCTION) {
            current_screen = SCREEN_PRODUCTION;
            production_mode_init();  // Re-initialize production screen
        }
    } else if (strstr(message, "cost:") != NULL) {
        // Extract cost value (format: "cost:5.50")
        float cost = atof(strstr(message, "cost:") + 5);
        if (cost > 0) {
            #ifdef ESP_PLATFORM
                ESP_LOGI(TAG_MQTT, "[MQTT] Updating cost per unit to: £%.2f", cost);
            #else
                Serial.printf("[MQTT] Updating cost per unit to: £%.2f\r\n", cost);
            #endif
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
                    #ifdef ESP_PLATFORM
                        ESP_LOGI(TAG_MQTT, "[MQTT] Updating cost per unit to: £%.2f", cost);
                    #else
                        Serial.printf("[MQTT] Updating cost per unit to: £%.2f\r\n", cost);
                    #endif
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
        #ifdef ESP_PLATFORM
            ESP_LOGI(TAG_MAIN, "[Main] Switched back to production screen");
        #else
            Serial.println("[Main] Switched back to production screen");
        #endif
    }
}
#endif

#ifdef ESP_PLATFORM
// ESP-IDF entry point
extern "C" void app_main() {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize serial communication (UART for ESP-IDF)
    // Serial logging is handled by ESP_LOG
    
    // Initialize watchdog timer if enabled
    #if ENABLE_WATCHDOG
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = WATCHDOG_TIMEOUT_SEC * 1000,
        .idle_core_mask = 0,
        .trigger_panic = true
    };
    esp_task_wdt_init(&wdt_config);
    // Don't add app_main to watchdog - it will exit quickly
    // The main loop task will be added after it's created
    ESP_LOGI(TAG_MAIN, "[Setup] Watchdog enabled (%d second timeout)", WATCHDOG_TIMEOUT_SEC);
    #endif
    
    // Initialize error tracking
    consecutive_errors = 0;
    last_error_time = 0;
    
    ESP_LOGI(TAG_MAIN, "\n\nESP32 Touchscreen Display Firmware");
    ESP_LOGI(TAG_MAIN, "=====================================");
    ESP_LOGI(TAG_MAIN, "Chip model: %s", ESP.getChipModel());
    ESP_LOGI(TAG_MAIN, "Chip revision: %d", ESP.getChipRevision());
    ESP_LOGI(TAG_MAIN, "CPU frequency: %d MHz", ESP.getCpuFreqMHz());
    ESP_LOGI(TAG_MAIN, "Flash size: %d bytes", ESP.getFlashChipSize());
    ESP_LOGI(TAG_MAIN, "Free heap: %d bytes", ESP.getFreeHeap());
#else
// Arduino entry point
void setup() {
    // Initialize serial communication
    Serial.begin(SERIAL_BAUD);
    delay(100);
    
    // Initialize watchdog timer if enabled
    #if ENABLE_WATCHDOG
    esp_task_wdt_init(WATCHDOG_TIMEOUT_SEC, true);  // Enable panic so ESP32 will reset
    esp_task_wdt_add(NULL);  // Add current task to watchdog
    Serial.printf("[Setup] Watchdog enabled (%d second timeout)\r\n", WATCHDOG_TIMEOUT_SEC);
    #endif
    
    // Initialize error tracking
    consecutive_errors = 0;
    last_error_time = 0;
    
    Serial.println("\n\nESP32 Touchscreen Display Firmware");
    Serial.println("=====================================");
    Serial.printf("Chip model: %s\r\n", ESP.getChipModel());
    Serial.printf("Chip revision: %d\r\n", ESP.getChipRevision());
    Serial.printf("CPU frequency: %d MHz\r\n", ESP.getCpuFreqMHz());
    Serial.printf("Flash size: %d bytes\r\n", ESP.getFlashChipSize());
    Serial.printf("Free heap: %d bytes\r\n", ESP.getFreeHeap());
#endif
    
    // Initialize backlight early so we can see the display
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    
    // Initialize LVGL
    lv_init();
    #ifdef ESP_PLATFORM
        ESP_LOGI(TAG_MAIN, "LVGL initialized");
    #else
        Serial.println("LVGL initialized");
    #endif
    
    // Initialize display driver
    lvgl_display_init();
    
    // Set up LVGL tick timer (1ms)
    #ifdef ESP_PLATFORM
        // ESP-IDF: Use esp_timer for 1ms periodic timer
        esp_timer_create_args_t timer_args = {};
        timer_args.callback = lvgl_tick_handler;
        timer_args.name = "lvgl_tick";
        esp_timer_handle_t lvgl_esp_timer;
        ESP_ERROR_CHECK(esp_timer_create(&timer_args, &lvgl_esp_timer));
        ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_esp_timer, 1000));  // 1000us = 1ms
        // Store in hw_timer_t structure for compatibility
        lvgl_timer = (hw_timer_t*)malloc(sizeof(hw_timer_t));
        if (lvgl_timer) {
            lvgl_timer->timer_handle = lvgl_esp_timer;
            lvgl_timer->callback = NULL;  // Not used with esp_timer
        }
    #else
        // Arduino: Use hardware timer
        lvgl_timer = timerBegin(0, 80, true);  // 80MHz / 80 = 1MHz, 1 tick = 1us
        timerAttachInterrupt(lvgl_timer, &lvgl_tick_handler, true);
        timerAlarmWrite(lvgl_timer, 1000, true);  // 1000us = 1ms
        timerAlarmEnable(lvgl_timer);
    #endif
    
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
    #ifdef ESP_PLATFORM
        ESP_LOGI(TAG_MAIN, "[Setup] Setting progress to 90%...");
    #else
        Serial.println("[Setup] Setting progress to 90%...");
        Serial.flush();
    #endif
    splashscreen_set_progress(90);
    splashscreen_set_status("Loading UI...");
    #ifdef ESP_PLATFORM
        ESP_LOGI(TAG_MAIN, "[Setup] Progress set to 90%");
    #else
        Serial.println("[Setup] Progress set to 90%");
        Serial.flush();
    #endif
    delay(200);
    
    // Set background to pure black (RGB 0,0,0) BEFORE removing splashscreen
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);
    lv_timer_handler();
    delay(5);
    
    // Remove splashscreen FIRST, then initialize UI
    // (UI init clears the screen, so we need to remove splashscreen first)
    #ifdef ESP_PLATFORM
        ESP_LOGI(TAG_MAIN, "[Setup] Removing splashscreen before UI init...");
    #else
        Serial.println("[Setup] Removing splashscreen before UI init...");
        Serial.flush();
    #endif
    splashscreen_remove();
    #ifdef ESP_PLATFORM
        ESP_LOGI(TAG_MAIN, "[Setup] Splashscreen removed");
    #else
        Serial.println("[Setup] Splashscreen removed");
        Serial.flush();
    #endif
    
    // Ensure background stays pure black (RGB 0,0,0) during transition
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);
    lv_timer_handler();
    delay(5);
    
    // Now initialize main UI (this will create the UI on clean screen)
    #ifdef ESP_PLATFORM
        ESP_LOGI(TAG_MAIN, "\n[Setup] About to initialize UI...");
    #else
        Serial.println("\n[Setup] About to initialize UI...");
    #endif
    #if TEST_MODE
        #ifdef ESP_PLATFORM
            ESP_LOGI(TAG_MAIN, "[Setup] Initializing test mode UI...");
            test_mode_init();
            ESP_LOGI(TAG_MAIN, "[Setup] Test mode UI initialized - DONE");
        #else
            Serial.println("[Setup] Initializing test mode UI...");
            test_mode_init();
            Serial.println("[Setup] Test mode UI initialized - DONE");
        #endif
    #else
        #ifdef ESP_PLATFORM
            ESP_LOGI(TAG_MAIN, "[Setup] Initializing production mode UI...");
            production_mode_init();
            ESP_LOGI(TAG_MAIN, "[Setup] Production mode UI initialized - DONE");
        #else
            Serial.println("[Setup] Initializing production mode UI...");
            production_mode_init();
            Serial.println("[Setup] Production mode UI initialized - DONE");
        #endif
        
        // Set up callback for pouring screen to switch back to production screen
        pouring_mode_set_screen_switch_callback(switch_to_production_screen);
    #endif
    #ifndef ESP_PLATFORM
        Serial.flush();
    #endif
    
    // Finalize (100% - just for logging, splashscreen is already gone)
    #ifdef ESP_PLATFORM
        ESP_LOGI(TAG_MAIN, "[Setup] Setup sequence complete!");
        ESP_LOGI(TAG_MAIN, "\n========================================");
        ESP_LOGI(TAG_MAIN, "SETUP COMPLETE!");
        ESP_LOGI(TAG_MAIN, "========================================");
    #else
        Serial.println("[Setup] Setup sequence complete!");
        Serial.flush();
        Serial.println("\n========================================");
        Serial.println("SETUP COMPLETE!");
        Serial.println("========================================");
        Serial.flush();
    #endif
    
    // Initialize WiFi connection
    #ifdef ESP_PLATFORM
        ESP_LOGI(TAG_MAIN, "\n[Setup] Initializing WiFi...");
    #else
        Serial.println("\n[Setup] Initializing WiFi...");
    #endif
    bool wifi_ok = wifi_manager_init();
    if (wifi_ok) {
        #ifdef ESP_PLATFORM
            ESP_LOGI(TAG_MAIN, "[Setup] WiFi initialized successfully");
        #else
            Serial.println("[Setup] WiFi initialized successfully");
        #endif
        
        // Wait a bit for DNS to be ready after WiFi connection
        #ifdef ESP_PLATFORM
            ESP_LOGI(TAG_MAIN, "[Setup] Waiting for network stack to be ready...");
        #else
            Serial.println("[Setup] Waiting for network stack to be ready...");
        #endif
        delay(2000);  // 2 second delay for DNS/DHCP to stabilize
        
        // Verify WiFi is still connected
        if (!wifi_manager_is_connected()) {
            #ifdef ESP_PLATFORM
                ESP_LOGW(TAG_MAIN, "[Setup] WiFi disconnected after delay, will retry MQTT in loop");
            #else
                Serial.println("[Setup] WiFi disconnected after delay, will retry MQTT in loop");
            #endif
            wifi_ok = false;
        }
    } else {
        #ifdef ESP_PLATFORM
            ESP_LOGW(TAG_MAIN, "[Setup] WiFi initialization failed, will retry in loop");
        #else
            Serial.println("[Setup] WiFi initialization failed, will retry in loop");
        #endif
    }
    
    // Get chip ID for MQTT using SOC UID (unique chip identifier)
    char chip_id[17] = {0};  // 16 hex chars + null terminator for 64-bit SOC UID
    if (get_soc_uid_string(chip_id, sizeof(chip_id))) {
        #ifdef ESP_PLATFORM
            ESP_LOGI(TAG_MAIN, "[Setup] SOC UID: %s", chip_id);
        #else
            Serial.printf("[Setup] SOC UID: %s\r\n", chip_id);
        #endif
    } else {
        #ifdef ESP_PLATFORM
            ESP_LOGE(TAG_MAIN, "[Setup] Failed to get SOC UID");
        #else
            Serial.println("[Setup] Failed to get SOC UID");
        #endif
        chip_id[0] = '\0';  // Ensure it's empty on failure
    }
    
    // Initialize MQTT client (only if WiFi is connected and stable)
    if (wifi_ok && wifi_manager_is_connected() && strlen(chip_id) > 0) {
        #ifdef ESP_PLATFORM
            ESP_LOGI(TAG_MAIN, "\n[Setup] Initializing MQTT...");
        #else
            Serial.println("\n[Setup] Initializing MQTT...");
        #endif
        #if !TEST_MODE
            // Set MQTT message callback for screen switching
            mqtt_client_set_callback(on_mqtt_message);
        #endif
        bool mqtt_ok = mqtt_client_init(chip_id);
        if (mqtt_ok) {
            #ifdef ESP_PLATFORM
                ESP_LOGI(TAG_MAIN, "[Setup] MQTT initialized successfully");
            #else
                Serial.println("[Setup] MQTT initialized successfully");
            #endif
        } else {
            #ifdef ESP_PLATFORM
                ESP_LOGW(TAG_MAIN, "[Setup] MQTT initialization failed, will retry in loop");
            #else
                Serial.println("[Setup] MQTT initialization failed, will retry in loop");
            #endif
        }
    } else {
        #ifdef ESP_PLATFORM
            ESP_LOGW(TAG_MAIN, "[Setup] Skipping MQTT initialization (WiFi not connected or chip ID unavailable)");
        #else
            Serial.println("[Setup] Skipping MQTT initialization (WiFi not connected or chip ID unavailable)");
        #endif
    }
    #if TEST_MODE
        #ifdef ESP_PLATFORM
            ESP_LOGI(TAG_MAIN, "Running in TEST MODE");
        #else
            Serial.println("Running in TEST MODE");
        #endif
    #else
        #ifdef ESP_PLATFORM
            ESP_LOGI(TAG_MAIN, "Running in PRODUCTION MODE");
        #else
            Serial.println("Running in PRODUCTION MODE");
        #endif
    #endif
    #ifdef ESP_PLATFORM
        ESP_LOGI(TAG_MAIN, "Free heap after setup: %d bytes", ESP.getFreeHeap());
    #else
        Serial.printf("Free heap after setup: %d bytes\r\n", ESP.getFreeHeap());
    #endif

// Forward declaration for ESP-IDF
#ifdef ESP_PLATFORM
void loop_body();

    // ESP-IDF: Create main loop task instead of using loop()
    TaskHandle_t main_loop_task_handle = NULL;
    xTaskCreate([](void* param) {
        #if ENABLE_WATCHDOG
        // Add this task to the watchdog (must be done from within the task)
        esp_task_wdt_add(NULL);
        ESP_LOGI(TAG_MAIN, "[Main Loop] Task added to watchdog");
        #endif
        while (1) {
            loop_body();
            // Small delay to prevent CPU spinning, but short enough for watchdog
            vTaskDelay(pdMS_TO_TICKS(10));  // Increased from 5ms to 10ms for stability
        }
    }, "main_loop", 8192, NULL, 5, &main_loop_task_handle);
    
    #if ENABLE_WATCHDOG
    // Wait a bit for the task to start and register itself
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG_MAIN, "[Setup] Main loop task created, watchdog should be active");
    #endif
#endif
}

// Main loop body (extracted for ESP-IDF task)
#ifdef ESP_PLATFORM
void loop_body() {
#else
void loop() {
#endif
    // Feed watchdog timer at the start of each loop iteration (defensive)
    #if ENABLE_WATCHDOG
    esp_task_wdt_reset();
    #endif
    
    // Handle LVGL tasks (should be called every few milliseconds)
    lv_timer_handler();
    
    // Feed watchdog after LVGL (in case LVGL operations take time)
    #if ENABLE_WATCHDOG
    esp_task_wdt_reset();
    #endif
    
    // WiFi connection maintenance
    wifi_manager_loop();
    
    // Feed watchdog after WiFi operations (in case they take time)
    #if ENABLE_WATCHDOG
    esp_task_wdt_reset();
    #endif
    
    // MQTT connection maintenance (only if WiFi is connected)
    if (wifi_manager_is_connected()) {
        mqtt_client_loop();
    }
    
    // Feed watchdog after MQTT operations
    #if ENABLE_WATCHDOG
    esp_task_wdt_reset();
    #endif
    
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
    
    // Feed watchdog timer at the end of loop
    #if ENABLE_WATCHDOG
    esp_task_wdt_reset();
    #endif
    
    // Check for persistent errors and reset if necessary
    if (consecutive_errors >= MAX_CONSECUTIVE_ERRORS) {
        unsigned long time_since_error = millis() - last_error_time;
        if (time_since_error > ERROR_RESET_DELAY_MS) {
            #ifdef ESP_PLATFORM
                ESP_LOGE(TAG_MAIN, "[Error] Too many consecutive errors (%d), resetting in %d ms...", 
                         consecutive_errors, ERROR_RESET_DELAY_MS);
                // Use vTaskDelay instead of delay() to allow watchdog feeding
                // Break the delay into smaller chunks and feed watchdog
                int delay_ms = ERROR_RESET_DELAY_MS;
                while (delay_ms > 0) {
                    int chunk = (delay_ms > 1000) ? 1000 : delay_ms;
                    vTaskDelay(pdMS_TO_TICKS(chunk));
                    #if ENABLE_WATCHDOG
                    esp_task_wdt_reset();
                    #endif
                    delay_ms -= chunk;
                }
            #else
                Serial.printf("[Error] Too many consecutive errors (%d), resetting in %d ms...\r\n", 
                             consecutive_errors, ERROR_RESET_DELAY_MS);
                Serial.flush();
                delay(ERROR_RESET_DELAY_MS);
            #endif
            ESP.restart();
        }
    } else if (consecutive_errors > 0 && (millis() - last_error_time) > 60000) {
        // Reset error counter after 60 seconds of no errors
        consecutive_errors = 0;
        #ifdef ESP_PLATFORM
            ESP_LOGI(TAG_MAIN, "[Error] Error counter reset (60s without errors)");
        #else
            Serial.println("[Error] Error counter reset (60s without errors)");
        #endif
    }
    
    // Small delay to prevent watchdog issues (handled by task delay in ESP-IDF)
    #ifndef ESP_PLATFORM
        delay(5);
    #endif
}
