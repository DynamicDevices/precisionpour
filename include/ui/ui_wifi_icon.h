/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * Shared WiFi Icon Component
 * 
 * Provides a single WiFi status icon that can be reused across all screens
 * Shows WiFi connection status and signal strength.
 */

#ifndef UI_WIFI_ICON_H
#define UI_WIFI_ICON_H

#include <lvgl.h>

/**
 * Create the shared WiFi icon
 * Should be called once during initialization
 * @param parent Parent object (usually lv_scr_act() or a container)
 * @return Pointer to WiFi icon container, or NULL on error
 */
lv_obj_t* ui_wifi_icon_create(lv_obj_t* parent);

/**
 * Get the existing WiFi icon container
 * @return Pointer to WiFi icon container, or NULL if not created
 */
lv_obj_t* ui_wifi_icon_get_container();

/**
 * Update WiFi icon status
 * Call this periodically to update the icon based on current WiFi status
 * @param connected WiFi connection status
 * @param rssi WiFi signal strength (RSSI in dBm)
 * @param flashing Whether to flash the icon (e.g., WiFi connected but MQTT not)
 */
void ui_wifi_icon_update(bool connected, int rssi, bool flashing);

/**
 * Set flashing state
 * @param flashing True to enable flashing, false to disable
 */
void ui_wifi_icon_set_flashing(bool flashing);

/**
 * Clean up the WiFi icon
 */
void ui_wifi_icon_cleanup();

#endif // UI_WIFI_ICON_H
