/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * Finished Screen
 * 
 * Screen displayed after pouring completes, showing final volume and cost.
 * Automatically times out and returns to QR code screen.
 * Uses base_screen for standard layout (logo, WiFi icon, data icon).
 */

#ifndef FINISHED_SCREEN_H
#define FINISHED_SCREEN_H

#include <lvgl.h>

/**
 * Initialize the finished screen
 * @param final_volume_ml Final volume in milliliters
 * @param final_cost Final cost
 * @param currency Currency symbol (e.g., "GBP ", "$")
 */
void finished_screen_init(float final_volume_ml, float final_cost, const char* currency);

/**
 * Update the finished screen
 * Call this periodically to handle timeout countdown
 * @return true if timeout has elapsed (should transition to QR code screen)
 */
bool finished_screen_update();

/**
 * Clean up the finished screen
 */
void finished_screen_cleanup();

#endif // FINISHED_SCREEN_H
