/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * Shared Data Icon Component
 * 
 * Provides a single MQTT/data connection status icon that can be reused across all screens
 * Shows MQTT connection status and activity.
 */

#ifndef UI_DATA_ICON_H
#define UI_DATA_ICON_H

#include <lvgl.h>

/**
 * Create the shared data icon
 * Should be called once during initialization
 * @param parent Parent object (usually lv_scr_act() or a container)
 * @return Pointer to data icon container, or NULL on error
 */
lv_obj_t* ui_data_icon_create(lv_obj_t* parent);

/**
 * Get the existing data icon container
 * @return Pointer to data icon container, or NULL if not created
 */
lv_obj_t* ui_data_icon_get_container();

/**
 * Update data icon status
 * Call this periodically to update the icon based on current MQTT status
 * @param connected MQTT connection status
 * @param active Whether there's recent MQTT activity
 */
void ui_data_icon_update(bool connected, bool active);

/**
 * Set activity state (for animation)
 * @param active True to show activity animation, false to show static state
 */
void ui_data_icon_set_active(bool active);

/**
 * Clean up the data icon
 */
void ui_data_icon_cleanup();

#endif // UI_DATA_ICON_H
