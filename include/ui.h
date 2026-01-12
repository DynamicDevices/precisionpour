/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * UI Setup and Management
 * 
 * Creates and manages the LVGL user interface.
 */

#ifndef UI_H
#define UI_H

#include <lvgl.h>

/**
 * Initialize and create the UI
 */
void ui_init();

/**
 * Update UI (call periodically if needed)
 */
void ui_update();

#endif // UI_H
