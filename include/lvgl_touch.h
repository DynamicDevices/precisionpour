/**
 * LVGL Touch Driver Integration
 * 
 * Integrates touch controller with LVGL input handling.
 */

#ifndef LVGL_TOUCH_H
#define LVGL_TOUCH_H

#include <lvgl.h>
#include "config.h"

/**
 * Initialize LVGL touch input
 * Call this after initializing the touch controller
 */
void lvgl_touch_init();

/**
 * LVGL touch read callback
 * Called by LVGL to read touch input (LVGL v8 API)
 */
void lvgl_touch_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);

/**
 * Update touch state (call from touch ISR or main loop)
 * Use this helper function if reading touch in your main loop
 */
void update_touch_state(int16_t x, int16_t y, bool pressed);

/**
 * Get current touch state (for test mode)
 * Returns true if touch is currently pressed
 */
bool get_touch_state(int16_t *x, int16_t *y);

#endif // LVGL_TOUCH_H
