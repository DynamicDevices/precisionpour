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

#include <WiFi.h>

// WiFi connection status
bool wifi_manager_init();
bool wifi_manager_is_connected();
void wifi_manager_loop();  // Call this in main loop for reconnection handling and Improv WiFi
String wifi_manager_get_ip();
String wifi_manager_get_mac_address();
int wifi_manager_get_rssi();  // Get WiFi signal strength (RSSI in dBm)

// Improv WiFi provisioning status
bool wifi_manager_is_provisioning();  // Returns true if Improv WiFi provisioning is active
void wifi_manager_start_provisioning();  // Manually start Improv WiFi provisioning

#endif // WIFI_MANAGER_H
