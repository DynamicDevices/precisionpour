/**
 * WiFi Manager Implementation
 * 
 * Handles WiFi connection and automatic reconnection
 */

#include "wifi_manager.h"
#include "config.h"
#include <Arduino.h>

static bool wifi_connected = false;
static unsigned long last_reconnect_attempt = 0;

bool wifi_manager_init() {
    Serial.println("\n=== Initializing WiFi ===");
    
    // Set WiFi mode to station (client)
    WiFi.mode(WIFI_STA);
    
    // Disable WiFi power saving for better connection stability
    WiFi.setSleep(false);
    
    // Start WiFi connection
    Serial.printf("Connecting to WiFi: %s\r\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    // Wait for connection (with timeout)
    unsigned long start_time = millis();
    unsigned long timeout = 30000; // 30 second timeout
    
    while (WiFi.status() != WL_CONNECTED && (millis() - start_time) < timeout) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        wifi_connected = true;
        Serial.println("WiFi connected!");
        Serial.printf("IP address: %s\r\n", WiFi.localIP().toString().c_str());
        Serial.printf("MAC address: %s\r\n", WiFi.macAddress().c_str());
        Serial.printf("Signal strength (RSSI): %d dBm\r\n", WiFi.RSSI());
        return true;
    } else {
        wifi_connected = false;
        Serial.println("WiFi connection failed!");
        Serial.println("Will attempt to reconnect in main loop...");
        last_reconnect_attempt = millis();
        return false;
    }
}

bool wifi_manager_is_connected() {
    // Check actual WiFi status
    if (WiFi.status() == WL_CONNECTED) {
        if (!wifi_connected) {
            // Just reconnected
            wifi_connected = true;
            Serial.println("WiFi reconnected!");
            Serial.printf("IP address: %s\r\n", WiFi.localIP().toString().c_str());
        }
        return true;
    } else {
        if (wifi_connected) {
            // Just disconnected
            wifi_connected = false;
            Serial.println("WiFi disconnected!");
        }
        return false;
    }
}

void wifi_manager_loop() {
    // Check connection status
    if (!wifi_manager_is_connected()) {
        // Try to reconnect if enough time has passed
        unsigned long now = millis();
        if (now - last_reconnect_attempt >= WIFI_RECONNECT_DELAY) {
            last_reconnect_attempt = now;
            Serial.println("Attempting WiFi reconnection...");
            
            WiFi.disconnect();
            delay(100);
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            
            // Wait a bit for connection
            int attempts = 0;
            while (WiFi.status() != WL_CONNECTED && attempts < 20) {
                delay(500);
                attempts++;
            }
            
            if (WiFi.status() == WL_CONNECTED) {
                wifi_connected = true;
                Serial.println("WiFi reconnected!");
                Serial.printf("IP address: %s\r\n", WiFi.localIP().toString().c_str());
            } else {
                Serial.println("WiFi reconnection failed, will try again...");
            }
        }
    }
}

String wifi_manager_get_ip() {
    if (wifi_manager_is_connected()) {
        return WiFi.localIP().toString();
    }
    return "Not connected";
}

String wifi_manager_get_mac_address() {
    return WiFi.macAddress();
}

int wifi_manager_get_rssi() {
    if (wifi_manager_is_connected()) {
        return WiFi.RSSI();
    }
    return -100;  // Return very weak signal when not connected
}
