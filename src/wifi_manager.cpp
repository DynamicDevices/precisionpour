/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * WiFi Manager Implementation
 * 
 * Handles WiFi connection and automatic reconnection
 * Supports Improv WiFi BLE provisioning for credential setup
 */

// Project headers
#include "config.h"
#include "wifi_manager.h"

// System/Standard library headers
#ifdef ESP_PLATFORM
    // ESP-IDF framework headers
    #include <esp_event.h>
    #include <esp_log.h>
    #include <esp_mac.h>
    #include <esp_netif.h>
    #include <esp_system.h>
    #include <esp_wifi.h>
    #include <nvs.h>
    #include <nvs_flash.h>
    #if ENABLE_WATCHDOG
    #include <esp_task_wdt.h>
    #endif
    #include <cstring>
    #include <string>
    #define TAG "wifi"
    
    // Project compatibility headers
    #include "esp_idf_compat.h"
    #include "esp_system_compat.h"
    
    // Third-party library headers (if enabled)
    #if USE_IMPROV_WIFI
        #include <ImprovWiFiBLE.h>
        #include <NimBLEAdvertising.h>
        #include <NimBLEDevice.h>
    #endif
#else
    // Arduino framework headers
    #include <Arduino.h>
    #include <Preferences.h>
    #include <ImprovWiFiBLE.h>
    #include <NimBLEAdvertising.h>
    #include <NimBLEDevice.h>
    
    // ESP32 Arduino headers
    #include "esp_efuse.h"
    #include "esp_system.h"
#endif

// EEPROM/Preferences keys for storing WiFi credentials
#define PREF_NAMESPACE "wifi"
#define PREF_KEY_SSID "ssid"
#define PREF_KEY_PASSWORD "password"
#define PREF_KEY_USE_SAVED "use_saved"

static bool wifi_connected = false;
static unsigned long last_reconnect_attempt = 0;
static bool improv_provisioning_active = false;

#ifdef ESP_PLATFORM
    // ESP-IDF: Use NVS instead of Preferences
    static nvs_handle_t wifi_nvs_handle = 0;
    static esp_netif_t* sta_netif = NULL;
    
    // WiFi event handler
    static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data) {
        if (event_base == WIFI_EVENT) {
            switch (event_id) {
                case WIFI_EVENT_STA_START:
                    ESP_LOGI(TAG, "WiFi station started");
                    break;
                case WIFI_EVENT_STA_CONNECTED: {
                    wifi_event_sta_connected_t* event = (wifi_event_sta_connected_t*)event_data;
                    ESP_LOGI(TAG, "Connected to AP SSID: %s, channel: %d", event->ssid, event->channel);
                    break;
                }
                case WIFI_EVENT_STA_DISCONNECTED: {
                    wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*)event_data;
                    ESP_LOGW(TAG, "Disconnected from AP, reason: %d", event->reason);
                    wifi_connected = false;
                    break;
                }
                default:
                    break;
            }
        } else if (event_base == IP_EVENT) {
            if (event_id == IP_EVENT_STA_GOT_IP) {
                ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
                ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
                wifi_connected = true;
            }
        }
    }
#else
    // Arduino: Use Preferences
    static Preferences preferences;
#endif

// Forward declarations
static bool connect_to_wifi(const String& ssid, const String& password);
#if USE_IMPROV_WIFI
static void save_credentials(const String& ssid, const String& password);
#endif
static bool load_saved_credentials(String& ssid, String& password);

// Improv WiFi BLE instance
#if USE_IMPROV_WIFI
static ImprovWiFiBLE improvWiFiBLE;

// Callback functions for Improv WiFi BLE
static void on_improv_error(ImprovTypes::Error error) {
    #ifdef ESP_PLATFORM
        ESP_LOGE(TAG, "[Improv WiFi BLE] Error: %d", (int)error);
    #else
        Serial.printf("[Improv WiFi BLE] Error: %d\r\n", (int)error);
    #endif
}

static void on_improv_connected(const char* ssid, const char* password) {
    #ifdef ESP_PLATFORM
        ESP_LOGI(TAG, "[Improv WiFi BLE] Received credentials for: %s", ssid);
    #else
        Serial.printf("[Improv WiFi BLE] Received credentials for: %s\r\n", ssid);
    #endif
    
    // Deinitialize BLE first before re-enabling WiFi
    #ifdef ESP_PLATFORM
        if (NimBLEDevice::getInitialized()) {
            ESP_LOGI(TAG, "[Improv WiFi BLE] Deinitializing BLE before connecting to WiFi...");
            NimBLEDevice::deinit(true);
            delay(200);  // Give BLE time to fully shut down
        }
        
        // Re-enable WiFi
        esp_wifi_start();
        delay(100);
    #else
        if (NimBLEDevice::getInitialized()) {
            Serial.println("[Improv WiFi BLE] Deinitializing BLE before connecting to WiFi...");
            NimBLEDevice::deinit(true);
            delay(200);  // Give BLE time to fully shut down
        }
        
        // Re-enable WiFi
        WiFi.mode(WIFI_STA);
        delay(100);
    #endif
    
    // Try to connect with new credentials
    if (connect_to_wifi(String(ssid), String(password))) {
        // Save credentials for future use
        save_credentials(String(ssid), String(password));
        improv_provisioning_active = false;
        #ifdef ESP_PLATFORM
            ESP_LOGI(TAG, "[Improv WiFi BLE] Provisioning successful!");
            ESP_LOGI(TAG, "[Improv WiFi BLE] Credentials saved, restarting device...");
        #else
            Serial.println("[Improv WiFi BLE] Provisioning successful!");
            Serial.println("[Improv WiFi BLE] Credentials saved, restarting device...");
        #endif
        delay(1000);  // Give time for serial output to flush
        ESP.restart();  // Restart the device after successful provisioning
    } else {
        #ifdef ESP_PLATFORM
            ESP_LOGE(TAG, "[Improv WiFi BLE] Failed to connect with provided credentials");
            ESP_LOGI(TAG, "[Improv WiFi BLE] Restarting BLE provisioning...");
        #else
            Serial.println("[Improv WiFi BLE] Failed to connect with provided credentials");
            Serial.println("[Improv WiFi BLE] Restarting BLE provisioning...");
        #endif
        delay(1000);
        wifi_manager_start_provisioning();
    }
}
#endif

/**
 * Load saved WiFi credentials from NVS/Preferences
 */
static bool load_saved_credentials(String& ssid, String& password) {
    if (!USE_SAVED_CREDENTIALS) {
        return false;
    }
    
    #ifdef ESP_PLATFORM
        // ESP-IDF: Use NVS
        esp_err_t err = nvs_open(PREF_NAMESPACE, NVS_READONLY, &wifi_nvs_handle);
        if (err != ESP_OK) {
            return false;  // Namespace doesn't exist yet
        }
        
        bool use_saved = false;
        size_t required_size = sizeof(bool);
        err = nvs_get_blob(wifi_nvs_handle, PREF_KEY_USE_SAVED, &use_saved, &required_size);
        if (err != ESP_OK || !use_saved) {
            nvs_close(wifi_nvs_handle);
            return false;
        }
        
        // Read SSID
        required_size = 64;
        char ssid_buf[64] = {0};
        err = nvs_get_str(wifi_nvs_handle, PREF_KEY_SSID, ssid_buf, &required_size);
        if (err == ESP_OK && strlen(ssid_buf) > 0) {
            ssid = String(ssid_buf);
            
            // Read password
            required_size = 64;
            char password_buf[64] = {0};
            err = nvs_get_str(wifi_nvs_handle, PREF_KEY_PASSWORD, password_buf, &required_size);
            if (err == ESP_OK) {
                password = String(password_buf);
            }
            
            nvs_close(wifi_nvs_handle);
            ESP_LOGI(TAG, "[WiFi] Loaded saved credentials for: %s", ssid.c_str());
            return true;
        }
        
        nvs_close(wifi_nvs_handle);
        return false;
    #else
        // Arduino: Use Preferences
        if (!preferences.begin(PREF_NAMESPACE, true)) {
            return false;
        }
        
        bool use_saved = preferences.getBool(PREF_KEY_USE_SAVED, false);
        
        if (use_saved) {
            ssid = preferences.getString(PREF_KEY_SSID, "");
            password = preferences.getString(PREF_KEY_PASSWORD, "");
            preferences.end();
            
            if (ssid.length() > 0) {
                Serial.printf("[WiFi] Loaded saved credentials for: %s\r\n", ssid.c_str());
                return true;
            }
        }
        
        preferences.end();
        return false;
    #endif
}

/**
 * Save WiFi credentials to NVS/Preferences
 */
#if USE_IMPROV_WIFI
static void save_credentials(const String& ssid, const String& password) {
    #ifdef ESP_PLATFORM
        // ESP-IDF: Use NVS
        esp_err_t err = nvs_open(PREF_NAMESPACE, NVS_READWRITE, &wifi_nvs_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "[WiFi] ERROR: Failed to open NVS namespace for writing");
            return;
        }
        
        err = nvs_set_str(wifi_nvs_handle, PREF_KEY_SSID, ssid.c_str());
        if (err == ESP_OK) {
            err = nvs_set_str(wifi_nvs_handle, PREF_KEY_PASSWORD, password.c_str());
        }
        if (err == ESP_OK) {
            bool use_saved = true;
            err = nvs_set_blob(wifi_nvs_handle, PREF_KEY_USE_SAVED, &use_saved, sizeof(bool));
        }
        if (err == ESP_OK) {
            err = nvs_commit(wifi_nvs_handle);
        }
        
        nvs_close(wifi_nvs_handle);
        
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "[WiFi] Saved credentials for: %s", ssid.c_str());
        } else {
            ESP_LOGE(TAG, "[WiFi] Failed to save credentials: %s", esp_err_to_name(err));
        }
    #else
        // Arduino: Use Preferences
        if (!preferences.begin(PREF_NAMESPACE, false)) {
            Serial.println("[WiFi] ERROR: Failed to open Preferences namespace for writing");
            return;
        }
        
        preferences.putString(PREF_KEY_SSID, ssid);
        preferences.putString(PREF_KEY_PASSWORD, password);
        preferences.putBool(PREF_KEY_USE_SAVED, true);
        preferences.end();
        
        Serial.printf("[WiFi] Saved credentials for: %s\r\n", ssid.c_str());
    #endif
}
#endif // USE_IMPROV_WIFI

/**
 * Connect to WiFi network
 */
static bool connect_to_wifi(const String& ssid, const String& password) {
    #ifdef ESP_PLATFORM
        // ESP-IDF: Use esp_wifi APIs
        ESP_LOGI(TAG, "[WiFi] Connecting to: %s", ssid.c_str());
        
        wifi_config_t wifi_config = {};
        strncpy((char*)wifi_config.sta.ssid, ssid.c_str(), sizeof(wifi_config.sta.ssid) - 1);
        strncpy((char*)wifi_config.sta.password, password.c_str(), sizeof(wifi_config.sta.password) - 1);
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
        wifi_config.sta.pmf_cfg.capable = true;
        wifi_config.sta.pmf_cfg.required = false;
        
        esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "[WiFi] Failed to set WiFi configuration: %s", esp_err_to_name(err));
            return false;
        }
        
        err = esp_wifi_connect();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "[WiFi] Failed to start WiFi connection: %s", esp_err_to_name(err));
            return false;
        }
        
        // Wait for connection (with timeout)
        unsigned long start_time = millis();
        unsigned long timeout = 30000; // 30 second timeout
        
        // Set WiFi component log level to INFO
        esp_log_level_set("wifi", ESP_LOG_INFO);
        
        while (!wifi_connected && (millis() - start_time) < timeout) {
            #ifdef ESP_PLATFORM
                vTaskDelay(pdMS_TO_TICKS(500));
                // Note: Don't reset watchdog here - this might be called from app_main
                // before the main loop task is registered with the watchdog
            #else
                delay(500);
            #endif
            // Removed dot printing - too verbose
        }
        
        // WiFi log level remains at INFO
        
        if (wifi_connected) {
            ESP_LOGI(TAG, "[WiFi] Connected!");
            esp_netif_ip_info_t ip_info;
            if (esp_netif_get_ip_info(sta_netif, &ip_info) == ESP_OK) {
                ESP_LOGI(TAG, "[WiFi] IP address: " IPSTR, IP2STR(&ip_info.ip));
            }
            
            uint8_t mac[6];
            if (esp_wifi_get_mac(WIFI_IF_STA, mac) == ESP_OK) {
                ESP_LOGI(TAG, "[WiFi] MAC address: %02X:%02X:%02X:%02X:%02X:%02X",
                         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            }
            
            wifi_ap_record_t ap_info;
            if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
                ESP_LOGI(TAG, "[WiFi] Signal strength (RSSI): %d dBm", ap_info.rssi);
            }
            return true;
        } else {
            wifi_connected = false;
            ESP_LOGE(TAG, "[WiFi] Connection failed!");
            return false;
        }
    #else
        // Arduino: Use WiFi library
        Serial.printf("[WiFi] Connecting to: %s\r\n", ssid.c_str());
        
        WiFi.mode(WIFI_STA);
        WiFi.setSleep(false);
        WiFi.begin(ssid.c_str(), password.c_str());
        
        // Wait for connection (with timeout)
        unsigned long start_time = millis();
        unsigned long timeout = 30000; // 30 second timeout
        
        while (WiFi.status() != WL_CONNECTED && (millis() - start_time) < timeout) {
            delay(500);
            Serial.print(".");
        }
        Serial.println();
        
        if (WiFi.status() == WL_CONNECTED) {
            wifi_connected = true;
            Serial.println("[WiFi] Connected!");
            Serial.printf("[WiFi] IP address: %s\r\n", WiFi.localIP().toString().c_str());
            Serial.printf("[WiFi] MAC address: %s\r\n", WiFi.macAddress().c_str());
            Serial.printf("[WiFi] Signal strength (RSSI): %d dBm\r\n", WiFi.RSSI());
            return true;
        } else {
            wifi_connected = false;
            Serial.println("[WiFi] Connection failed!");
            return false;
        }
    #endif
}

bool wifi_manager_init() {
    #ifdef ESP_PLATFORM
        ESP_LOGI(TAG, "\n=== Initializing WiFi ===");
        
        // Set WiFi component log level to INFO
        esp_log_level_set("wifi", ESP_LOG_INFO);
    #else
        Serial.println("\n=== Initializing WiFi ===");
    #endif
    
    #ifdef ESP_PLATFORM
        // ESP-IDF: Initialize WiFi
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        
        sta_netif = esp_netif_create_default_wifi_sta();
        if (!sta_netif) {
            ESP_LOGE(TAG, "Failed to create WiFi station netif");
            return false;
        }
        
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        
        // Set WiFi component log level to INFO
        esp_log_level_set("wifi", ESP_LOG_INFO);
        
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &wifi_event_handler,
                                                            NULL,
                                                            NULL));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                            IP_EVENT_STA_GOT_IP,
                                                            &wifi_event_handler,
                                                            NULL,
                                                            NULL));
        
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));  // Disable power saving
        ESP_ERROR_CHECK(esp_wifi_start());
    #endif
    
    String ssid, password;
    
    // Try to load saved credentials first
    if (USE_SAVED_CREDENTIALS && load_saved_credentials(ssid, password)) {
        #ifdef ESP_PLATFORM
            ESP_LOGI(TAG, "[WiFi] Using saved credentials");
        #else
            Serial.println("[WiFi] Using saved credentials");
        #endif
    } else {
        // Use credentials from KConfig (ESP-IDF) or secrets.h (Arduino)
        // KConfig values take priority, secrets.h is fallback if empty
        #ifdef ESP_PLATFORM
            // ESP-IDF: Use KConfig values if set (non-empty), otherwise fallback to secrets.h
            // CONFIG_WIFI_SSID is available from sdkconfig.h (included via config.h)
            #ifdef CONFIG_WIFI_SSID
                if (strlen(CONFIG_WIFI_SSID) > 0) {
                    ssid = String(CONFIG_WIFI_SSID);
                    password = String(CONFIG_WIFI_PASSWORD);
                    ESP_LOGI(TAG, "[WiFi] Using KConfig credentials");
                } else {
                    // Fallback to secrets.h if KConfig values are empty
                    ssid = String(WIFI_SSID);  // From secrets.h
                    password = String(WIFI_PASSWORD);  // From secrets.h
                    ESP_LOGI(TAG, "[WiFi] Using secrets.h credentials (KConfig empty)");
                }
            #else
                // CONFIG_WIFI_SSID not defined, use secrets.h
                ssid = String(WIFI_SSID);
                password = String(WIFI_PASSWORD);
                ESP_LOGI(TAG, "[WiFi] Using secrets.h credentials (KConfig not available)");
            #endif
        #else
            // Arduino: Always use secrets.h
            ssid = String(WIFI_SSID);
            password = String(WIFI_PASSWORD);
            Serial.printf("[WiFi] Using credentials from secrets.h\r\n");
            Serial.printf("[WiFi] Connecting to SSID: '%s'\r\n", ssid.c_str());
        #endif
        #ifdef ESP_PLATFORM
            ESP_LOGI(TAG, "[WiFi] Using hardcoded credentials");
            ESP_LOGI(TAG, "[WiFi] Connecting to SSID: '%s'", ssid.c_str());
        #else
            Serial.println("[WiFi] Using hardcoded credentials");
        #endif
    }
    
    // Try to connect with available credentials
    if (connect_to_wifi(ssid, password)) {
        return true;
    }
    
    // Connection failed - start Improv WiFi provisioning if enabled
    #if USE_IMPROV_WIFI
    if (!wifi_connected) {
        #ifdef ESP_PLATFORM
            ESP_LOGW(TAG, "[WiFi] Connection failed - starting Improv WiFi provisioning...");
        #else
            Serial.println("[WiFi] Connection failed - starting Improv WiFi provisioning...");
        #endif
        wifi_manager_start_provisioning();
        return false;  // Not connected yet, provisioning in progress
    }
    #else
    #ifdef ESP_PLATFORM
        ESP_LOGW(TAG, "[WiFi] Connection failed - Improv WiFi disabled, will retry...");
    #else
        Serial.println("[WiFi] Connection failed - Improv WiFi disabled, will retry...");
    #endif
    last_reconnect_attempt = millis();
    return false;
    #endif
    
    return false;
}

void wifi_manager_start_provisioning() {
    #if USE_IMPROV_WIFI
    if (improv_provisioning_active) {
        return;  // Already provisioning
    }
    
    #ifdef ESP_PLATFORM
        ESP_LOGI(TAG, "[Improv WiFi BLE] Starting BLE provisioning...");
    #else
        Serial.println("[Improv WiFi BLE] Starting BLE provisioning...");
    #endif
    
    // Get unique chip ID (MAC address formatted as hex string) BEFORE disabling WiFi
    char chip_id[32] = {0};
    uint8_t mac[6];
    esp_err_t ret = esp_efuse_mac_get_default(mac);
    
    if (ret == ESP_OK) {
        // Format MAC address as hex string without colons (e.g., "AABBCCDDEEFF")
        snprintf(chip_id, sizeof(chip_id), "%02X%02X%02X%02X%02X%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    } else {
        #ifdef ESP_PLATFORM
            // ESP-IDF: Get MAC from WiFi
            if (esp_wifi_get_mac(WIFI_IF_STA, mac) == ESP_OK) {
                snprintf(chip_id, sizeof(chip_id), "%02X%02X%02X%02X%02X%02X",
                         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            }
        #else
            // Fallback: use WiFi MAC address (get it before disabling WiFi)
            String wifi_mac = WiFi.macAddress();
            wifi_mac.replace(":", "");
            strncpy(chip_id, wifi_mac.c_str(), sizeof(chip_id) - 1);
        #endif
    }
    
    // Build BLE device name: Use empty string to avoid adding to primary advertisement
    char ble_device_name[64] = {0};
    ble_device_name[0] = '\0';
    
    #ifdef ESP_PLATFORM
        ESP_LOGI(TAG, "[Improv WiFi BLE] Chip ID: %s", chip_id);
        ESP_LOGI(TAG, "[Improv WiFi BLE] BLE device name: %s", ble_device_name);
    #else
        Serial.printf("[Improv WiFi BLE] Chip ID: %s\r\n", chip_id);
        Serial.printf("[Improv WiFi BLE] BLE device name: %s\r\n", ble_device_name);
    #endif
    
    // Disable WiFi before initializing BLE to avoid coexistence conflicts
    #ifdef ESP_PLATFORM
        ESP_LOGI(TAG, "[Improv WiFi BLE] Disabling WiFi for BLE...");
        esp_wifi_stop();
        esp_wifi_deinit();
        delay(100);  // Give WiFi time to fully shut down
    #else
        Serial.println("[Improv WiFi BLE] Disabling WiFi for BLE...");
        WiFi.disconnect(true);  // Disconnect and disable WiFi
        WiFi.mode(WIFI_OFF);     // Turn off WiFi completely
        delay(100);               // Give WiFi time to fully shut down
    #endif
    
    // Initialize BLE with device name including chip ID
    if (!NimBLEDevice::getInitialized()) {
        #ifdef ESP_PLATFORM
            ESP_LOGI(TAG, "[Improv WiFi BLE] Initializing BLE...");
        #else
            Serial.println("[Improv WiFi BLE] Initializing BLE...");
        #endif
        NimBLEDevice::init(ble_device_name);
        #ifdef ESP_PLATFORM
            ESP_LOGI(TAG, "[Improv WiFi BLE] BLE initialized");
        #else
            Serial.println("[Improv WiFi BLE] BLE initialized");
        #endif
    }
    
    // Set device info for Improv WiFi BLE
    const char* firmware_name = "P";
    const char* firmware_version = "1";
    
    #ifdef ESP_PLATFORM
        ESP_LOGI(TAG, "[Improv WiFi BLE] Setting device info...");
    #else
        Serial.printf("[Improv WiFi BLE] Advertising data breakdown:\r\n");
        Serial.printf("  Primary advertisement components:\r\n");
        Serial.printf("    - Flags: 3 bytes (0x06)\r\n");
        Serial.printf("    - 128-bit Service UUID: 18 bytes (Improv protocol requirement)\r\n");
        Serial.printf("    - Service Data (0x4677 + 8-byte payload): 11 bytes\r\n");
        Serial.printf("    Subtotal: 32 bytes (ALREADY OVER 31-byte limit!)\r\n");
    #endif
    
    improvWiFiBLE.setDeviceInfo(
        ImprovTypes::ChipFamily::CF_ESP32,
        firmware_name,
        firmware_version,
        ble_device_name
    );
    
    // Set callbacks
    improvWiFiBLE.onImprovError(on_improv_error);
    improvWiFiBLE.onImprovConnected(on_improv_connected);
    
    improv_provisioning_active = true;
    
    #ifdef ESP_PLATFORM
        ESP_LOGI(TAG, "[Improv WiFi BLE] BLE provisioning active");
        ESP_LOGI(TAG, "[Improv WiFi BLE] Device advertising as '%s'", ble_device_name);
        ESP_LOGI(TAG, "[Improv WiFi BLE] Connect with Improv WiFi mobile app");
    #else
        Serial.println("[Improv WiFi BLE] BLE provisioning active");
        Serial.printf("[Improv WiFi BLE] Device advertising as '%s'\r\n", ble_device_name);
        Serial.println("[Improv WiFi BLE] Connect with Improv WiFi mobile app");
    #endif
    #else
    #ifdef ESP_PLATFORM
        ESP_LOGI(TAG, "[Improv WiFi] Improv WiFi is disabled in config.h");
    #else
        Serial.println("[Improv WiFi] Improv WiFi is disabled in config.h");
    #endif
    #endif
}

bool wifi_manager_is_provisioning() {
    return improv_provisioning_active;
}

bool wifi_manager_is_connected() {
    #ifdef ESP_PLATFORM
        // ESP-IDF: Check WiFi status via event system
        // wifi_connected is updated by event handler
        return wifi_connected;
    #else
        // Arduino: Check actual WiFi status
        if (WiFi.status() == WL_CONNECTED) {
            if (!wifi_connected) {
                // Just reconnected
                wifi_connected = true;
                Serial.println("[WiFi] Reconnected!");
                Serial.printf("[WiFi] IP address: %s\r\n", WiFi.localIP().toString().c_str());
                
                // Stop provisioning if it was active
                if (improv_provisioning_active) {
                    improv_provisioning_active = false;
                    Serial.println("[Improv WiFi] Provisioning stopped - WiFi connected");
                }
            }
            return true;
        } else {
            if (wifi_connected) {
                // Just disconnected
                wifi_connected = false;
                Serial.println("[WiFi] Disconnected!");
            }
            return false;
        }
    #endif
}

void wifi_manager_loop() {
    #ifdef ESP_PLATFORM
        #if ENABLE_WATCHDOG
        // Feed watchdog at start of WiFi loop (defensive)
        esp_task_wdt_reset();
        #endif
    #endif
    
    #if USE_IMPROV_WIFI
    // Handle Improv WiFi BLE provisioning if active
    if (improv_provisioning_active) {
        // Check for timeout
        static unsigned long provisioning_start = 0;
        if (provisioning_start == 0) {
            provisioning_start = millis();
        }
        
        if (millis() - provisioning_start > IMPROV_WIFI_TIMEOUT_MS) {
            #ifdef ESP_PLATFORM
                ESP_LOGW(TAG, "[Improv WiFi BLE] Provisioning timeout (5 minutes) - restarting device");
            #else
                Serial.println("[Improv WiFi BLE] Provisioning timeout (5 minutes) - restarting device");
            #endif
            improv_provisioning_active = false;
            provisioning_start = 0;
            
            // Deinitialize BLE first
            if (NimBLEDevice::getInitialized()) {
                #ifdef ESP_PLATFORM
                    ESP_LOGI(TAG, "[Improv WiFi BLE] Deinitializing BLE...");
                #else
                    Serial.println("[Improv WiFi BLE] Deinitializing BLE...");
                #endif
                NimBLEDevice::deinit(true);
                delay(200);  // Give BLE time to fully shut down
            }
            
            #ifdef ESP_PLATFORM
                ESP_LOGI(TAG, "[Improv WiFi BLE] Restarting device after timeout...");
            #else
                Serial.println("[Improv WiFi BLE] Restarting device after timeout...");
            #endif
            delay(1000);  // Give time for serial output to flush
            ESP.restart();  // Restart the device after timeout
        }
        
        // If WiFi connected, stop provisioning and re-enable WiFi
        if (wifi_manager_is_connected() && improv_provisioning_active) {
            improv_provisioning_active = false;
            provisioning_start = 0;
            
            // Deinitialize BLE first
            if (NimBLEDevice::getInitialized()) {
                #ifdef ESP_PLATFORM
                    ESP_LOGI(TAG, "[Improv WiFi BLE] Deinitializing BLE...");
                #else
                    Serial.println("[Improv WiFi BLE] Deinitializing BLE...");
                #endif
                NimBLEDevice::deinit(true);
                delay(200);  // Give BLE time to fully shut down
            }
            
            #ifdef ESP_PLATFORM
                // Re-enable WiFi (it should already be connected, but ensure it's enabled)
                esp_wifi_start();
                ESP_LOGI(TAG, "[Improv WiFi BLE] Provisioning stopped - WiFi connected, BLE deinitialized");
            #else
                // Re-enable WiFi (it should already be connected, but ensure it's enabled)
                WiFi.mode(WIFI_STA);
                Serial.println("[Improv WiFi BLE] Provisioning stopped - WiFi connected, BLE deinitialized");
            #endif
        }
        
        return;  // Don't try to reconnect while provisioning
    }
    #endif
    
    // Check connection status
    if (!wifi_manager_is_connected()) {
        // Try to reconnect if enough time has passed
        unsigned long now = millis();
        if (now - last_reconnect_attempt >= WIFI_RECONNECT_DELAY) {
            last_reconnect_attempt = now;
            #ifdef ESP_PLATFORM
                ESP_LOGI(TAG, "[WiFi] Attempting reconnection...");
            #else
                Serial.println("[WiFi] Attempting reconnection...");
            #endif
            
            String ssid, password;
            
            // Try saved credentials first
            if (USE_SAVED_CREDENTIALS && load_saved_credentials(ssid, password)) {
                #ifdef ESP_PLATFORM
                    ESP_LOGI(TAG, "[WiFi] Reconnecting to SSID: '%s' (from saved credentials)", ssid.c_str());
                #else
                    Serial.printf("[WiFi] Reconnecting to SSID: '%s' (from saved credentials)\r\n", ssid.c_str());
                #endif
            } else {
                // Use credentials from KConfig (ESP-IDF) or secrets.h (Arduino)
                #ifdef ESP_PLATFORM
                    // ESP-IDF: Use KConfig values if set, otherwise fallback to secrets.h
                    #ifdef CONFIG_WIFI_SSID
                        if (strlen(CONFIG_WIFI_SSID) > 0) {
                            ssid = String(CONFIG_WIFI_SSID);
                            password = String(CONFIG_WIFI_PASSWORD);
                        } else {
                            // Fallback to secrets.h if KConfig values are empty
                            ssid = String(WIFI_SSID);  // From secrets.h
                            password = String(WIFI_PASSWORD);  // From secrets.h
                        }
                    #else
                        // CONFIG_WIFI_SSID not defined, use secrets.h
                        ssid = String(WIFI_SSID);
                        password = String(WIFI_PASSWORD);
                    #endif
                    ESP_LOGI(TAG, "[WiFi] Reconnecting to SSID: '%s'", ssid.c_str());
                #else
                    // Arduino: Always use secrets.h
                    ssid = String(WIFI_SSID);
                    password = String(WIFI_PASSWORD);
                    Serial.printf("[WiFi] Reconnecting to SSID: '%s'\r\n", ssid.c_str());
                #endif
            }
            
            #ifdef ESP_PLATFORM
                esp_wifi_disconnect();
                delay(100);
            #else
                WiFi.disconnect();
                delay(100);
            #endif
            
            // Try to reconnect
            if (connect_to_wifi(ssid, password)) {
                #ifdef ESP_PLATFORM
                    ESP_LOGI(TAG, "[WiFi] Reconnected!");
                    esp_netif_ip_info_t ip_info;
                    if (esp_netif_get_ip_info(sta_netif, &ip_info) == ESP_OK) {
                        ESP_LOGI(TAG, "[WiFi] IP address: " IPSTR, IP2STR(&ip_info.ip));
                    }
                #else
                    wifi_connected = true;
                    Serial.println("[WiFi] Reconnected!");
                    Serial.printf("[WiFi] IP address: %s\r\n", WiFi.localIP().toString().c_str());
                #endif
            } else {
                #ifdef ESP_PLATFORM
                    ESP_LOGW(TAG, "[WiFi] Reconnection failed, will try again...");
                #else
                    Serial.println("[WiFi] Reconnection failed, will try again...");
                #endif
                
                // If reconnection fails and Improv WiFi is enabled, start provisioning
                #if USE_IMPROV_WIFI
                if (!improv_provisioning_active && (now - last_reconnect_attempt) > 60000) {
                    // Only start provisioning after 1 minute of failed reconnections
                    #ifdef ESP_PLATFORM
                        ESP_LOGI(TAG, "[WiFi] Starting Improv WiFi provisioning after failed reconnection...");
                    #else
                        Serial.println("[WiFi] Starting Improv WiFi provisioning after failed reconnection...");
                    #endif
                    wifi_manager_start_provisioning();
                }
                #endif
            }
        }
    }
}

String wifi_manager_get_ip() {
    #ifdef ESP_PLATFORM
        if (wifi_manager_is_connected()) {
            esp_netif_ip_info_t ip_info;
            if (esp_netif_get_ip_info(sta_netif, &ip_info) == ESP_OK) {
                char ip_str[16];
                snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip_info.ip));
                return String(ip_str);
            }
        }
        return String("Not connected");
    #else
        if (wifi_manager_is_connected()) {
            return WiFi.localIP().toString();
        }
        return "Not connected";
    #endif
}

String wifi_manager_get_mac_address() {
    #ifdef ESP_PLATFORM
        uint8_t mac[6];
        if (esp_wifi_get_mac(WIFI_IF_STA, mac) == ESP_OK) {
            char mac_str[18];
            snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            return String(mac_str);
        }
        return String("Unknown");
    #else
        return WiFi.macAddress();
    #endif
}

int wifi_manager_get_rssi() {
    #ifdef ESP_PLATFORM
        if (wifi_manager_is_connected()) {
            wifi_ap_record_t ap_info;
            if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
                return ap_info.rssi;
            }
        }
        return -100;  // Return very weak signal when not connected
    #else
        if (wifi_manager_is_connected()) {
            return WiFi.RSSI();
        }
        return -100;  // Return very weak signal when not connected
    #endif
}
