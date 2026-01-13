/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * Pouring Screen
 * 
 * Screen displayed during active pouring, showing flow rate, volume, and cost.
 * Uses base_screen for standard layout (logo, WiFi icon, data icon).
 */

#ifndef POURING_SCREEN_H
#define POURING_SCREEN_H

#include <lvgl.h>

/**
 * Initialize the pouring screen
 */
void pouring_screen_init();

/**
 * Update the pouring screen
 * Call this periodically to update flow rate, volume, and cost
 */
void pouring_screen_update();

/**
 * Reset volume and cost for new pour
 */
void pouring_screen_reset();

/**
 * Set pour parameters from MQTT "paid" command
 * @param unique_id Unique ID for this pour
 * @param cost_per_ml Cost per milliliter
 * @param max_ml Maximum milliliters allowed
 * @param currency Currency symbol (e.g., "GBP ", "$")
 */
void pouring_screen_set_params(const char* unique_id, float cost_per_ml, int max_ml, const char* currency);

/**
 * Start pouring with paid parameters
 * @param unique_id Unique ID for this pour
 * @param cost_per_ml Cost per milliliter
 * @param max_ml Maximum milliliters allowed
 * @param currency Currency symbol
 */
void pouring_screen_start_pour(const char* unique_id, float cost_per_ml, int max_ml, const char* currency);

/**
 * Check if maximum volume has been reached
 * @return true if max ml reached, false otherwise
 */
bool pouring_screen_is_max_reached();

/**
 * Set callback function to switch back to QR code screen
 * Called when screen is tapped
 * @param callback Function to call when switching screens
 */
void pouring_screen_set_switch_callback(void (*callback)(void));

/**
 * Get current pour cost per ml
 * @return Cost per milliliter
 */
float pouring_screen_get_cost_per_ml();

/**
 * Clean up the pouring screen
 */
void pouring_screen_cleanup();

#endif // POURING_SCREEN_H
