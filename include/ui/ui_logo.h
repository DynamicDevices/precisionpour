/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * Shared Logo Component
 * 
 * Provides a single logo image object that can be reused across all screens
 * to minimize memory usage and ensure consistent appearance.
 */

#ifndef UI_LOGO_H
#define UI_LOGO_H

#include <lvgl.h>

/**
 * Create the shared logo object
 * Should be called once during initialization
 * @param parent Parent object (usually lv_scr_act() or a container)
 * @return Pointer to logo image object, or NULL on error
 */
lv_obj_t* ui_logo_create(lv_obj_t* parent);

/**
 * Get the existing logo object
 * @return Pointer to logo image object, or NULL if not created
 */
lv_obj_t* ui_logo_get_object();

/**
 * Clean up the logo object
 * Call this when transitioning away from screens that use the logo
 */
void ui_logo_cleanup();

#endif // UI_LOGO_H
