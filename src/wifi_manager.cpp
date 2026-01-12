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

#include "wifi_manager.h"
#include "config.h"
#include <Arduino.h>
#include <Preferences.h>
#include <ImprovWiFiBLE.h>
#include "esp_efuse.h"
#include "esp_system.h"
#include <NimBLEDevice.h>
#include <NimBLEAdvertising.h>

// EEPROM/Preferences keys for storing WiFi credentials
#define PREF_NAMESPACE "wifi"
#define PREF_KEY_SSID "ssid"
#define PREF_KEY_PASSWORD "password"
#define PREF_KEY_USE_SAVED "use_saved"

static bool wifi_connected = false;
static unsigned long last_reconnect_attempt = 0;
static bool improv_provisioning_active = false;
static Preferences preferences;

// Forward declarations
static bool connect_to_wifi(const String& ssid, const String& password);
static void save_credentials(const String& ssid, const String& password);

// Improv WiFi BLE instance
#if USE_IMPROV_WIFI
static ImprovWiFiBLE improvWiFiBLE;

// Callback functions for Improv WiFi BLE
static void on_improv_error(ImprovTypes::Error error) {
    Serial.printf("[Improv WiFi BLE] Error: %d\r\n", (int)error);
}

static void on_improv_connected(const char* ssid, const char* password) {
    Serial.printf("[Improv WiFi BLE] Received credentials for: %s\r\n", ssid);
    
    // Deinitialize BLE first before re-enabling WiFi
    if (NimBLEDevice::getInitialized()) {
        Serial.println("[Improv WiFi BLE] Deinitializing BLE before connecting to WiFi...");
        NimBLEDevice::deinit(true);
        delay(200);  // Give BLE time to fully shut down
    }
    
    // Re-enable WiFi
    WiFi.mode(WIFI_STA);
    delay(100);
    
    // Try to connect with new credentials
    if (connect_to_wifi(String(ssid), String(password))) {
        // Save credentials for future use
        save_credentials(String(ssid), String(password));
        improv_provisioning_active = false;
        Serial.println("[Improv WiFi BLE] Provisioning successful!");
        Serial.println("[Improv WiFi BLE] Credentials saved, restarting device...");
        delay(1000);  // Give time for serial output to flush
        ESP.restart();  // Restart the device after successful provisioning
    } else {
        Serial.println("[Improv WiFi BLE] Failed to connect with provided credentials");
        // Restart BLE provisioning if connection failed
        Serial.println("[Improv WiFi BLE] Restarting BLE provisioning...");
        delay(1000);
        wifi_manager_start_provisioning();
    }
}
#endif

// Forward declarations (already declared above for Improv callbacks)
static bool load_saved_credentials(String& ssid, String& password);
static void clear_saved_credentials();

/**
 * Load saved WiFi credentials from EEPROM/Preferences
 */
static bool load_saved_credentials(String& ssid, String& password) {
    if (!USE_SAVED_CREDENTIALS) {
        return false;
    }
    
    // Try to open Preferences namespace (read-only)
    // If namespace doesn't exist (NOT_FOUND), that's okay - just means no saved credentials yet
    if (!preferences.begin(PREF_NAMESPACE, true)) {  // Read-only mode
        // Namespace doesn't exist yet - this is normal on first boot
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
}

/**
 * Save WiFi credentials to EEPROM/Preferences
 */
static void save_credentials(const String& ssid, const String& password) {
    // Open Preferences namespace (read-write mode)
    // This will create the namespace if it doesn't exist
    if (!preferences.begin(PREF_NAMESPACE, false)) {  // Read-write mode
        Serial.println("[WiFi] ERROR: Failed to open Preferences namespace for writing");
        return;
    }
    
    preferences.putString(PREF_KEY_SSID, ssid);
    preferences.putString(PREF_KEY_PASSWORD, password);
    preferences.putBool(PREF_KEY_USE_SAVED, true);
    preferences.end();
    
    Serial.printf("[WiFi] Saved credentials for: %s\r\n", ssid.c_str());
}

/**
 * Clear saved WiFi credentials
 */
static void clear_saved_credentials() {
    // Try to open Preferences namespace (read-write mode)
    // If namespace doesn't exist, there's nothing to clear
    if (!preferences.begin(PREF_NAMESPACE, false)) {
        // Namespace doesn't exist - nothing to clear
        return;
    }
    
    preferences.remove(PREF_KEY_SSID);
    preferences.remove(PREF_KEY_PASSWORD);
    preferences.putBool(PREF_KEY_USE_SAVED, false);
    preferences.end();
    
    Serial.println("[WiFi] Cleared saved credentials");
}

/**
 * Connect to WiFi network
 */
static bool connect_to_wifi(const String& ssid, const String& password) {
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
}

bool wifi_manager_init() {
    Serial.println("\n=== Initializing WiFi ===");
    
    String ssid, password;
    bool use_saved = false;
    
    // Try to load saved credentials first
    if (USE_SAVED_CREDENTIALS && load_saved_credentials(ssid, password)) {
        use_saved = true;
        Serial.println("[WiFi] Using saved credentials");
    } else {
        // Use hardcoded credentials from secrets.h
        ssid = String(WIFI_SSID);
        password = String(WIFI_PASSWORD);
        Serial.println("[WiFi] Using hardcoded credentials");
    }
    
    // Try to connect with available credentials
    if (connect_to_wifi(ssid, password)) {
        return true;
    }
    
    // Connection failed - start Improv WiFi provisioning if enabled
    #if USE_IMPROV_WIFI
    if (!wifi_connected) {
        Serial.println("[WiFi] Connection failed - starting Improv WiFi provisioning...");
        wifi_manager_start_provisioning();
        return false;  // Not connected yet, provisioning in progress
    }
    #else
    Serial.println("[WiFi] Connection failed - Improv WiFi disabled, will retry...");
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
    
    Serial.println("[Improv WiFi BLE] Starting BLE provisioning...");
    
    // Get unique chip ID (MAC address formatted as hex string) BEFORE disabling WiFi
    char chip_id[32] = {0};
    uint8_t mac[6];
    esp_err_t ret = esp_efuse_mac_get_default(mac);
    
    if (ret == ESP_OK) {
        // Format MAC address as hex string without colons (e.g., "AABBCCDDEEFF")
        snprintf(chip_id, sizeof(chip_id), "%02X%02X%02X%02X%02X%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    } else {
        // Fallback: use WiFi MAC address (get it before disabling WiFi)
        String wifi_mac = WiFi.macAddress();
        wifi_mac.replace(":", "");
        strncpy(chip_id, wifi_mac.c_str(), sizeof(chip_id) - 1);
    }
    
    // Build BLE device name: Use empty string to avoid adding to primary advertisement
    // The name will be in scan response instead (handled by Improv library)
    // This saves space in the 31-byte primary advertisement packet
    // Primary ADV breakdown: Flags (3) + 128-bit UUID (18) + Service Data (11) = 32 bytes (over limit!)
    // So we MUST use empty device name to avoid exceeding limit
    char ble_device_name[64] = {0};
    // Use empty string - name will be in scan response
    ble_device_name[0] = '\0';
    
    Serial.printf("[Improv WiFi BLE] Chip ID: %s\r\n", chip_id);
    Serial.printf("[Improv WiFi BLE] BLE device name: %s\r\n", ble_device_name);
    
    // Disable WiFi before initializing BLE to avoid coexistence conflicts
    // WiFi and BLE share the same radio on ESP32, so we need to disable WiFi first
    Serial.println("[Improv WiFi BLE] Disabling WiFi for BLE...");
    WiFi.disconnect(true);  // Disconnect and disable WiFi
    WiFi.mode(WIFI_OFF);     // Turn off WiFi completely
    delay(100);               // Give WiFi time to fully shut down
    
    // Initialize BLE with device name including chip ID
    if (!NimBLEDevice::getInitialized()) {
        Serial.println("[Improv WiFi BLE] Initializing BLE...");
        NimBLEDevice::init(ble_device_name);
        Serial.println("[Improv WiFi BLE] BLE initialized");
    }
    
    // Set device info for Improv WiFi BLE (this also sets up BLE advertising)
    // Use short strings to avoid exceeding BLE advertising data length (31 bytes max)
    
    // Log string lengths for debugging advertising data size
    // Minimized to absolute minimum to fit within 31-byte BLE advertising limit
    const char* firmware_name = "P";      // Single character (was "PP")
    const char* firmware_version = "1";   // Single character (was "1.0")
    Serial.printf("[Improv WiFi BLE] Advertising data breakdown:\r\n");
    Serial.printf("  Primary advertisement components:\r\n");
    Serial.printf("    - Flags: 3 bytes (0x06)\r\n");
    Serial.printf("    - 128-bit Service UUID: 18 bytes (Improv protocol requirement)\r\n");
    Serial.printf("    - Service Data (0x4677 + 8-byte payload): 11 bytes\r\n");
    Serial.printf("    Subtotal: 32 bytes (ALREADY OVER 31-byte limit!)\r\n");
    Serial.printf("  Device info strings (used in service, not primary ADV):\r\n");
    Serial.printf("    - Firmware name: '%s' (%d bytes)\r\n", firmware_name, strlen(firmware_name));
    Serial.printf("    - Firmware version: '%s' (%d bytes)\r\n", firmware_version, strlen(firmware_version));
    Serial.printf("    - Device name: '%s' (%d bytes) - will be in scan response\r\n", 
                  ble_device_name[0] ? ble_device_name : "(empty)", 
                  strlen(ble_device_name));
    Serial.printf("  NOTE: Device name set to empty to avoid adding to primary ADV\r\n");
    Serial.printf("        Name will appear in scan response instead (separate packet)\r\n");
    
    improvWiFiBLE.setDeviceInfo(
        ImprovTypes::ChipFamily::CF_ESP32,
        firmware_name,
        firmware_version,
        ble_device_name
    );
    
    // Research findings from official Improv WiFi SDK (https://github.com/improv-wifi/sdk-cpp):
    // - The SDK defines the 128-bit UUIDs but doesn't implement BLE advertising
    // - Our library (Improv WiFi Library) implements BLE on top of the SDK
    // - The library code shows awareness of 31-byte limit (see ImprovWiFiBLE.cpp line 315-316)
    //
    // Primary advertisement breakdown (REQUIRED by Improv protocol):
    // 1. Flags: 3 bytes (0x06)
    // 2. 128-bit Service UUID: 18 bytes (REQUIRED - defined in SDK, cannot be shortened)
    // 3. Service Data (UUID 0x4677 + 8-byte payload): 11 bytes (REQUIRED)
    // Total: 32 bytes (exceeds 31-byte BLE limit by 1 byte)
    //
    // The library already optimizes by:
    // - Using scan response for full device name (separate 31-byte packet)
    // - Only adding short name to primary ADV if device_name is not empty
    //
    // Current solution: Use empty device name to prevent adding it to primary ADV
    // This keeps primary ADV at exactly 32 bytes (1 byte over, but NimBLE may handle it)
    // Device name appears in scan response for device identification
    
    Serial.println("[Improv WiFi BLE] Primary ADV: 32 bytes (Flags:3 + UUID:18 + ServiceData:11)");
    Serial.println("[Improv WiFi BLE] This exceeds 31-byte limit by 1 byte (Improv protocol requirement)");
    Serial.println("[Improv WiFi BLE] Using empty device name to avoid adding to primary ADV");
    Serial.println("[Improv WiFi BLE] Device name will appear in scan response (separate packet)");
    Serial.println("[Improv WiFi BLE] NimBLE may handle 1-byte overage gracefully or log warning");
    
    // Set callbacks
    improvWiFiBLE.onImprovError(on_improv_error);
    improvWiFiBLE.onImprovConnected(on_improv_connected);
    
    improv_provisioning_active = true;
    
    Serial.println("[Improv WiFi BLE] BLE provisioning active");
    Serial.printf("[Improv WiFi BLE] Device advertising as '%s'\r\n", ble_device_name);
    Serial.println("[Improv WiFi BLE] Connect with Improv WiFi mobile app");
    #else
    Serial.println("[Improv WiFi] Improv WiFi is disabled in config.h");
    #endif
}

bool wifi_manager_is_provisioning() {
    return improv_provisioning_active;
}

bool wifi_manager_is_connected() {
    // Check actual WiFi status
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
}

void wifi_manager_loop() {
    #if USE_IMPROV_WIFI
    // Handle Improv WiFi BLE provisioning if active
    // Note: BLE callbacks are handled automatically by NimBLE
    // No need to call a loop() function like Serial version
    
    if (improv_provisioning_active) {
        // Check for timeout
        static unsigned long provisioning_start = 0;
        if (provisioning_start == 0) {
            provisioning_start = millis();
        }
        
        if (millis() - provisioning_start > IMPROV_WIFI_TIMEOUT_MS) {
            Serial.println("[Improv WiFi BLE] Provisioning timeout (5 minutes) - restarting device");
            improv_provisioning_active = false;
            provisioning_start = 0;
            
            // Deinitialize BLE first
            if (NimBLEDevice::getInitialized()) {
                Serial.println("[Improv WiFi BLE] Deinitializing BLE...");
                NimBLEDevice::deinit(true);
                delay(200);  // Give BLE time to fully shut down
            }
            
            Serial.println("[Improv WiFi BLE] Restarting device after timeout...");
            delay(1000);  // Give time for serial output to flush
            ESP.restart();  // Restart the device after timeout
        }
        
        // If WiFi connected, stop provisioning and re-enable WiFi
        if (wifi_manager_is_connected() && improv_provisioning_active) {
            improv_provisioning_active = false;
            provisioning_start = 0;
            
            // Deinitialize BLE first
            if (NimBLEDevice::getInitialized()) {
                Serial.println("[Improv WiFi BLE] Deinitializing BLE...");
                NimBLEDevice::deinit(true);
                delay(200);  // Give BLE time to fully shut down
            }
            
            // Re-enable WiFi (it should already be connected, but ensure it's enabled)
            WiFi.mode(WIFI_STA);
            Serial.println("[Improv WiFi BLE] Provisioning stopped - WiFi connected, BLE deinitialized");
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
            Serial.println("[WiFi] Attempting reconnection...");
            
            String ssid, password;
            bool use_saved = false;
            
            // Try saved credentials first
            if (USE_SAVED_CREDENTIALS && load_saved_credentials(ssid, password)) {
                use_saved = true;
            } else {
                ssid = String(WIFI_SSID);
                password = String(WIFI_PASSWORD);
            }
            
            WiFi.disconnect();
            delay(100);
            WiFi.begin(ssid.c_str(), password.c_str());
            
            // Wait a bit for connection
            int attempts = 0;
            while (WiFi.status() != WL_CONNECTED && attempts < 20) {
                delay(500);
                attempts++;
            }
            
            if (WiFi.status() == WL_CONNECTED) {
                wifi_connected = true;
                Serial.println("[WiFi] Reconnected!");
                Serial.printf("[WiFi] IP address: %s\r\n", WiFi.localIP().toString().c_str());
            } else {
                Serial.println("[WiFi] Reconnection failed, will try again...");
                
                // If reconnection fails and Improv WiFi is enabled, start provisioning
                #if USE_IMPROV_WIFI
                if (!improv_provisioning_active && (now - last_reconnect_attempt) > 60000) {
                    // Only start provisioning after 1 minute of failed reconnections
                    Serial.println("[WiFi] Starting Improv WiFi provisioning after failed reconnection...");
                    wifi_manager_start_provisioning();
                }
                #endif
            }
        }
    }
}

String wifi_manager_get_ip() {
    if (wifi_manager_is_connected()) {
        return WiFi.localIP().toString();
    }
    return "Not connected";
}

String wifi_manager_get_mac_address() {
    return WiFi.macAddress();
}

int wifi_manager_get_rssi() {
    if (wifi_manager_is_connected()) {
        return WiFi.RSSI();
    }
    return -100;  // Return very weak signal when not connected
}
