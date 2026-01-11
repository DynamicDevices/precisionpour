/**
 * WiFi Manager
 * 
 * Handles WiFi connection and automatic reconnection
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>

// WiFi connection status
bool wifi_manager_init();
bool wifi_manager_is_connected();
void wifi_manager_loop();  // Call this in main loop for reconnection handling
String wifi_manager_get_ip();
String wifi_manager_get_mac_address();

#endif // WIFI_MANAGER_H
