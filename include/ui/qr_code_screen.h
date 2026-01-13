/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * QR Code Screen
 * 
 * Displays the QR code for payment, waiting for customer to scan and pay.
 * Uses base_screen for standard layout (logo, WiFi icon, data icon).
 */

#ifndef QR_CODE_SCREEN_H
#define QR_CODE_SCREEN_H

#include <lvgl.h>

/**
 * Initialize the QR code screen
 * Creates the QR code with device-specific URL
 */
void qr_code_screen_init();

/**
 * Update the QR code screen
 * Call this periodically to update status icons
 */
void qr_code_screen_update();

/**
 * Clean up the QR code screen
 */
void qr_code_screen_cleanup();

#endif // QR_CODE_SCREEN_H
