/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * WiFi Credentials Storage Implementation
 * 
 * Handles saving and loading WiFi credentials from NVS (ESP-IDF) or Preferences (Arduino)
 */

#include "config.h"
#include "wifi/wifi_credentials.h"

// System/Standard library headers
#include <esp_log.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <cstring>
#define TAG "wifi_creds"

// Project compatibility headers
#include "system/esp_idf_compat.h"

static nvs_handle_t wifi_nvs_handle = 0;

// EEPROM/Preferences keys for storing WiFi credentials
#define PREF_NAMESPACE "wifi"
#define PREF_KEY_SSID "ssid"
#define PREF_KEY_PASSWORD "password"
#define PREF_KEY_USE_SAVED "use_saved"

/**
 * Load saved WiFi credentials from NVS
 */
bool wifi_credentials_load(String& ssid, String& password) {
    if (!USE_SAVED_CREDENTIALS) {
        return false;
    }
    
    // ESP-IDF: Use NVS
    esp_err_t err = nvs_open(PREF_NAMESPACE, NVS_READONLY, &wifi_nvs_handle);
    if (err != ESP_OK) {
        return false;  // Namespace doesn't exist yet
    }
    
    bool use_saved = false;
    size_t required_size = sizeof(bool);
    err = nvs_get_blob(wifi_nvs_handle, PREF_KEY_USE_SAVED, &use_saved, &required_size);
    if (err != ESP_OK || !use_saved) {
        nvs_close(wifi_nvs_handle);
        return false;
    }
    
    // Read SSID
    required_size = 64;
    char ssid_buf[64] = {0};
    err = nvs_get_str(wifi_nvs_handle, PREF_KEY_SSID, ssid_buf, &required_size);
    if (err == ESP_OK && strlen(ssid_buf) > 0) {
        ssid = String(ssid_buf);
        
        // Read password
        required_size = 64;
        char password_buf[64] = {0};
        err = nvs_get_str(wifi_nvs_handle, PREF_KEY_PASSWORD, password_buf, &required_size);
        if (err == ESP_OK) {
            password = String(password_buf);
        }
        
        nvs_close(wifi_nvs_handle);
        ESP_LOGI(TAG, "[WiFi] Loaded saved credentials for: %s", ssid.c_str());
        return true;
    }
    
    nvs_close(wifi_nvs_handle);
    return false;
}

/**
 * Save WiFi credentials to NVS
 */
void wifi_credentials_save(const String& ssid, const String& password) {
    // ESP-IDF: Use NVS
    esp_err_t err = nvs_open(PREF_NAMESPACE, NVS_READWRITE, &wifi_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "[WiFi] ERROR: Failed to open NVS namespace for writing");
        return;
    }
    
    err = nvs_set_str(wifi_nvs_handle, PREF_KEY_SSID, ssid.c_str());
    if (err == ESP_OK) {
        err = nvs_set_str(wifi_nvs_handle, PREF_KEY_PASSWORD, password.c_str());
    }
    if (err == ESP_OK) {
        bool use_saved = true;
        err = nvs_set_blob(wifi_nvs_handle, PREF_KEY_USE_SAVED, &use_saved, sizeof(bool));
    }
    if (err == ESP_OK) {
        err = nvs_commit(wifi_nvs_handle);
    }
    
    nvs_close(wifi_nvs_handle);
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "[WiFi] Saved credentials for: %s", ssid.c_str());
    } else {
        ESP_LOGE(TAG, "[WiFi] Failed to save credentials: %s", esp_err_to_name(err));
    }
}
