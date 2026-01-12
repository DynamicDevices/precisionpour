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

// Project headers
#include "config.h"
#include "lvgl_display.h"

// System/Standard library headers
#ifdef ESP_PLATFORM
    // ESP-IDF framework headers
    #include <driver/gpio.h>
    #include <driver/spi_master.h>
    #include <esp_log.h>
    #include <string.h>
    #define TAG "display"
    
    // Project compatibility headers
    #include "esp_idf_compat.h"
    
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
    #define ILI9341_PWRCTL1     0xC0
    #define ILI9341_PWRCTL2     0xC1
    #define ILI9341_VMCTL1      0xC5
    #define ILI9341_VMCTL2      0xC7
    #define ILI9341_FRMCTR1     0xB1
    #define ILI9341_DFUNCTR     0xB6
    #define ILI9341_GMCTRP1     0xE0
    #define ILI9341_GMCTRN1     0xE1
    
    // Helper function to send command
    static void ili9341_send_cmd(uint8_t cmd) {
        gpio_set_level((gpio_num_t)TFT_DC, 0);  // Command mode
        spi_transaction_t t = {};
        t.length = 8;
        t.flags = SPI_TRANS_USE_TXDATA;
        t.tx_data[0] = cmd;
        esp_err_t ret = spi_device_transmit(spi_handle, &t);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "SPI command transmit error: %s", esp_err_to_name(ret));
        }
    }
    
    // Helper function to send data
    static void ili9341_send_data(const uint8_t* data, size_t len) {
        gpio_set_level((gpio_num_t)TFT_DC, 1);  // Data mode
        spi_transaction_t t = {};
        t.length = len * 8;
        if (len <= 4) {
            t.flags = SPI_TRANS_USE_TXDATA;
            memcpy(t.tx_data, data, len);
        } else {
            t.tx_buffer = data;
        }
        esp_err_t ret = spi_device_transmit(spi_handle, &t);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "SPI data transmit error: %s", esp_err_to_name(ret));
        }
    }
    
    // Helper function to send command with data
    static void ili9341_send_cmd_data(uint8_t cmd, const uint8_t* data, size_t len) {
        ili9341_send_cmd(cmd);
        if (data && len > 0) {
            ili9341_send_data(data, len);
        }
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
        
        // Use SPI2_HOST (HSPI) for ESP32
        ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO));
        
        // Configure SPI device
        spi_device_interface_config_t dev_cfg = {};
        dev_cfg.clock_speed_hz = 27000000;  // 27 MHz
        dev_cfg.mode = 0;
        dev_cfg.spics_io_num = TFT_CS;
        dev_cfg.queue_size = 1;
        dev_cfg.flags = 0;
        dev_cfg.pre_cb = NULL;
        
        ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev_cfg, &spi_handle));
        
        // Configure control pins
        gpio_set_direction((gpio_num_t)TFT_CS, GPIO_MODE_OUTPUT);
        gpio_set_direction((gpio_num_t)TFT_DC, GPIO_MODE_OUTPUT);
        gpio_set_direction((gpio_num_t)TFT_RST, GPIO_MODE_OUTPUT);
        gpio_set_direction((gpio_num_t)TFT_BL, GPIO_MODE_OUTPUT);
        
        // Initialize CS pin (HIGH = inactive, SPI driver will control it during transactions)
        gpio_set_level((gpio_num_t)TFT_CS, 1);
        
        // Initialize DC pin
        gpio_set_level((gpio_num_t)TFT_DC, 0);
        
        // Reset display
        gpio_set_level((gpio_num_t)TFT_RST, 0);
        delay(10);
        gpio_set_level((gpio_num_t)TFT_RST, 1);
        delay(120);  // Wait for display to stabilize after reset
        
        // Initialize ILI9341 with complete sequence
        ili9341_send_cmd(ILI9341_SWRESET);
        delay(120);
        
        // Power Control 1
        uint8_t pwr1_data[] = {0x23};
        ili9341_send_cmd_data(ILI9341_PWRCTL1, pwr1_data, 1);
        
        // Power Control 2
        uint8_t pwr2_data[] = {0x10};
        ili9341_send_cmd_data(ILI9341_PWRCTL2, pwr2_data, 1);
        
        // VCOM Control 1
        uint8_t vcom_data[] = {0x2E, 0x86};
        ili9341_send_cmd_data(ILI9341_VMCTL1, vcom_data, 2);
        
        // VCOM Control 2
        uint8_t vcom2_data[] = {0xC0};
        ili9341_send_cmd_data(ILI9341_VMCTL2, vcom2_data, 1);
        
        // Memory Access Control (MADCTL) - rotation and color order
        // ILI9341 MADCTL bits:
        // Bit 0 (0x01): MY (row address order)
        // Bit 1 (0x02): MX (column address order)
        // Bit 2 (0x04): MV (row/column exchange)
        // Bit 3 (0x08): ML (vertical refresh order)
        // Bit 4 (0x10): BGR (0=RGB, 1=BGR) - we use BGR (1) to match LVGL RGB565 output
        // Bit 5 (0x20): MH (horizontal refresh order)
        // Bit 6 (0x40): Reserved
        // Bit 7 (0x80): Reserved
        // 
        // Note: ILI9341 interprets data differently - when BGR=1, it expects BGR order
        // but LVGL outputs RGB565 in RGB order, so we need BGR=1 to swap the color channels
        // For landscape (rotation 1): MX=1, MH=1, BGR=1 (BGR mode)
        // For portrait (rotation 0/2): MV=1, BGR=1 (BGR mode)
        uint8_t madctl = 0x00;
        if (DISPLAY_ROTATION == 1) {
            // Landscape: MX=1, MH=1, BGR mode (BGR=1)
            madctl = 0x20 | 0x02 | 0x10;  // MH=1, MX=1, BGR=1
        } else if (DISPLAY_ROTATION == 3) {
            // Landscape flipped: MY=1, MX=1, MV=1, MH=1, BGR mode
            madctl = 0x80 | 0x20 | 0x02 | 0x01 | 0x10;  // MY, MH, MX, MV, BGR=1
        } else {
            // Portrait: MV=1, BGR mode
            madctl = 0x04 | 0x10;  // MV=1, BGR=1
        }
        ili9341_send_cmd_data(ILI9341_MADCTL, &madctl, 1);
        
        // Pixel format: 16-bit color
        uint8_t pixfmt = 0x55;  // 16-bit/pixel
        ili9341_send_cmd_data(ILI9341_PIXFMT, &pixfmt, 1);
        
        // Frame Rate Control
        uint8_t frctrl_data[] = {0x00, 0x18};
        ili9341_send_cmd_data(ILI9341_FRMCTR1, frctrl_data, 2);
        
        // Display Function Control
        uint8_t dfunc_data[] = {0x08, 0x82, 0x27};
        ili9341_send_cmd_data(ILI9341_DFUNCTR, dfunc_data, 3);
        
        // Gamma correction (positive)
        uint8_t gamma_p[] = {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 
                             0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00};
        ili9341_send_cmd_data(ILI9341_GMCTRP1, gamma_p, 15);
        
        // Gamma correction (negative)
        uint8_t gamma_n[] = {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 
                             0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F};
        ili9341_send_cmd_data(ILI9341_GMCTRN1, gamma_n, 15);
        
        // Sleep out
        ili9341_send_cmd(ILI9341_SLPOUT);
        delay(120);
        
        // Display on
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
        
        // Write to RAM - clear screen more efficiently
        ili9341_send_cmd(ILI9341_RAMWR);
        gpio_set_level((gpio_num_t)TFT_DC, 1);  // Data mode
        
        // Send black pixels in chunks
        uint16_t black = 0x0000;
        const size_t clear_chunk_size = 1024;  // 1024 pixels = 2048 bytes
        size_t total_pixels = DISPLAY_WIDTH * DISPLAY_HEIGHT;
        
        for (size_t i = 0; i < total_pixels; i += clear_chunk_size) {
            size_t chunk_pixels = (total_pixels - i > clear_chunk_size) ? clear_chunk_size : (total_pixels - i);
            size_t chunk_bytes = chunk_pixels * 2;
            
            // Create a buffer of black pixels
            uint16_t black_buffer[clear_chunk_size];
            for (size_t j = 0; j < chunk_pixels; j++) {
                black_buffer[j] = black;
            }
            
            spi_transaction_t t = {};
            t.length = chunk_bytes * 8;
            t.tx_buffer = black_buffer;
            t.flags = 0;
            spi_device_transmit(spi_handle, &t);
        }
        
        // Enable backlight
        gpio_set_level((gpio_num_t)TFT_BL, 1);
        
        ESP_LOGI(TAG, "ILI9341 initialized");
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
    uint32_t pixel_count = w * h;
    
    #ifdef ESP_PLATFORM
        // ESP-IDF: Use SPI to send pixel data
        static bool first_flush = true;
        if (first_flush) {
            ESP_LOGI(TAG, "First flush: area (%d,%d) to (%d,%d), %dx%d pixels", 
                     area->x1, area->y1, area->x2, area->y2, w, h);
            first_flush = false;
        }
        
        ili9341_set_window(area->x1, area->y1, area->x2, area->y2);
        
        // Send pixel data in chunks (SPI has max transfer size limits)
        // Max chunk size: 4092 bytes (ESP-IDF default, but we'll use 4096 for safety)
        const size_t max_chunk_bytes = 4096;
        const size_t max_chunk_pixels = max_chunk_bytes / 2;  // 2 bytes per pixel
        
        gpio_set_level((gpio_num_t)TFT_DC, 1);  // Data mode
        
        // LVGL outputs RGB565 in RGB order, display is configured for BGR mode (BGR bit set in MADCTL)
        // The BGR bit swaps the color channels to match LVGL's RGB565 output
        // No additional color swapping needed - send pixels directly
        uint16_t *pixels = (uint16_t *)color_p;
        size_t remaining_pixels = pixel_count;
        size_t offset = 0;
        
        while (remaining_pixels > 0) {
            size_t chunk_pixels = (remaining_pixels > max_chunk_pixels) ? max_chunk_pixels : remaining_pixels;
            size_t chunk_bytes = chunk_pixels * 2;
            
            spi_transaction_t t = {};
            t.length = chunk_bytes * 8;  // Length in bits
            t.tx_buffer = pixels + offset;
            t.flags = 0;  // No special flags
            
            esp_err_t ret = spi_device_transmit(spi_handle, &t);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "SPI transmit error: %s", esp_err_to_name(ret));
                break;
            }
            
            offset += chunk_pixels;
            remaining_pixels -= chunk_pixels;
        }
    #else
        // Arduino: Use TFT_eSPI
        tft.startWrite();
        tft.setAddrWindow(area->x1, area->y1, w, h);
        tft.pushColors((uint16_t *)color_p, w * h, true);
        tft.endWrite();
    #endif
    
    lv_disp_flush_ready(disp_drv);
}
