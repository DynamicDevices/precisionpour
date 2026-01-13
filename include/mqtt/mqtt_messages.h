/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * MQTT Message Handling
 * 
 * Handles MQTT event callbacks, message processing, and activity tracking
 */

#ifndef MQTT_MESSAGES_H
#define MQTT_MESSAGES_H

#ifdef ESP_PLATFORM
    #include <stdint.h>
    typedef uint8_t byte;
#else
    #include <stdint.h>
    typedef uint8_t byte;
#endif

// Set user callback for received messages
void mqtt_messages_set_callback(void (*callback)(char* topic, byte* payload, unsigned int length));

// Check if there was recent MQTT activity (TX/RX)
bool mqtt_messages_has_activity();

// Mark activity (called internally)
void mqtt_messages_mark_activity();

// Initialize message handling (register event handlers)
void mqtt_messages_init(void* client_handle);

#endif // MQTT_MESSAGES_H
