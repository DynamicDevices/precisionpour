/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * MQTT Client Manager
 * 
 * Handles MQTT connection, subscription, and message handling
 */

#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#ifdef ESP_PLATFORM
    // ESP-IDF framework
    #include <stdint.h>
    typedef uint8_t byte;
#else
    // Arduino framework
    #include <PubSubClient.h>
    #include <WiFiClient.h>
#endif

// MQTT connection status
bool mqtt_client_init(const char* chip_id);
bool mqtt_client_is_connected();
void mqtt_client_loop();  // Call this in main loop
bool mqtt_client_publish(const char* topic, const char* payload);
bool mqtt_client_subscribe(const char* topic);
void mqtt_client_set_callback(void (*callback)(char* topic, byte* payload, unsigned int length));
bool mqtt_client_has_activity();  // Returns true if there was recent TX/RX activity

#endif // MQTT_MANAGER_H
