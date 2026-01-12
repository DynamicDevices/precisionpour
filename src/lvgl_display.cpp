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

#ifdef ESP_PLATFORM
    // ESP-IDF framework: Use ESP-IDF SPI driver
    #include "esp_idf_compat.h"
    #include "esp_log.h"
    #include "driver/spi_master.h"
    #include "driver/gpio.h"
    #include <string.h>
    #define TAG "display"
    
    // ESP-IDF SPI handle
    static spi_device_handle_t spi_handle = NULL;
    
    // ILI9341 command constants
    #define ILI9341_SWRESET     0x01
    #define ILI9341_SLPOUT      0x11
    #define ILI9341_DISPLAYON   0x29
    #define ILI9341_CASET       0x2A
    #define ILI9341_PASET       0x2B
    #define ILI9341_RAMWR       0x2C
    #define ILI9341_MADCTL      0x36
    #define ILI9341_PIXFMT      0x3A
    
    // Helper function to send command
    static void ili9341_send_cmd(uint8_t cmd) {
        gpio_set_level((gpio_num_t)TFT_DC, 0);  // Command mode
        spi_transaction_t t = {};
        t.length = 8;
        t.tx_buffer = &cmd;
        t.flags = SPI_TRANS_USE_TXDATA;
        spi_device_transmit(spi_handle, &t);
    }
    
    // Helper function to send data
    static void ili9341_send_data(const uint8_t* data, size_t len) {
        gpio_set_level((gpio_num_t)TFT_DC, 1);  // Data mode
        spi_transaction_t t = {};
        t.length = len * 8;
        t.tx_buffer = data;
        if (len <= 4) {
            t.flags = SPI_TRANS_USE_TXDATA;
            memcpy(t.tx_data, data, len);
        }
        spi_device_transmit(spi_handle, &t);
    }
    
    // Helper function to send command with data
    static void ili9341_send_cmd_data(uint8_t cmd, const uint8_t* data, size_t len) {
        ili9341_send_cmd(cmd);
        if (data && len > 0) {
            ili9341_send_data(data, len);
        }
    }
    
    // Initialize ILI9341 display
    static void ili9341_init() {
        ESP_LOGI(TAG, "Initializing ILI9341 display...");
        
        // Configure SPI bus
        spi_bus_config_t bus_cfg = {};
        bus_cfg.mosi_io_num = TFT_MOSI;
        bus_cfg.miso_io_num = TFT_MISO;
        bus_cfg.sclk_io_num = TFT_SCLK;
        bus_cfg.quadwp_io_num = -1;
        bus_cfg.quadhd_io_num = -1;
        bus_cfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * 2;  // 2 bytes per pixel
        
        ESP_ERROR_CHECK(spi_bus_initialize(HSPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO));
        
        // Configure SPI device
        spi_device_interface_config_t dev_cfg = {};
        dev_cfg.clock_speed_hz = 27000000;  // 27 MHz
        dev_cfg.mode = 0;
        dev_cfg.spics_io_num = TFT_CS;
        dev_cfg.queue_size = 1;
        dev_cfg.flags = 0;
        dev_cfg.pre_cb = NULL;
        
        ESP_ERROR_CHECK(spi_bus_add_device(HSPI_HOST, &dev_cfg, &spi_handle));
        
        // Configure control pins
        gpio_set_direction((gpio_num_t)TFT_DC, GPIO_MODE_OUTPUT);
        gpio_set_direction((gpio_num_t)TFT_RST, GPIO_MODE_OUTPUT);
        gpio_set_direction((gpio_num_t)TFT_BL, GPIO_MODE_OUTPUT);
        
        // Reset display
        gpio_set_level((gpio_num_t)TFT_RST, 0);
        delay(10);
        gpio_set_level((gpio_num_t)TFT_RST, 1);
        delay(10);
        
        // Initialize ILI9341
        ili9341_send_cmd(ILI9341_SWRESET);
        delay(120);
        
        ili9341_send_cmd(ILI9341_SLPOUT);
        delay(120);
        
        // Memory Access Control (MADCTL) - rotation
        uint8_t madctl = 0x00;
        if (DISPLAY_ROTATION == 1) {
            madctl = 0x20 | 0x08;  // Landscape, BGR
        } else if (DISPLAY_ROTATION == 3) {
            madctl = 0x20 | 0x08 | 0x40 | 0x80;  // Landscape flipped, BGR
        } else {
            madctl = 0x08;  // Portrait, BGR
        }
        ili9341_send_cmd_data(ILI9341_MADCTL, &madctl, 1);
        
        // Pixel format: 16-bit color
        uint8_t pixfmt = 0x55;  // 16-bit/pixel
        ili9341_send_cmd_data(ILI9341_PIXFMT, &pixfmt, 1);
        
        ili9341_send_cmd(ILI9341_DISPLAYON);
        delay(10);
        
        // Clear screen (set window first, then send data)
        uint8_t data[4];
        
        // Column address set
        data[0] = 0x00;
        data[1] = 0x00;
        data[2] = ((DISPLAY_WIDTH - 1) >> 8) & 0xFF;
        data[3] = (DISPLAY_WIDTH - 1) & 0xFF;
        ili9341_send_cmd_data(ILI9341_CASET, data, 4);
        
        // Row address set
        data[0] = 0x00;
        data[1] = 0x00;
        data[2] = ((DISPLAY_HEIGHT - 1) >> 8) & 0xFF;
        data[3] = (DISPLAY_HEIGHT - 1) & 0xFF;
        ili9341_send_cmd_data(ILI9341_PASET, data, 4);
        
        // Write to RAM
        ili9341_send_cmd(ILI9341_RAMWR);
        uint16_t black = 0x0000;
        for (int i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; i++) {
            ili9341_send_data((uint8_t*)&black, 2);
        }
        
        // Enable backlight
        gpio_set_level((gpio_num_t)TFT_BL, 1);
        
        ESP_LOGI(TAG, "ILI9341 initialized");
    }
    
    // Forward declaration
    static void ili9341_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
    
    // Set display window for drawing
    static void ili9341_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
        uint8_t data[4];
        
        // Column address set
        data[0] = (x1 >> 8) & 0xFF;
        data[1] = x1 & 0xFF;
        data[2] = (x2 >> 8) & 0xFF;
        data[3] = x2 & 0xFF;
        ili9341_send_cmd_data(ILI9341_CASET, data, 4);
        
        // Row address set
        data[0] = (y1 >> 8) & 0xFF;
        data[1] = y1 & 0xFF;
        data[2] = (y2 >> 8) & 0xFF;
        data[3] = y2 & 0xFF;
        ili9341_send_cmd_data(ILI9341_PASET, data, 4);
        
        // Write to RAM
        ili9341_send_cmd(ILI9341_RAMWR);
    }
    
#else
    // Arduino framework: Use TFT_eSPI
    #include "User_Setup.h"
    #include <TFT_eSPI.h>
    
    // Forward declare TFT_eSPI
    extern TFT_eSPI tft;
#endif

// Display buffer
static lv_color_t buf1[LVGL_BUFFER_SIZE];
static lv_color_t buf2[LVGL_BUFFER_SIZE];

void lvgl_display_init() {
    #ifdef ESP_PLATFORM
        // ESP-IDF: Initialize ILI9341
        ili9341_init();
        
        // Initialize LVGL display driver
        lv_disp_draw_buf_t *draw_buf = (lv_disp_draw_buf_t *)malloc(sizeof(lv_disp_draw_buf_t));
        lv_disp_draw_buf_init(draw_buf, buf1, buf2, LVGL_BUFFER_SIZE);
        
        static lv_disp_drv_t disp_drv;
        lv_disp_drv_init(&disp_drv);
        disp_drv.hor_res = DISPLAY_WIDTH;
        disp_drv.ver_res = DISPLAY_HEIGHT;
        disp_drv.flush_cb = lvgl_display_flush;
        disp_drv.draw_buf = draw_buf;
        lv_disp_drv_register(&disp_drv);
        
        ESP_LOGI(TAG, "LVGL display initialized");
    #else
        // Arduino: Initialize TFT display
        tft.init();
        tft.setRotation(DISPLAY_ROTATION);
        tft.fillScreen(TFT_BLACK);
        
        // Initialize LVGL display driver
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
    #endif
}

void lvgl_display_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    
    #ifdef ESP_PLATFORM
        // ESP-IDF: Use SPI to send pixel data
        ili9341_set_window(area->x1, area->y1, area->x2, area->y2);
        
        // Send pixel data
        gpio_set_level((gpio_num_t)TFT_DC, 1);  // Data mode
        spi_transaction_t t = {};
        t.length = w * h * 16;  // 16 bits per pixel
        t.tx_buffer = color_p;
        spi_device_transmit(spi_handle, &t);
    #else
        // Arduino: Use TFT_eSPI
        tft.startWrite();
        tft.setAddrWindow(area->x1, area->y1, w, h);
        tft.pushColors((uint16_t *)color_p, w * h, true);
        tft.endWrite();
    #endif
    
    lv_disp_flush_ready(disp_drv);
}
