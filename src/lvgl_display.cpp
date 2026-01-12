/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * LVGL Display Driver Implementation
 * Compatible with LVGL v8.x
 */

#include "lvgl_display.h"
#include "config.h"

// Include User_Setup.h before TFT_eSPI.h
#include "User_Setup.h"
#include <TFT_eSPI.h>

// Forward declare TFT_eSPI - will be defined after User_Setup.h is processed
extern TFT_eSPI tft;

// Display buffer
static lv_color_t buf1[LVGL_BUFFER_SIZE];
static lv_color_t buf2[LVGL_BUFFER_SIZE];

void lvgl_display_init() {
    // Initialize TFT display
    tft.init();
    tft.setRotation(DISPLAY_ROTATION);
    tft.fillScreen(TFT_BLACK);
    
    // Initialize LVGL display driver (LVGL v8 API)
    lv_disp_draw_buf_t *draw_buf = (lv_disp_draw_buf_t *)malloc(sizeof(lv_disp_draw_buf_t));
    lv_disp_draw_buf_init(draw_buf, buf1, buf2, LVGL_BUFFER_SIZE);
    
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = DISPLAY_WIDTH;
    disp_drv.ver_res = DISPLAY_HEIGHT;
    disp_drv.flush_cb = lvgl_display_flush;
    disp_drv.draw_buf = draw_buf;
    lv_disp_drv_register(&disp_drv);
    
    Serial.println("LVGL display initialized");
}

void lvgl_display_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)color_p, w * h, true);
    tft.endWrite();
    
    lv_disp_flush_ready(disp_drv);
}
