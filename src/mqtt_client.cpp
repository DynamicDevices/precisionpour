/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * MQTT Client Implementation
 * 
 * Handles MQTT connection, subscription, and message handling
 */

#include "mqtt_manager.h"
#include "config.h"
#include "wifi_manager.h"

#ifdef ESP_PLATFORM
    // ESP-IDF framework
    #include "esp_idf_compat.h"
    #include "esp_log.h"
    #include "mqtt_client.h"  // ESP-IDF MQTT client component (from components/mqtt/esp-mqtt/include)
    #include "esp_netif.h"
    #include <string.h>
    #include <cstring>
    #define TAG "mqtt"
    
    static esp_mqtt_client_handle_t mqtt_client_handle = NULL;
#else
    // Arduino framework
    #include <Arduino.h>
    #include <WiFi.h>
    static WiFiClient wifi_client;
    static PubSubClient mqtt_client(wifi_client);
#endif

static char mqtt_client_id[64] = {0};
static char mqtt_subscribe_topic[128] = {0};
static char mqtt_paid_topic[128] = {0};  // Topic for "paid" command
static unsigned long last_reconnect_attempt = 0;
static bool mqtt_connected = false;
static unsigned long last_activity_time = 0;  // Track last TX/RX activity
static const unsigned long ACTIVITY_TIMEOUT_MS = 500;  // Show activity for 500ms after last TX/RX

// MQTT callback function (can be set by user)
static void (*user_callback)(char* topic, byte* payload, unsigned int length) = NULL;

#ifdef ESP_PLATFORM
// ESP-IDF: MQTT event handler
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;
    
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Connected");
            mqtt_connected = true;
            last_activity_time = millis();
            
            // Subscribe to device-specific command topics
            if (strlen(mqtt_subscribe_topic) > 0) {
                int msg_id = esp_mqtt_client_subscribe(client, mqtt_subscribe_topic, 0);
                ESP_LOGI(TAG, "Subscribed to topic: %s (msg_id: %d)", mqtt_subscribe_topic, msg_id);
            }
            
            // Subscribe to paid command topic
            if (strlen(mqtt_paid_topic) > 0) {
                int msg_id = esp_mqtt_client_subscribe(client, mqtt_paid_topic, 0);
                ESP_LOGI(TAG, "Subscribed to paid topic: %s (msg_id: %d)", mqtt_paid_topic, msg_id);
            }
            break;
            
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT Disconnected");
            mqtt_connected = false;
            break;
            
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT subscribed, msg_id=%d", event->msg_id);
            last_activity_time = millis();
            break;
            
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT unsubscribed, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT published, msg_id=%d", event->msg_id);
            last_activity_time = millis();
            break;
            
        case MQTT_EVENT_DATA: {
            ESP_LOGI(TAG, "MQTT message received");
            last_activity_time = millis();
            
            // Extract topic
            char topic[128] = {0};
            int topic_len = event->topic_len;
            if (topic_len > 0 && topic_len < sizeof(topic)) {
                memcpy(topic, event->topic, topic_len);
                topic[topic_len] = '\0';
            }
            
            // Extract payload
            char message[512] = {0};
            int data_len = event->data_len;
            if (data_len > 0 && data_len < sizeof(message)) {
                memcpy(message, event->data, data_len);
                message[data_len] = '\0';
            }
            
            ESP_LOGI(TAG, "Message received on topic: %s", topic);
            ESP_LOGI(TAG, "Message: %s", message);
            
            // Call user callback if set
            if (user_callback != NULL) {
                user_callback(topic, (byte*)event->data, event->data_len);
            }
            break;
        }
        
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error");
            mqtt_connected = false;
            break;
            
        default:
            break;
    }
}
#else
// Arduino: PubSubClient callback
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    // Mark activity (message received)
    last_activity_time = millis();
    
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
#endif

bool mqtt_client_reconnect(const char* chip_id) {
    // Verify WiFi is connected before attempting MQTT connection
    if (!wifi_manager_is_connected()) {
        #ifdef ESP_PLATFORM
            ESP_LOGW(TAG, "[MQTT] WiFi not connected, skipping MQTT connection");
        #else
            Serial.println("[MQTT] WiFi not connected, skipping MQTT connection");
        #endif
        mqtt_connected = false;
        return false;
    }
    
    #ifdef ESP_PLATFORM
        // ESP-IDF: Check if we have an IP address
        String ip = wifi_manager_get_ip();
        if (ip.empty() || ip == std::string("Not connected")) {
            ESP_LOGW(TAG, "[MQTT] No IP address assigned, waiting for DHCP...");
            mqtt_connected = false;
            return false;
        }
        
        ESP_LOGI(TAG, "[MQTT] Attempting to connect...");
        ESP_LOGI(TAG, "[MQTT] WiFi IP: %s", ip.c_str());
        
        // Start MQTT client (will connect automatically)
        if (mqtt_client_handle != NULL) {
            esp_err_t err = esp_mqtt_client_start(mqtt_client_handle);
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "[MQTT] Connection initiated");
                return true;
            } else {
                ESP_LOGE(TAG, "[MQTT] Failed to start client: %s", esp_err_to_name(err));
                mqtt_connected = false;
                return false;
            }
        } else {
            ESP_LOGE(TAG, "[MQTT] Client not initialized");
            mqtt_connected = false;
            return false;
        }
    #else
        // Arduino: Check if we have an IP address (DNS won't work without IP)
        IPAddress local_ip = WiFi.localIP();
        if (local_ip == IPAddress(0, 0, 0, 0)) {
            Serial.println("[MQTT] No IP address assigned, waiting for DHCP...");
            mqtt_connected = false;
            return false;
        }
        
        Serial.println("[MQTT] Attempting to connect...");
        Serial.printf("[MQTT] WiFi IP: %s\r\n", local_ip.toString().c_str());
        
        // Try to connect with client ID
        if (mqtt_client.connect(mqtt_client_id)) {
            mqtt_connected = true;
            Serial.println("[MQTT] Connected!");
            
            // Subscribe to device-specific command topics
            if (strlen(mqtt_subscribe_topic) > 0) {
                Serial.printf("[MQTT] Subscribing to topic: %s\r\n", mqtt_subscribe_topic);
                if (mqtt_client.subscribe(mqtt_subscribe_topic)) {
                    Serial.println("[MQTT] Subscription successful!");
                } else {
                    Serial.println("[MQTT] Subscription failed!");
                }
            }
            
            // Subscribe to paid command topic
            if (strlen(mqtt_paid_topic) > 0) {
                Serial.printf("[MQTT] Subscribing to topic: %s\r\n", mqtt_paid_topic);
                if (mqtt_client.subscribe(mqtt_paid_topic)) {
                    Serial.println("[MQTT] Paid topic subscription successful!");
                } else {
                    Serial.println("[MQTT] Paid topic subscription failed!");
                }
            }
            
            return true;
        } else {
            Serial.printf("[MQTT] Connection failed, rc=%d\r\n", mqtt_client.state());
            mqtt_connected = false;
            return false;
        }
    #endif
}

bool mqtt_client_init(const char* chip_id) {
    #ifdef ESP_PLATFORM
        ESP_LOGI(TAG, "\n=== Initializing MQTT Client ===");
    #else
        Serial.println("\n=== Initializing MQTT Client ===");
    #endif
    
    // Build client ID: prefix + chip_id
    snprintf(mqtt_client_id, sizeof(mqtt_client_id), "%s_%s", MQTT_CLIENT_ID_PREFIX, chip_id);
    #ifdef ESP_PLATFORM
        ESP_LOGI(TAG, "[MQTT] Client ID: %s", mqtt_client_id);
    #else
        Serial.printf("[MQTT] Client ID: %s\r\n", mqtt_client_id);
    #endif
    
    // Build subscribe topic: prefix/chip_id/commands
    snprintf(mqtt_subscribe_topic, sizeof(mqtt_subscribe_topic), "%s/%s/commands", MQTT_TOPIC_PREFIX, chip_id);
    #ifdef ESP_PLATFORM
        ESP_LOGI(TAG, "[MQTT] Subscribe topic: %s", mqtt_subscribe_topic);
    #else
        Serial.printf("[MQTT] Subscribe topic: %s\r\n", mqtt_subscribe_topic);
    #endif
    
    // Build paid command topic: prefix/chip_id/commands/paid
    snprintf(mqtt_paid_topic, sizeof(mqtt_paid_topic), "%s/%s/commands/paid", MQTT_TOPIC_PREFIX, chip_id);
    #ifdef ESP_PLATFORM
        ESP_LOGI(TAG, "[MQTT] Paid topic: %s", mqtt_paid_topic);
    #else
        Serial.printf("[MQTT] Paid topic: %s\r\n", mqtt_paid_topic);
    #endif
    
    #ifdef ESP_PLATFORM
        // ESP-IDF: Configure and create MQTT client
        // Use URI-based configuration (simpler and more compatible)
        char uri[256];
        snprintf(uri, sizeof(uri), "mqtt://%s:%d", MQTT_SERVER, MQTT_PORT);
        
        esp_mqtt_client_config_t mqtt_cfg = {};
        mqtt_cfg.broker.address.hostname = MQTT_SERVER;
        mqtt_cfg.broker.address.port = MQTT_PORT;
        mqtt_cfg.credentials.client_id = mqtt_client_id;
        mqtt_cfg.session.keepalive = MQTT_KEEPALIVE;
        mqtt_cfg.network.reconnect_timeout_ms = MQTT_RECONNECT_DELAY;
        mqtt_cfg.network.timeout_ms = 5000;  // 5 second socket timeout
        
        mqtt_client_handle = esp_mqtt_client_init(&mqtt_cfg);
        if (mqtt_client_handle == NULL) {
            ESP_LOGE(TAG, "[MQTT] Failed to initialize MQTT client");
            return false;
        }
        
        // Register event handler
        esp_mqtt_client_register_event(mqtt_client_handle, MQTT_EVENT_ANY, mqtt_event_handler, NULL);
        
        // Try to connect
        last_reconnect_attempt = 0;  // Force immediate connection attempt
        return mqtt_client_reconnect(chip_id);
    #else
        // Arduino: Configure MQTT server
        mqtt_client.setServer(MQTT_SERVER, MQTT_PORT);
        mqtt_client.setCallback(mqtt_callback);
        mqtt_client.setKeepAlive(MQTT_KEEPALIVE);
        mqtt_client.setSocketTimeout(5);  // 5 second socket timeout
        
        // Try to connect
        last_reconnect_attempt = 0;  // Force immediate connection attempt
        return mqtt_client_reconnect(chip_id);
    #endif
}

bool mqtt_client_is_connected() {
    #ifdef ESP_PLATFORM
        // ESP-IDF: Check connection status
        // mqtt_connected is updated by event handler
        return mqtt_connected;
    #else
        // Arduino: Check connection status
        bool connected = mqtt_client.connected();
        if (connected != mqtt_connected) {
            mqtt_connected = connected;
            if (!connected) {
                Serial.println("[MQTT] Disconnected!");
            }
        }
        return connected;
    #endif
}

void mqtt_client_loop() {
    #ifdef ESP_PLATFORM
        // ESP-IDF: MQTT client runs in background, no loop needed
        // But we check connection and reconnect if needed
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
    #else
        // Arduino: Process MQTT messages
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
    #endif
}

bool mqtt_client_publish(const char* topic, const char* payload) {
    if (!mqtt_client_is_connected()) {
        #ifdef ESP_PLATFORM
            ESP_LOGW(TAG, "[MQTT] Cannot publish - not connected");
        #else
            Serial.println("[MQTT] Cannot publish - not connected");
        #endif
        return false;
    }
    
    #ifdef ESP_PLATFORM
        // ESP-IDF: Publish message
        int msg_id = esp_mqtt_client_publish(mqtt_client_handle, topic, payload, 0, 1, 0);
        if (msg_id >= 0) {
            last_activity_time = millis();
            ESP_LOGI(TAG, "[MQTT] Published to %s: %s (msg_id: %d)", topic, payload, msg_id);
            return true;
        } else {
            ESP_LOGE(TAG, "[MQTT] Failed to publish to %s", topic);
            return false;
        }
    #else
        // Arduino: Publish message
        bool result = mqtt_client.publish(topic, payload);
        if (result) {
            // Mark activity (message sent)
            last_activity_time = millis();
            Serial.printf("[MQTT] Published to %s: %s\r\n", topic, payload);
        } else {
            Serial.printf("[MQTT] Failed to publish to %s\r\n", topic);
        }
        return result;
    #endif
}

bool mqtt_client_subscribe(const char* topic) {
    if (!mqtt_client_is_connected()) {
        #ifdef ESP_PLATFORM
            ESP_LOGW(TAG, "[MQTT] Cannot subscribe - not connected");
        #else
            Serial.println("[MQTT] Cannot subscribe - not connected");
        #endif
        return false;
    }
    
    #ifdef ESP_PLATFORM
        // ESP-IDF: Subscribe to topic
        int msg_id = esp_mqtt_client_subscribe(mqtt_client_handle, topic, 0);
        if (msg_id >= 0) {
            last_activity_time = millis();
            ESP_LOGI(TAG, "[MQTT] Subscribed to: %s (msg_id: %d)", topic, msg_id);
            return true;
        } else {
            ESP_LOGE(TAG, "[MQTT] Failed to subscribe to: %s", topic);
            return false;
        }
    #else
        // Arduino: Subscribe to topic
        bool result = mqtt_client.subscribe(topic);
        if (result) {
            // Mark activity (subscription is a form of communication)
            last_activity_time = millis();
            Serial.printf("[MQTT] Subscribed to: %s\r\n", topic);
        } else {
            Serial.printf("[MQTT] Failed to subscribe to: %s\r\n", topic);
        }
        return result;
    #endif
}

bool mqtt_client_has_activity() {
    // Return true if there was activity within the timeout period
    if (last_activity_time == 0) {
        return false;
    }
    unsigned long now = millis();
    // Handle millis() overflow (happens every ~49 days)
    if (now < last_activity_time) {
        // Overflow occurred, reset
        last_activity_time = 0;
        return false;
    }
    return (now - last_activity_time) < ACTIVITY_TIMEOUT_MS;
}

void mqtt_client_set_callback(void (*callback)(char* topic, byte* payload, unsigned int length)) {
    user_callback = callback;
}
