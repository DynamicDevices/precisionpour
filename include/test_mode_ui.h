/**
 * Test Mode UI
 * 
 * Comprehensive hardware testing interface for:
 * - Display (colors, pixels, text)
 * - Touchscreen (touch coordinates, calibration)
 * - Flow meter (future)
 * - RFID/NFC (future)
 */

#ifndef TEST_MODE_UI_H
#define TEST_MODE_UI_H

#include <lvgl.h>

void test_mode_init();
void test_mode_update();

#endif // TEST_MODE_UI_H
