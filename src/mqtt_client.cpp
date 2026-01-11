/**
 * MQTT Client Implementation
 * 
 * Handles MQTT connection, subscription, and message handling
 */

#include "mqtt_client.h"
#include "config.h"
#include <Arduino.h>

static WiFiClient wifi_client;
static PubSubClient mqtt_client(wifi_client);
static char mqtt_client_id[64] = {0};
static char mqtt_subscribe_topic[128] = {0};
static unsigned long last_reconnect_attempt = 0;
static bool mqtt_connected = false;

// MQTT callback function (can be set by user)
static void (*user_callback)(char* topic, byte* payload, unsigned int length) = NULL;

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    // Null-terminate the payload
    char message[256] = {0};
    if (length < sizeof(message)) {
        memcpy(message, payload, length);
        message[length] = '\0';
    } else {
        memcpy(message, payload, sizeof(message) - 1);
    }
    
    Serial.printf("[MQTT] Message received on topic: %s\r\n", topic);
    Serial.printf("[MQTT] Message: %s\r\n", message);
    
    // Call user callback if set
    if (user_callback != NULL) {
        user_callback(topic, payload, length);
    }
}

bool mqtt_client_reconnect(const char* chip_id) {
    Serial.println("[MQTT] Attempting to connect...");
    
    // Try to connect with client ID
    if (mqtt_client.connect(mqtt_client_id)) {
        mqtt_connected = true;
        Serial.println("[MQTT] Connected!");
        
        // Subscribe to device-specific topic
        if (strlen(mqtt_subscribe_topic) > 0) {
            Serial.printf("[MQTT] Subscribing to topic: %s\r\n", mqtt_subscribe_topic);
            if (mqtt_client.subscribe(mqtt_subscribe_topic)) {
                Serial.println("[MQTT] Subscription successful!");
            } else {
                Serial.println("[MQTT] Subscription failed!");
            }
        }
        
        return true;
    } else {
        Serial.printf("[MQTT] Connection failed, rc=%d\r\n", mqtt_client.state());
        mqtt_connected = false;
        return false;
    }
}

bool mqtt_client_init(const char* chip_id) {
    Serial.println("\n=== Initializing MQTT Client ===");
    
    // Build client ID: prefix + chip_id
    snprintf(mqtt_client_id, sizeof(mqtt_client_id), "%s_%s", MQTT_CLIENT_ID_PREFIX, chip_id);
    Serial.printf("[MQTT] Client ID: %s\r\n", mqtt_client_id);
    
    // Build subscribe topic: prefix/chip_id/commands
    snprintf(mqtt_subscribe_topic, sizeof(mqtt_subscribe_topic), "%s/%s/commands", MQTT_TOPIC_PREFIX, chip_id);
    Serial.printf("[MQTT] Subscribe topic: %s\r\n", mqtt_subscribe_topic);
    
    // Configure MQTT server
    mqtt_client.setServer(MQTT_SERVER, MQTT_PORT);
    mqtt_client.setCallback(mqtt_callback);
    mqtt_client.setKeepAlive(MQTT_KEEPALIVE);
    mqtt_client.setSocketTimeout(5);  // 5 second socket timeout
    
    // Try to connect
    last_reconnect_attempt = 0;  // Force immediate connection attempt
    return mqtt_client_reconnect(chip_id);
}

bool mqtt_client_is_connected() {
    bool connected = mqtt_client.connected();
    if (connected != mqtt_connected) {
        mqtt_connected = connected;
        if (!connected) {
            Serial.println("[MQTT] Disconnected!");
        }
    }
    return connected;
}

void mqtt_client_loop() {
    // Process MQTT messages
    mqtt_client.loop();
    
    // Check connection and reconnect if needed
    if (!mqtt_client_is_connected()) {
        unsigned long now = millis();
        if (now - last_reconnect_attempt >= MQTT_RECONNECT_DELAY) {
            last_reconnect_attempt = now;
            
            // Get chip ID from client ID (extract after prefix_)
            const char* chip_id = strstr(mqtt_client_id, "_");
            if (chip_id != NULL) {
                chip_id++;  // Skip the underscore
                mqtt_client_reconnect(chip_id);
            }
        }
    }
}

bool mqtt_client_publish(const char* topic, const char* payload) {
    if (!mqtt_client_is_connected()) {
        Serial.println("[MQTT] Cannot publish - not connected");
        return false;
    }
    
    bool result = mqtt_client.publish(topic, payload);
    if (result) {
        Serial.printf("[MQTT] Published to %s: %s\r\n", topic, payload);
    } else {
        Serial.printf("[MQTT] Failed to publish to %s\r\n", topic);
    }
    return result;
}

bool mqtt_client_subscribe(const char* topic) {
    if (!mqtt_client_is_connected()) {
        Serial.println("[MQTT] Cannot subscribe - not connected");
        return false;
    }
    
    bool result = mqtt_client.subscribe(topic);
    if (result) {
        Serial.printf("[MQTT] Subscribed to: %s\r\n", topic);
    } else {
        Serial.printf("[MQTT] Failed to subscribe to: %s\r\n", topic);
    }
    return result;
}

void mqtt_client_set_callback(void (*callback)(char* topic, byte* payload, unsigned int length)) {
    user_callback = callback;
}
