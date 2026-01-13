/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * Screen Manager
 * 
 * Centralized screen state management and transitions.
 * Handles the UX flow: Splash → QR Code → Pouring → Finished → QR Code
 */

#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include <lvgl.h>

/**
 * Screen states
 */
enum ScreenState {
    SCREEN_SPLASH,      // Startup splashscreen
    SCREEN_QR_CODE,     // QR code / payment waiting
    SCREEN_POURING,     // Active pouring
    SCREEN_FINISHED     // Pouring complete
};

/**
 * Initialize the screen manager
 * Should be called once during startup
 */
void screen_manager_init();

/**
 * Get current screen state
 * @return Current screen state
 */
ScreenState screen_manager_get_state();

/**
 * Transition to QR code screen
 */
void screen_manager_show_qr_code();

/**
 * Transition to pouring screen
 * @param unique_id Unique ID for this pour
 * @param cost_per_ml Cost per milliliter
 * @param max_ml Maximum milliliters allowed
 * @param currency Currency symbol
 */
void screen_manager_show_pouring(const char* unique_id, float cost_per_ml, int max_ml, const char* currency);

/**
 * Transition to finished screen
 * @param final_volume_ml Final volume in milliliters
 * @param final_cost Final cost
 * @param currency Currency symbol
 */
void screen_manager_show_finished(float final_volume_ml, float final_cost, const char* currency);

/**
 * Update the screen manager
 * Call this periodically from main loop
 * Handles screen updates and timeout transitions
 */
void screen_manager_update();

/**
 * Clean up the screen manager
 */
void screen_manager_cleanup();

#endif // SCREEN_MANAGER_H
