/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * Splashscreen Management
 * 
 * Handles display of splashscreen image during startup with progress bar.
 */

#ifndef SPLASHSCREEN_H
#define SPLASHSCREEN_H

#include <lvgl.h>

/**
 * Initialize and display splashscreen with progress bar
 * Call this early in setup() before other UI initialization
 */
void splashscreen_init();

/**
 * Update progress bar (0-100)
 * @param percent Progress percentage (0-100)
 */
void splashscreen_set_progress(uint8_t percent);

/**
 * Set progress bar text (optional status message)
 * @param text Status text to display
 */
void splashscreen_set_status(const char *text);

/**
 * Remove splashscreen and show main UI
 * Call this when ready to show the main interface
 */
void splashscreen_remove();

/**
 * Check if splashscreen is currently displayed
 */
bool splashscreen_is_active();

#endif // SPLASHSCREEN_H
