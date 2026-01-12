/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * WiFi Credentials Storage
 * 
 * Handles saving and loading WiFi credentials from NVS (ESP-IDF) or Preferences (Arduino)
 */

#ifndef WIFI_CREDENTIALS_H
#define WIFI_CREDENTIALS_H

#ifdef ESP_PLATFORM
    #include "esp_idf_compat.h"
    #include <string>
    #include <cstring>
#else
    #include <WiFi.h>
    #include <WString.h>
    typedef String String;
#endif

// Load saved WiFi credentials
bool wifi_credentials_load(String& ssid, String& password);

// Save WiFi credentials
void wifi_credentials_save(const String& ssid, const String& password);

#endif // WIFI_CREDENTIALS_H
