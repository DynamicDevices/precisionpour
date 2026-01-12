/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * WiFi Improv Provisioning Implementation
 * 
 * Handles Improv WiFi BLE provisioning for credential setup
 */

#include "config.h"
#include "wifi_improv.h"
#include "wifi_credentials.h"

// System/Standard library headers
#ifdef ESP_PLATFORM
    #include <esp_log.h>
    #include <esp_mac.h>
    #include <esp_system.h>
    #include <esp_wifi.h>
    #include <cstring>
    #define TAG "wifi_improv"
    
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
    #include <Arduino.h>
    #include <ImprovWiFiBLE.h>
    #include <NimBLEAdvertising.h>
    #include <NimBLEDevice.h>
    #include "esp_efuse.h"
    #include "esp_system.h"
    #include <WiFi.h>
#endif

// Forward declaration - callback to connect to WiFi (provided by wifi_manager)
extern bool wifi_manager_connect(const String& ssid, const String& password);

static bool improv_provisioning_active = false;

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
    if (wifi_manager_connect(String(ssid), String(password))) {
        // Save credentials for future use
        wifi_credentials_save(String(ssid), String(password));
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
        wifi_improv_start_provisioning();
    }
}
#endif

void wifi_improv_start_provisioning() {
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

bool wifi_improv_is_provisioning() {
    return improv_provisioning_active;
}

void wifi_improv_loop() {
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
    }
    #endif
}
