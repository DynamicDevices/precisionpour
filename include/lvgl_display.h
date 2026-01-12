/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * LVGL Display Driver Integration
 * 
 * Integrates TFT_eSPI display with LVGL graphics library.
 */

#ifndef LVGL_DISPLAY_H
#define LVGL_DISPLAY_H

#include "config.h"
#include <lvgl.h>

// Display buffer size (adjust based on available RAM)
// Using 1/10 of screen size for double buffering
#define LVGL_BUFFER_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT / 10)

// Display object (forward declaration)
class TFT_eSPI;
extern TFT_eSPI tft;

/**
 * Initialize LVGL display driver
 * Call this after initializing the TFT display
 */
void lvgl_display_init();

/**
 * LVGL display flush callback
 * Called by LVGL to update the display (LVGL v8 API)
 */
void lvgl_display_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p);

#endif // LVGL_DISPLAY_H
