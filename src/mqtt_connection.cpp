/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * MQTT Connection Management Implementation
 * 
 * Handles MQTT client initialization, connection, and reconnection
 */

#include "config.h"
#include "mqtt_connection.h"
#include "wifi_manager.h"
#include "mqtt_messages.h"

// System/Standard library headers
#include <esp_log.h>
#include <esp_netif.h>
#include <mqtt_client.h>  // ESP-IDF MQTT client component
#include <cstring>
#include <string.h>
#define TAG "mqtt_conn"

// Project compatibility headers
#include "esp_idf_compat.h"

static esp_mqtt_client_handle_t mqtt_client_handle = NULL;

// Connection state
static char mqtt_client_id[64] = {0};
static char mqtt_subscribe_topic[128] = {0};
static char mqtt_paid_topic[128] = {0};  // Topic for "paid" command
static unsigned long last_reconnect_attempt = 0;
static bool mqtt_connected = false;
static bool mqtt_connecting = false;  // Track if we're in the process of connecting

bool mqtt_connection_init(const char* chip_id) {
    ESP_LOGI(TAG, "\n=== Initializing MQTT Client ===");
    
    // Build client ID: prefix + chip_id
    snprintf(mqtt_client_id, sizeof(mqtt_client_id), "%s_%s", MQTT_CLIENT_ID_PREFIX, chip_id);
    ESP_LOGI(TAG, "[MQTT] Client ID: %s", mqtt_client_id);
    
    // Build subscribe topic: prefix/chip_id/commands
    snprintf(mqtt_subscribe_topic, sizeof(mqtt_subscribe_topic), "%s/%s/commands", MQTT_TOPIC_PREFIX, chip_id);
    ESP_LOGI(TAG, "[MQTT] Subscribe topic: %s", mqtt_subscribe_topic);
    
    // Build paid command topic: prefix/chip_id/commands/paid
    snprintf(mqtt_paid_topic, sizeof(mqtt_paid_topic), "%s/%s/commands/paid", MQTT_TOPIC_PREFIX, chip_id);
    ESP_LOGI(TAG, "[MQTT] Paid topic: %s", mqtt_paid_topic);
    
    // ESP-IDF: Configure and create MQTT client
    // Check for KConfig value first, fallback to secrets.h
    const char* mqtt_server = NULL;
    #ifdef CONFIG_MQTT_SERVER
        if (strlen(CONFIG_MQTT_SERVER) > 0) {
            mqtt_server = CONFIG_MQTT_SERVER;
            ESP_LOGI(TAG, "[MQTT] Using KConfig server: %s", mqtt_server);
        } else {
            mqtt_server = MQTT_SERVER;  // Fallback to secrets.h
            ESP_LOGI(TAG, "[MQTT] KConfig server empty, using secrets.h: %s", mqtt_server);
        }
    #else
        mqtt_server = MQTT_SERVER;  // Fallback to secrets.h
        ESP_LOGI(TAG, "[MQTT] Using secrets.h server: %s", mqtt_server);
    #endif
    
    // Use URI format with mqtt:// scheme (required by ESP-IDF MQTT client)
    char uri[256];
    snprintf(uri, sizeof(uri), "mqtt://%s:%d", mqtt_server, MQTT_PORT);
    ESP_LOGI(TAG, "[MQTT] Connecting to: %s", uri);
    
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = uri;  // Use URI with scheme
    mqtt_cfg.credentials.client_id = mqtt_client_id;
    mqtt_cfg.session.keepalive = MQTT_KEEPALIVE;
    mqtt_cfg.network.reconnect_timeout_ms = MQTT_RECONNECT_DELAY;
    mqtt_cfg.network.timeout_ms = 5000;  // 5 second socket timeout
    
    mqtt_client_handle = esp_mqtt_client_init(&mqtt_cfg);
    if (mqtt_client_handle == NULL) {
        ESP_LOGE(TAG, "[MQTT] Failed to initialize MQTT client");
        return false;
    }
    
    // Initialize message handling (register event handlers)
    mqtt_messages_init(mqtt_client_handle);
    
    // Try to connect
    last_reconnect_attempt = 0;  // Force immediate connection attempt
    return mqtt_connection_reconnect(chip_id);
}

bool mqtt_connection_reconnect(const char* chip_id) {
    // Verify WiFi is connected before attempting MQTT connection
    if (!wifi_manager_is_connected()) {
        ESP_LOGW(TAG, "[MQTT] WiFi not connected, skipping MQTT connection");
        mqtt_connected = false;
        mqtt_connecting = false;
        return false;
    }
    
    // ESP-IDF: Check if we have an IP address
    String ip = wifi_manager_get_ip();
    if (ip.empty() || ip == std::string("Not connected")) {
        ESP_LOGW(TAG, "[MQTT] No IP address assigned, waiting for DHCP...");
        mqtt_connected = false;
        mqtt_connecting = false;
        return false;
    }
    
    // Start MQTT client (will connect automatically)
    if (mqtt_client_handle != NULL) {
        // Check if already connected - if so, don't try to start again
        if (mqtt_connected) {
            ESP_LOGI(TAG, "[MQTT] Already connected, skipping start");
            return true;
        }
        
        // Check if we're already in the process of connecting
        if (mqtt_connecting) {
            ESP_LOGI(TAG, "[MQTT] Connection already in progress, skipping start");
            return true;
        }
        
        ESP_LOGI(TAG, "[MQTT] Attempting to connect...");
        ESP_LOGI(TAG, "[MQTT] WiFi IP: %s", ip.c_str());
        
        // Mark as connecting to prevent multiple simultaneous attempts
        mqtt_connecting = true;
        
        // Try to start the client
        esp_err_t err = esp_mqtt_client_start(mqtt_client_handle);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "[MQTT] Connection initiated");
            return true;
        } else if (err == ESP_ERR_INVALID_STATE || err == ESP_FAIL) {
            // Client is already started (ESP_ERR_INVALID_STATE) or in a state where
            // start fails but it's already running (ESP_FAIL with "Client has started" message)
            // This is not an error - the client will connect automatically
            // Keep mqtt_connecting = true since connection is in progress
            ESP_LOGI(TAG, "[MQTT] Client already started (err=%s), connection in progress...", esp_err_to_name(err));
            return true;
        } else {
            ESP_LOGE(TAG, "[MQTT] Failed to start client: %s", esp_err_to_name(err));
            mqtt_connected = false;
            mqtt_connecting = false;  // Reset connecting flag on error
            return false;
        }
    } else {
        ESP_LOGE(TAG, "[MQTT] Client not initialized");
        mqtt_connected = false;
        mqtt_connecting = false;
        return false;
    }
}

bool mqtt_connection_is_connected() {
    // ESP-IDF: Check connection status
    // mqtt_connected is updated by event handler
    return mqtt_connected;
}

void mqtt_connection_loop() {
    // ESP-IDF: MQTT client runs in background, no loop needed
    // But we check connection and reconnect if needed
    // Don't try to reconnect if we're already connected or in the process of connecting
    if (!mqtt_connection_is_connected() && !mqtt_connection_is_connecting()) {
        unsigned long now = millis();
        if (now - last_reconnect_attempt >= MQTT_RECONNECT_DELAY) {
            last_reconnect_attempt = now;
            
            // Get chip ID from client ID (extract after prefix_)
            const char* chip_id = strstr(mqtt_client_id, "_");
            if (chip_id != NULL) {
                chip_id++;  // Skip the underscore
                mqtt_connection_reconnect(chip_id);
            }
        }
    }
}

void* mqtt_connection_get_handle() {
    return (void*)mqtt_client_handle;
}

const char* mqtt_connection_get_subscribe_topic() {
    return mqtt_subscribe_topic;
}

const char* mqtt_connection_get_paid_topic() {
    return mqtt_paid_topic;
}

// Internal function to update connection state (called by mqtt_messages)
void mqtt_connection_set_connected(bool connected) {
    mqtt_connected = connected;
}

void mqtt_connection_set_connecting(bool connecting) {
    mqtt_connecting = connecting;
}

// Get connecting state (for use by mqtt_client_loop)
bool mqtt_connection_is_connecting() {
    return mqtt_connecting;
}
