/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * WiFi Manager
 * 
 * Handles WiFi connection and automatic reconnection
 * Supports Improv WiFi BLE provisioning for credential setup
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#ifdef ESP_PLATFORM
    // ESP-IDF framework
    #include "esp_idf_compat.h"  // Include this first to get String typedef
    #include <string>
    #include <cstring>
#else
    // Arduino framework
    #include <WiFi.h>
    #include <WString.h>
    typedef String String;  // Keep Arduino String type
#endif

// WiFi connection status
bool wifi_manager_init();
bool wifi_manager_is_connected();
void wifi_manager_loop();  // Call this in main loop for reconnection handling and Improv WiFi
String wifi_manager_get_ip();
String wifi_manager_get_mac_address();
int wifi_manager_get_rssi();  // Get WiFi signal strength (RSSI in dBm)

// Internal function for connecting to WiFi (used by wifi_improv)
bool wifi_manager_connect(const String& ssid, const String& password);

// Improv WiFi provisioning status (delegated to wifi_improv component)
bool wifi_manager_is_provisioning();  // Returns true if Improv WiFi provisioning is active
void wifi_manager_start_provisioning();  // Manually start Improv WiFi provisioning

#endif // WIFI_MANAGER_H
