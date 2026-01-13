/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * Base Screen Layout Component
 * 
 * Provides a standardized screen layout with:
 * - Logo at the top (shared component)
 * - Content area in the middle (for screen-specific content)
 * - WiFi icon at bottom left (shared component)
 * - Data icon at bottom right (shared component)
 * 
 * All screens should use this component to ensure consistent layout
 * and minimize memory usage through shared UI elements.
 */

#ifndef BASE_SCREEN_H
#define BASE_SCREEN_H

#include <lvgl.h>

/**
 * Layout constants
 */
#define BASE_SCREEN_LOGO_HEIGHT 60  // Reduced from 70px to match logo container
#define BASE_SCREEN_CONTENT_Y 70    // Content area starts at y=70 (logo container ends at y=70)
#define BASE_SCREEN_ICON_SIZE 20
#define BASE_SCREEN_ICON_MARGIN 5

/**
 * Create the base screen layout
 * This creates the standard layout with logo, WiFi icon, and data icon
 * @param parent Parent object (usually lv_scr_act())
 * @return Pointer to content area container, or NULL on error
 *         Screen-specific content should be added to this container
 */
lv_obj_t* base_screen_create(lv_obj_t* parent);

/**
 * Get the content area container
 * Use this to add screen-specific content
 * @return Pointer to content area container, or NULL if not created
 */
lv_obj_t* base_screen_get_content_area();

/**
 * Update the base screen (WiFi and data icons)
 * Call this periodically from the screen's update function
 */
void base_screen_update();

/**
 * Clean up the base screen
 * Note: Shared components (logo, WiFi icon, data icon) persist across screens
 */
void base_screen_cleanup();

#endif // BASE_SCREEN_H
