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
#include "wifi/wifi_manager.h"
#include "wifi/wifi_credentials.h"
#include "wifi/wifi_improv.h"

// System/Standard library headers
// ESP-IDF framework headers
#include <esp_event.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <esp_netif.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_sntp.h>
#include <nvs_flash.h>
#if ENABLE_WATCHDOG
#include <esp_task_wdt.h>
#endif
#include <time.h>
#include <cstring>
#include <string>
#define TAG "wifi"

// Project compatibility headers
#include "system/esp_idf_compat.h"
#include "system/esp_system_compat.h"

// Third-party library headers (if enabled)
#if USE_IMPROV_WIFI
    #include <ImprovWiFiBLE.h>
    #include <NimBLEAdvertising.h>
    #include <NimBLEDevice.h>
#endif

static bool wifi_connected = false;
static unsigned long last_reconnect_attempt = 0;
static esp_netif_t* sta_netif = NULL;

// WiFi activity tracking (TCP/IP traffic)
static unsigned long last_activity_time = 0;
static const unsigned long ACTIVITY_TIMEOUT_MS = 500;  // Activity visible for 500ms after last change

// NTP time synchronization
static bool ntp_initialized = false;

static void initialize_ntp(void) {
    if (ntp_initialized) {
        return;  // Already initialized
    }
    
    ESP_LOGI(TAG, "[NTP] Initializing NTP time synchronization...");
    
    // Configure SNTP operating mode
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    
    // Set NTP servers (use pool.ntp.org and regional servers)
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_setservername(1, "time.nist.gov");
    esp_sntp_setservername(2, "time.google.com");
    
    // Set timezone (GMT/BST for UK - adjust as needed)
    // GMT+0 in winter, GMT+1 in summer (BST)
    setenv("TZ", "GMT0BST,M3.5.0/1,M10.5.0", 1);
    tzset();
    
    // Initialize SNTP
    esp_sntp_init();
    
    ntp_initialized = true;
    ESP_LOGI(TAG, "[NTP] NTP initialized, waiting for time sync...");
    
    // Wait for time to be set (with timeout)
    int retry = 0;
    const int retry_count = 10;
    while (esp_sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && retry < retry_count) {
        ESP_LOGI(TAG, "[NTP] Waiting for time sync... (%d/%d)", retry + 1, retry_count);
        delay(1000);
        retry++;
    }
    
    if (esp_sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
        time_t now = 0;
        struct tm timeinfo;
        memset(&timeinfo, 0, sizeof(struct tm));
        time(&now);
        localtime_r(&now, &timeinfo);
        ESP_LOGI(TAG, "[NTP] Time synchronized: %04d-%02d-%02d %02d:%02d:%02d",
                 timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        ESP_LOGI(TAG, "[NTP] Date/time will now appear in log messages");
    } else {
        ESP_LOGW(TAG, "[NTP] Time synchronization not completed yet (will sync in background)");
    }
}

// IP event callback to track TCP/IP activity
static void ip_event_activity_handler(void* arg, esp_event_base_t event_base,
                                      int32_t event_id, void* event_data) {
    // Track activity when IP events occur (indicates TCP/IP traffic)
    if (event_base == IP_EVENT) {
        // Any IP event indicates network activity
        last_activity_time = millis();
    }
}

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
                // Reset NTP initialization flag on disconnect
                ntp_initialized = false;
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
            
            // Initialize NTP time synchronization when WiFi connects
            initialize_ntp();
            
            // Log current date/time after NTP sync (if available)
            time_t now = 0;
            struct tm timeinfo;
        memset(&timeinfo, 0, sizeof(struct tm));
            time(&now);
            if (now > 0 && localtime_r(&now, &timeinfo) != NULL) {
                ESP_LOGI(TAG, "Current date/time: %04d-%02d-%02d %02d:%02d:%02d",
                         timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                         timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            }
        }
    }
}

// Forward declarations
static bool connect_to_wifi(const String& ssid, const String& password);

/**
 * Connect to WiFi network (public function for use by wifi_improv)
 */
bool wifi_manager_connect(const String& ssid, const String& password) {
    return connect_to_wifi(ssid, password);
}

/**
 * Connect to WiFi network (internal implementation)
 */
static bool connect_to_wifi(const String& ssid, const String& password) {
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
        vTaskDelay(pdMS_TO_TICKS(500));
        // Note: Don't reset watchdog here - this might be called from app_main
        // before the main loop task is registered with the watchdog
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
}

bool wifi_manager_init() {
    ESP_LOGI(TAG, "\n=== Initializing WiFi ===");
    
    // Set WiFi component log level to INFO
    esp_log_level_set("wifi", ESP_LOG_INFO);
    
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
    
    // Register IP event handler for TCP/IP activity tracking
    // This will track all IP events (not just GOT_IP) to detect TCP/IP traffic
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &ip_event_activity_handler,
                                                        NULL,
                                                        NULL));
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));  // Disable power saving
    ESP_ERROR_CHECK(esp_wifi_start());
    
    String ssid, password;
    
    // Try to load saved credentials first
    if (USE_SAVED_CREDENTIALS && wifi_credentials_load(ssid, password)) {
        ESP_LOGI(TAG, "[WiFi] Using saved credentials");
    } else {
        // Use credentials from KConfig (ESP-IDF) or secrets.h
        // KConfig values take priority, secrets.h is fallback if empty
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
        ESP_LOGI(TAG, "[WiFi] Using hardcoded credentials");
        ESP_LOGI(TAG, "[WiFi] Connecting to SSID: '%s'", ssid.c_str());
    }
    
    // Try to connect with available credentials
    if (connect_to_wifi(ssid, password)) {
        return true;
    }
    
    // Connection failed - start Improv WiFi provisioning if enabled
    #if USE_IMPROV_WIFI
    if (!wifi_connected) {
        ESP_LOGW(TAG, "[WiFi] Connection failed - starting Improv WiFi provisioning...");
        wifi_manager_start_provisioning();
        return false;  // Not connected yet, provisioning in progress
    }
    #else
    ESP_LOGW(TAG, "[WiFi] Connection failed - Improv WiFi disabled, will retry...");
    last_reconnect_attempt = millis();
    return false;
    #endif
    
    return false;
}

void wifi_manager_start_provisioning() {
    wifi_improv_start_provisioning();
}

bool wifi_manager_is_provisioning() {
    return wifi_improv_is_provisioning();
}

bool wifi_manager_is_connected() {
    // ESP-IDF: Check WiFi status via event system
    // wifi_connected is updated by event handler
    return wifi_connected;
}

void wifi_manager_loop() {
    #if ENABLE_WATCHDOG
    // Feed watchdog at start of WiFi loop (defensive)
    esp_task_wdt_reset();
    #endif
    
    // Handle Improv WiFi BLE provisioning if active
    wifi_improv_loop();
    
    // Don't try to reconnect while provisioning
    if (wifi_improv_is_provisioning()) {
        return;
    }
    
    // Check connection status
    if (!wifi_manager_is_connected()) {
        // Try to reconnect if enough time has passed
        unsigned long now = millis();
        if (now - last_reconnect_attempt >= WIFI_RECONNECT_DELAY) {
            last_reconnect_attempt = now;
            ESP_LOGI(TAG, "[WiFi] Attempting reconnection...");
            
            String ssid, password;
            
            // Try saved credentials first
            if (USE_SAVED_CREDENTIALS && wifi_credentials_load(ssid, password)) {
                ESP_LOGI(TAG, "[WiFi] Reconnecting to SSID: '%s' (from saved credentials)", ssid.c_str());
            } else {
                // Use credentials from KConfig (ESP-IDF) or secrets.h
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
            }
            
            esp_wifi_disconnect();
            delay(100);
            
            // Try to reconnect
            if (connect_to_wifi(ssid, password)) {
                ESP_LOGI(TAG, "[WiFi] Reconnected!");
                esp_netif_ip_info_t ip_info;
                if (esp_netif_get_ip_info(sta_netif, &ip_info) == ESP_OK) {
                    ESP_LOGI(TAG, "[WiFi] IP address: " IPSTR, IP2STR(&ip_info.ip));
                }
            } else {
                ESP_LOGW(TAG, "[WiFi] Reconnection failed, will try again...");
                
                // If reconnection fails and Improv WiFi is enabled, start provisioning
                #if USE_IMPROV_WIFI
                if (!wifi_improv_is_provisioning() && (now - last_reconnect_attempt) > 60000) {
                    // Only start provisioning after 1 minute of failed reconnections
                    ESP_LOGI(TAG, "[WiFi] Starting Improv WiFi provisioning after failed reconnection...");
                    wifi_manager_start_provisioning();
                }
                #endif
            }
        }
    }
}

String wifi_manager_get_ip() {
    if (wifi_manager_is_connected()) {
        esp_netif_ip_info_t ip_info;
        if (esp_netif_get_ip_info(sta_netif, &ip_info) == ESP_OK) {
            char ip_str[16];
            snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip_info.ip));
            return String(ip_str);
        }
    }
    return String("Not connected");
}

String wifi_manager_get_mac_address() {
    uint8_t mac[6];
    if (esp_wifi_get_mac(WIFI_IF_STA, mac) == ESP_OK) {
        char mac_str[18];
        snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return String(mac_str);
    }
    return String("Unknown");
}

int wifi_manager_get_rssi() {
    if (wifi_manager_is_connected()) {
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            return ap_info.rssi;
        }
    }
    return -100;  // Return very weak signal when not connected
}

bool wifi_manager_has_activity() {
    if (!wifi_manager_is_connected()) {
        return false;
    }
    
    unsigned long now = millis();
    
    // Check if activity was detected recently (within timeout window)
    // The activity is tracked via IP event callbacks which fire on TCP/IP traffic
    if (last_activity_time > 0 && (now - last_activity_time) < ACTIVITY_TIMEOUT_MS) {
        return true;
    }
    
    return false;
}
