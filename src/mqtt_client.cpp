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
 * Main MQTT client interface - delegates to mqtt_connection and mqtt_messages components
 */

// Project headers
#include "config.h"
#include "mqtt_manager.h"
#include "mqtt_connection.h"
#include "mqtt_messages.h"

// System/Standard library headers
#ifdef ESP_PLATFORM
    #include <esp_log.h>
    #include <mqtt_client.h>  // ESP-IDF MQTT client component
    #include <cstring>
    #define TAG "mqtt"
    
    // Project compatibility headers
    #include "esp_idf_compat.h"
#else
    // Arduino framework
    #include <Arduino.h>
    #include <PubSubClient.h>
#endif

bool mqtt_client_init(const char* chip_id) {
    return mqtt_connection_init(chip_id);
}

bool mqtt_client_is_connected() {
    return mqtt_connection_is_connected();
}

void mqtt_client_loop() {
    mqtt_connection_loop();
}

bool mqtt_client_publish(const char* topic, const char* payload) {
    if (!mqtt_connection_is_connected()) {
        #ifdef ESP_PLATFORM
            ESP_LOGW(TAG, "[MQTT] Cannot publish - not connected");
        #else
            Serial.println("[MQTT] Cannot publish - not connected");
        #endif
        return false;
    }
    
    #ifdef ESP_PLATFORM
        // ESP-IDF: Publish message
        esp_mqtt_client_handle_t handle = (esp_mqtt_client_handle_t)mqtt_connection_get_handle();
        if (handle == NULL) {
            ESP_LOGE(TAG, "[MQTT] Client handle is NULL");
            return false;
        }
        
        int msg_id = esp_mqtt_client_publish(handle, topic, payload, 0, 1, 0);
        if (msg_id >= 0) {
            mqtt_messages_mark_activity();
            ESP_LOGI(TAG, "[MQTT] Published to %s: %s (msg_id: %d)", topic, payload, msg_id);
            return true;
        } else {
            ESP_LOGE(TAG, "[MQTT] Failed to publish to %s", topic);
            return false;
        }
    #else
        // Arduino: Publish message
        PubSubClient* client = (PubSubClient*)mqtt_connection_get_handle();
        if (client == NULL) {
            Serial.println("[MQTT] Client handle is NULL");
            return false;
        }
        
        bool result = client->publish(topic, payload);
        if (result) {
            // Mark activity (message sent)
            mqtt_messages_mark_activity();
            Serial.printf("[MQTT] Published to %s: %s\r\n", topic, payload);
        } else {
            Serial.printf("[MQTT] Failed to publish to %s\r\n", topic);
        }
        return result;
    #endif
}

bool mqtt_client_subscribe(const char* topic) {
    if (!mqtt_connection_is_connected()) {
        #ifdef ESP_PLATFORM
            ESP_LOGW(TAG, "[MQTT] Cannot subscribe - not connected");
        #else
            Serial.println("[MQTT] Cannot subscribe - not connected");
        #endif
        return false;
    }
    
    #ifdef ESP_PLATFORM
        // ESP-IDF: Subscribe to topic
        esp_mqtt_client_handle_t handle = (esp_mqtt_client_handle_t)mqtt_connection_get_handle();
        if (handle == NULL) {
            ESP_LOGE(TAG, "[MQTT] Client handle is NULL");
            return false;
        }
        
        int msg_id = esp_mqtt_client_subscribe(handle, topic, 0);
        if (msg_id >= 0) {
            mqtt_messages_mark_activity();
            ESP_LOGI(TAG, "[MQTT] Subscribed to: %s (msg_id: %d)", topic, msg_id);
            return true;
        } else {
            ESP_LOGE(TAG, "[MQTT] Failed to subscribe to: %s", topic);
            return false;
        }
    #else
        // Arduino: Subscribe to topic
        PubSubClient* client = (PubSubClient*)mqtt_connection_get_handle();
        if (client == NULL) {
            Serial.println("[MQTT] Client handle is NULL");
            return false;
        }
        
        bool result = client->subscribe(topic);
        if (result) {
            // Mark activity (subscription is a form of communication)
            mqtt_messages_mark_activity();
            Serial.printf("[MQTT] Subscribed to: %s\r\n", topic);
        } else {
            Serial.printf("[MQTT] Failed to subscribe to: %s\r\n", topic);
        }
        return result;
    #endif
}

bool mqtt_client_has_activity() {
    return mqtt_messages_has_activity();
}

void mqtt_client_set_callback(void (*callback)(char* topic, byte* payload, unsigned int length)) {
    mqtt_messages_set_callback(callback);
}
