/**
 * MQTT Client Manager
 * 
 * Handles MQTT connection, subscription, and message handling
 */

#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <PubSubClient.h>
#include <WiFiClient.h>

// MQTT connection status
bool mqtt_client_init(const char* chip_id);
bool mqtt_client_is_connected();
void mqtt_client_loop();  // Call this in main loop
bool mqtt_client_publish(const char* topic, const char* payload);
bool mqtt_client_subscribe(const char* topic);
void mqtt_client_set_callback(void (*callback)(char* topic, byte* payload, unsigned int length));

#endif // MQTT_CLIENT_H
