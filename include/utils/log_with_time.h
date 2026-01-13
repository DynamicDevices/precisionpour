/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Logging with Date/Time Support
 * 
 * Provides wrapper macros for ESP-IDF logging that include full date/time
 * information in the log message when NTP time is synchronized.
 */

#ifndef LOG_WITH_TIME_H
#define LOG_WITH_TIME_H

#include <esp_log.h>
#include <time.h>
#include <string.h>

// Helper function to get formatted date/time string
static inline const char* get_log_time_string(void) {
    static char time_str[32];
    time_t now = 0;
    struct tm timeinfo;
    
    time(&now);
    if (now > 0 && localtime_r(&now, &timeinfo) != NULL) {
        // Format: YYYY-MM-DD HH:MM:SS
        snprintf(time_str, sizeof(time_str), "%04d-%02d-%02d %02d:%02d:%02d",
                 timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        return time_str;
    }
    
    // Fallback: return empty string if time not synced
    return "";
}

// Wrapper macros that include date/time in the log message
#define ESP_LOGI_TIME(tag, format, ...) do { \
    const char* dt = get_log_time_string(); \
    if (strlen(dt) > 0) { \
        ESP_LOGI(tag, "[%s] " format, dt, ##__VA_ARGS__); \
    } else { \
        ESP_LOGI(tag, format, ##__VA_ARGS__); \
    } \
} while(0)

#define ESP_LOGW_TIME(tag, format, ...) do { \
    const char* dt = get_log_time_string(); \
    if (strlen(dt) > 0) { \
        ESP_LOGW(tag, "[%s] " format, dt, ##__VA_ARGS__); \
    } else { \
        ESP_LOGW(tag, format, ##__VA_ARGS__); \
    } \
} while(0)

#define ESP_LOGE_TIME(tag, format, ...) do { \
    const char* dt = get_log_time_string(); \
    if (strlen(dt) > 0) { \
        ESP_LOGE(tag, "[%s] " format, dt, ##__VA_ARGS__); \
    } else { \
        ESP_LOGE(tag, format, ##__VA_ARGS__); \
    } \
} while(0)

#define ESP_LOGD_TIME(tag, format, ...) do { \
    const char* dt = get_log_time_string(); \
    if (strlen(dt) > 0) { \
        ESP_LOGD(tag, "[%s] " format, dt, ##__VA_ARGS__); \
    } else { \
        ESP_LOGD(tag, format, ##__VA_ARGS__); \
    } \
} while(0)

#endif // LOG_WITH_TIME_H
