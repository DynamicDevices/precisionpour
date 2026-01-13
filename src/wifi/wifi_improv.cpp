/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * WiFi Improv Provisioning Implementation
 * 
 * Handles Improv WiFi BLE provisioning for credential setup
 */

#include "config.h"
#include "wifi/wifi_improv.h"
#include "wifi/wifi_credentials.h"

// System/Standard library headers
#include <esp_log.h>
#include <esp_mac.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cstring>
#define TAG "wifi_improv"

// Third-party library headers (if enabled)
#if USE_IMPROV_WIFI
    #include <ImprovWiFiBLE.h>
    #include <NimBLEAdvertising.h>
    #include <NimBLEDevice.h>
#endif

// Forward declaration - callback to connect to WiFi (provided by wifi_manager)
extern bool wifi_manager_connect(const String& ssid, const String& password);

static bool improv_provisioning_active = false;

// Improv WiFi BLE instance
#if USE_IMPROV_WIFI
static ImprovWiFiBLE improvWiFiBLE;

// Callback functions for Improv WiFi BLE
static void on_improv_error(ImprovTypes::Error error) {
    ESP_LOGE(TAG, "[Improv WiFi BLE] Error: %d", (int)error);
}

static void on_improv_connected(const char* ssid, const char* password) {
    ESP_LOGI(TAG, "[Improv WiFi BLE] Received credentials for: %s", ssid);
    
    // Deinitialize BLE first before re-enabling WiFi
    if (NimBLEDevice::getInitialized()) {
        ESP_LOGI(TAG, "[Improv WiFi BLE] Deinitializing BLE before connecting to WiFi...");
        NimBLEDevice::deinit(true);
        vTaskDelay(pdMS_TO_TICKS(200));  // Give BLE time to fully shut down
    }
    
    // Re-enable WiFi
    esp_wifi_start();
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Try to connect with new credentials
    if (wifi_manager_connect(String(ssid), String(password))) {
        // Save credentials for future use
        wifi_credentials_save(String(ssid), String(password));
        improv_provisioning_active = false;
        ESP_LOGI(TAG, "[Improv WiFi BLE] Provisioning successful!");
        ESP_LOGI(TAG, "[Improv WiFi BLE] Credentials saved, restarting device...");
        vTaskDelay(pdMS_TO_TICKS(1000));  // Give time for serial output to flush
        esp_restart();  // Restart the device after successful provisioning
    } else {
        ESP_LOGE(TAG, "[Improv WiFi BLE] Failed to connect with provided credentials");
        ESP_LOGI(TAG, "[Improv WiFi BLE] Restarting BLE provisioning...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        wifi_improv_start_provisioning();
    }
}
#endif

void wifi_improv_start_provisioning() {
    #if USE_IMPROV_WIFI
    if (improv_provisioning_active) {
        return;  // Already provisioning
    }
    
    ESP_LOGI(TAG, "[Improv WiFi BLE] Starting BLE provisioning...");
    
    // Get unique chip ID (MAC address formatted as hex string) BEFORE disabling WiFi
    char chip_id[32] = {0};
    uint8_t mac[6];
    esp_err_t ret = esp_efuse_mac_get_default(mac);
    
    if (ret == ESP_OK) {
        // Format MAC address as hex string without colons (e.g., "AABBCCDDEEFF")
        snprintf(chip_id, sizeof(chip_id), "%02X%02X%02X%02X%02X%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    } else {
        // ESP-IDF: Get MAC from WiFi
        if (esp_wifi_get_mac(WIFI_IF_STA, mac) == ESP_OK) {
            snprintf(chip_id, sizeof(chip_id), "%02X%02X%02X%02X%02X%02X",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        }
    }
    
    // Build BLE device name: Use empty string to avoid adding to primary advertisement
    char ble_device_name[64] = {0};
    ble_device_name[0] = '\0';
    
    ESP_LOGI(TAG, "[Improv WiFi BLE] Chip ID: %s", chip_id);
    ESP_LOGI(TAG, "[Improv WiFi BLE] BLE device name: %s", ble_device_name);
    
    // Disable WiFi before initializing BLE to avoid coexistence conflicts
    ESP_LOGI(TAG, "[Improv WiFi BLE] Disabling WiFi for BLE...");
    esp_wifi_stop();
    esp_wifi_deinit();
    vTaskDelay(pdMS_TO_TICKS(100));  // Give WiFi time to fully shut down
    
    // Initialize BLE with device name including chip ID
    if (!NimBLEDevice::getInitialized()) {
        ESP_LOGI(TAG, "[Improv WiFi BLE] Initializing BLE...");
        NimBLEDevice::init(ble_device_name);
        ESP_LOGI(TAG, "[Improv WiFi BLE] BLE initialized");
    }
    
    // Set device info for Improv WiFi BLE
    const char* firmware_name = "P";
    const char* firmware_version = "1";
    
    ESP_LOGI(TAG, "[Improv WiFi BLE] Setting device info...");
    
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
    
    ESP_LOGI(TAG, "[Improv WiFi BLE] BLE provisioning active");
    ESP_LOGI(TAG, "[Improv WiFi BLE] Device advertising as '%s'", ble_device_name);
    ESP_LOGI(TAG, "[Improv WiFi BLE] Connect with Improv WiFi mobile app");
    #else
    ESP_LOGI(TAG, "[Improv WiFi] Improv WiFi is disabled in config.h");
    #endif
}

bool wifi_improv_is_provisioning() {
    return improv_provisioning_active;
}

void wifi_improv_loop() {
    #if USE_IMPROV_WIFI
    // Handle Improv WiFi BLE provisioning if active
    if (improv_provisioning_active) {
        // Check for timeout
        static uint64_t provisioning_start = 0;
        if (provisioning_start == 0) {
            provisioning_start = esp_timer_get_time() / 1000ULL;
        }
        
        uint64_t now = esp_timer_get_time() / 1000ULL;
        if (now - provisioning_start > IMPROV_WIFI_TIMEOUT_MS) {
            ESP_LOGW(TAG, "[Improv WiFi BLE] Provisioning timeout (5 minutes) - restarting device");
            improv_provisioning_active = false;
            provisioning_start = 0;
            
            // Deinitialize BLE first
            if (NimBLEDevice::getInitialized()) {
                ESP_LOGI(TAG, "[Improv WiFi BLE] Deinitializing BLE...");
                NimBLEDevice::deinit(true);
                vTaskDelay(pdMS_TO_TICKS(200));  // Give BLE time to fully shut down
            }
            
            ESP_LOGI(TAG, "[Improv WiFi BLE] Restarting device after timeout...");
            vTaskDelay(pdMS_TO_TICKS(1000));  // Give time for serial output to flush
            esp_restart();  // Restart the device after timeout
        }
    }
    #endif
}
