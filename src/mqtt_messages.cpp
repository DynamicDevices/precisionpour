/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * MQTT Message Handling Implementation
 * 
 * Handles MQTT event callbacks, message processing, and activity tracking
 */

#include "config.h"
#include "mqtt_messages.h"
#include "mqtt_connection.h"

// System/Standard library headers
#include <esp_log.h>
#include <mqtt_client.h>  // ESP-IDF MQTT client component
#include <cstring>
#include <string.h>
#define TAG "mqtt_msg"

// Project compatibility headers
#include "esp_idf_compat.h"

// Activity tracking
static unsigned long last_activity_time = 0;
static const unsigned long ACTIVITY_TIMEOUT_MS = 500;  // Show activity for 500ms after last TX/RX

// MQTT callback function (can be set by user)
static void (*user_callback)(char* topic, byte* payload, unsigned int length) = NULL;

// Include mqtt_connection.h for state updates
#include "mqtt_connection.h"

// ESP-IDF: MQTT event handler
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;
    
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED: {
            ESP_LOGI(TAG, "MQTT Connected");
            mqtt_connection_set_connected(true);
            mqtt_connection_set_connecting(false);  // No longer connecting
            mqtt_messages_mark_activity();
            
            // Subscribe to device-specific command topics
            const char* subscribe_topic = mqtt_connection_get_subscribe_topic();
            if (strlen(subscribe_topic) > 0) {
                int msg_id = esp_mqtt_client_subscribe(client, subscribe_topic, 0);
                ESP_LOGI(TAG, "Subscribed to topic: %s (msg_id: %d)", subscribe_topic, msg_id);
            }
            
            // Subscribe to paid command topic
            const char* paid_topic = mqtt_connection_get_paid_topic();
            if (strlen(paid_topic) > 0) {
                int msg_id = esp_mqtt_client_subscribe(client, paid_topic, 0);
                ESP_LOGI(TAG, "Subscribed to paid topic: %s (msg_id: %d)", paid_topic, msg_id);
            }
            break;
        }
            
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT Disconnected");
            mqtt_connection_set_connected(false);
            mqtt_connection_set_connecting(false);  // No longer connecting
            break;
            
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT subscribed, msg_id=%d", event->msg_id);
            mqtt_messages_mark_activity();
            break;
            
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT unsubscribed, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT published, msg_id=%d", event->msg_id);
            mqtt_messages_mark_activity();
            break;
            
        case MQTT_EVENT_DATA: {
            ESP_LOGI(TAG, "MQTT message received");
            mqtt_messages_mark_activity();
            
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
            mqtt_connection_set_connected(false);
            break;
            
        default:
            break;
    }
}

void mqtt_messages_init(void* client_handle) {
    // ESP-IDF: Register event handler
    esp_mqtt_client_handle_t handle = (esp_mqtt_client_handle_t)client_handle;
    if (handle != NULL) {
        esp_mqtt_client_register_event(handle, MQTT_EVENT_ANY, mqtt_event_handler, NULL);
    }
}

void mqtt_messages_set_callback(void (*callback)(char* topic, byte* payload, unsigned int length)) {
    user_callback = callback;
}

void mqtt_messages_mark_activity() {
    last_activity_time = millis();
}

bool mqtt_messages_has_activity() {
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
