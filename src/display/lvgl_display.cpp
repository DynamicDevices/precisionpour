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
#include "display/lvgl_display.h"

// System/Standard library headers
// ESP-IDF framework headers
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>
#define TAG "display"

// SPI clock for the display. 40MHz is typically fine for ESP32 + ILI9341 on short wires.
// If you see artifacts or instability, drop to e.g. 32000000 or 27000000.
#ifndef TFT_SPI_CLOCK_HZ
#define TFT_SPI_CLOCK_HZ 40000000
#endif

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
        dev_cfg.clock_speed_hz = TFT_SPI_CLOCK_HZ;
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
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level((gpio_num_t)TFT_RST, 1);
        vTaskDelay(pdMS_TO_TICKS(120));  // Wait for display to stabilize after reset
        
        // Initialize ILI9341 with complete sequence
        ili9341_send_cmd(ILI9341_SWRESET);
        vTaskDelay(pdMS_TO_TICKS(120));
        
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
        // Bit 3 (0x08): BGR (0=RGB, 1=BGR) - controls color order
        // Bit 4 (0x10): ML (vertical refresh order)
        // Bit 5 (0x20): MV (row/column exchange)
        // Bit 6 (0x40): MX (column address order)
        // Bit 7 (0x80): MY (row address order)
        // 
        // Note: Display configured for BGR mode (bit 3 = 0x08)
        // Rotation settings (for reference, these match previous TFT_eSPI configuration):
        // Rotation 0: MX=1, BGR=1 -> 0x40 | 0x08 = 0x48
        // Rotation 1: MV=1, BGR=1 -> 0x20 | 0x08 = 0x28
        // Rotation 2: MY=1, BGR=1 -> 0x80 | 0x08 = 0x88
        // Rotation 3: MX=1, MY=1, MV=1, BGR=1 -> 0x40 | 0x80 | 0x20 | 0x08 = 0xE8
        uint8_t madctl = 0x00;
        if (DISPLAY_ROTATION == 1) {
            // Landscape: MV=1, BGR=1
            madctl = 0x20 | 0x08;  // MV=1, BGR=1 (bit 3)
        } else if (DISPLAY_ROTATION == 3) {
            // Landscape flipped: MX=1, MY=1, MV=1, BGR=1
            madctl = 0x40 | 0x80 | 0x20 | 0x08;  // MX, MY, MV, BGR=1
        } else {
            // Portrait: MV=1, BGR=1
            madctl = 0x20 | 0x08;  // MV=1, BGR=1 (bit 3)
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
        vTaskDelay(pdMS_TO_TICKS(120));
        
        // Display on
        ili9341_send_cmd(ILI9341_DISPLAYON);
        vTaskDelay(pdMS_TO_TICKS(10));
        
        // Enable backlight
        gpio_set_level((gpio_num_t)TFT_BL, 1);
        
        // Note: Screen clearing is handled by LVGL when it first renders
        // No need to manually clear the screen here
        
        ESP_LOGI(TAG, "ILI9341 initialized");
    }

// Display buffer
static lv_color_t buf1[LVGL_BUFFER_SIZE];

void lvgl_display_init() {
    // ESP-IDF: Initialize ILI9341
    ili9341_init();
    
    // Initialize LVGL display driver
    lv_disp_draw_buf_t *draw_buf = (lv_disp_draw_buf_t *)malloc(sizeof(lv_disp_draw_buf_t));
    // Single buffer to reduce DRAM usage (double buffering costs ~2x LVGL_BUFFER_SIZE)
    lv_disp_draw_buf_init(draw_buf, buf1, NULL, LVGL_BUFFER_SIZE);
    
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = DISPLAY_WIDTH;
    disp_drv.ver_res = DISPLAY_HEIGHT;
    disp_drv.flush_cb = lvgl_display_flush;
    disp_drv.draw_buf = draw_buf;
    lv_disp_drv_register(&disp_drv);
    
    ESP_LOGI(TAG, "LVGL display initialized");
}

void lvgl_display_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    uint32_t pixel_count = w * h;
    
    // ESP-IDF: Use SPI to send pixel data
    static bool first_flush = true;
    if (first_flush) {
        ESP_LOGI(TAG, "First flush: area (%d,%d) to (%d,%d), %dx%d pixels", 
                 area->x1, area->y1, area->x2, area->y2, w, h);
        first_flush = false;
    }
    
    ili9341_set_window(area->x1, area->y1, area->x2, area->y2);
    
    // Send pixel data in chunks (SPI has max transfer size limits)
    // Chunked transfer (larger chunks reduce SPI transaction overhead).
    static constexpr size_t MAX_CHUNK_PIXELS = 4096; // 4096 px = 8192 bytes @ RGB565
    
    gpio_set_level((gpio_num_t)TFT_DC, 1);  // Data mode
    
    // LVGL provides RGB565 in native endian order; ILI9341 expects MSB-first over SPI,
    // so we byte-swap into a small reusable buffer (keeps colors correct).
    uint16_t *pixels = (uint16_t *)color_p;
    size_t remaining_pixels = pixel_count;
    size_t offset = 0;

    static uint16_t swapped_buffer[MAX_CHUNK_PIXELS];

    while (remaining_pixels > 0) {
        const size_t chunk_pixels = (remaining_pixels > MAX_CHUNK_PIXELS) ? MAX_CHUNK_PIXELS : remaining_pixels;

        // Fast byte-swap: operate on 32-bit words where possible (swaps bytes within each 16-bit pixel)
        size_t i = 0;

        // Align input to 4 bytes; keep output aligned too by consuming 2 pixels when needed.
        if ((((uintptr_t)(&pixels[offset]) & 0x3) != 0) && chunk_pixels >= 2) {
            const uint16_t p0 = pixels[offset + 0];
            const uint16_t p1 = pixels[offset + 1];
            swapped_buffer[0] = (uint16_t)(((p0 & 0x00FF) << 8) | ((p0 & 0xFF00) >> 8));
            swapped_buffer[1] = (uint16_t)(((p1 & 0x00FF) << 8) | ((p1 & 0xFF00) >> 8));
            i = 2;
        } else if ((((uintptr_t)(&pixels[offset]) & 0x3) != 0) && chunk_pixels == 1) {
            const uint16_t p0 = pixels[offset + 0];
            swapped_buffer[0] = (uint16_t)(((p0 & 0x00FF) << 8) | ((p0 & 0xFF00) >> 8));
            i = 1;
        }

        const size_t remaining_after_align = (chunk_pixels > i) ? (chunk_pixels - i) : 0;
        const size_t pairs = remaining_after_align / 2;

        if (pairs > 0 && (((uintptr_t)swapped_buffer & 0x3) == 0)) {
            const uint32_t *in32 = (const uint32_t *)(&pixels[offset + i]);
            uint32_t *out32 = (uint32_t *)(&swapped_buffer[i]);
            for (size_t j = 0; j < pairs; j++) {
                const uint32_t v = in32[j];
                out32[j] = ((v & 0x00FF00FFu) << 8) | ((v & 0xFF00FF00u) >> 8);
            }
            i += pairs * 2;
        }

        // Tail pixel (if odd count)
        for (; i < chunk_pixels; i++) {
            const uint16_t p = pixels[offset + i];
            swapped_buffer[i] = (uint16_t)(((p & 0x00FF) << 8) | ((p & 0xFF00) >> 8));
        }

        spi_transaction_t t = {};
        t.length = chunk_pixels * 2 * 8;  // Length in bits
        t.tx_buffer = swapped_buffer;
        t.flags = 0;  // No special flags
        
        esp_err_t ret = spi_device_transmit(spi_handle, &t);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "SPI transmit error: %s", esp_err_to_name(ret));
            break;
        }
        
        offset += chunk_pixels;
        remaining_pixels -= chunk_pixels;
    }
    
    lv_disp_flush_ready(disp_drv);
}
