/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * MQTT Connection Management
 * 
 * Handles MQTT client initialization, connection, and reconnection
 */

#ifndef MQTT_CONNECTION_H
#define MQTT_CONNECTION_H

// Initialize MQTT client
bool mqtt_connection_init(const char* chip_id);

// Reconnect to MQTT broker
bool mqtt_connection_reconnect(const char* chip_id);

// Main loop - call this periodically
void mqtt_connection_loop();

// Check if connected
bool mqtt_connection_is_connected();

// Get client handle (for use by mqtt_messages)
void* mqtt_connection_get_handle();

// Get subscribe topics (for use by mqtt_messages)
const char* mqtt_connection_get_subscribe_topic();
const char* mqtt_connection_get_paid_topic();

// Internal state updates (for use by mqtt_messages)
void mqtt_connection_set_connected(bool connected);
void mqtt_connection_set_connecting(bool connecting);
bool mqtt_connection_is_connecting();

#endif // MQTT_CONNECTION_H
