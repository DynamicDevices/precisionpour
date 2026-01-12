/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * WiFi Improv Provisioning
 * 
 * Handles Improv WiFi BLE provisioning for credential setup
 */

#ifndef WIFI_IMPROV_H
#define WIFI_IMPROV_H

// Start Improv WiFi BLE provisioning
void wifi_improv_start_provisioning();

// Check if provisioning is active
bool wifi_improv_is_provisioning();

// Call this in main loop to handle provisioning
void wifi_improv_loop();

#endif // WIFI_IMPROV_H
