/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * LVGL Touch Driver Implementation
 * Direct SPI communication with XPT2046 using dedicated SPI bus
 * Touch screen uses separate SPI pins: GPIO25(SCLK), GPIO32(MOSI), GPIO39(MISO)
 */

// Project headers
#include "config.h"
#include "display/lvgl_touch.h"
#include "system/esp_idf_compat.h"  // For gpio_isr_handler_t

// System/Standard library headers
// ESP-IDF framework headers
#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <esp_rom_sys.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#define TAG "touch"

// GPIO level constants
#define HIGH 1
#define LOW 0

// Arduino map() function compatibility
static inline long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Touch screen uses its own SPI bus (separate from LCD SPI)
// LCD SPI: GPIO14(SCLK), GPIO13(MOSI), GPIO12(MISO)
// Touch SPI: GPIO25(SCLK), GPIO32(MOSI), GPIO39(MISO)
// We'll use VSPI for LCD (default) and create a custom SPI for touch

// XPT2046 command bytes
#define XPT2046_CMD_X     0x90  // Read X position (DIN = 0b10010000)
#define XPT2046_CMD_Y     0xD0  // Read Y position (DIN = 0b11010000)
#define XPT2046_CMD_Z1    0xB0  // Read Z1 position (DIN = 0b10110000)
#define XPT2046_CMD_Z2    0xC0  // Read Z2 position (DIN = 0b11000000)

// Touch calibration values - map raw XPT2046 coordinates (0-4095) to display pixels
#define TOUCH_X_MIN   100
#define TOUCH_X_MAX   4000
#define TOUCH_Y_MIN   100
#define TOUCH_Y_MAX   4000
#define TOUCH_PRESSURE_THRESHOLD 50  // Pressure threshold for touch detection

// Touch state
static bool touch_pressed = false;
static int16_t touch_x = 0;
static int16_t touch_y = 0;

// IRQ pin monitoring
static volatile bool irq_triggered = false;
static volatile uint64_t last_irq_time = 0;
static int last_irq_state = -1;
static const uint64_t IRQ_DEBOUNCE_MS = 50;  // Debounce time: ignore interrupts within 50ms

// IRQ interrupt handler
void IRAM_ATTR irq_handler(void* arg) {
    uint64_t now = esp_timer_get_time() / 1000ULL;
    // Debounce: only set flag if enough time has passed since last interrupt
    if (now - last_irq_time > IRQ_DEBOUNCE_MS) {
        irq_triggered = true;
        last_irq_time = now;
    }
}

// Wrapper function that matches voidFuncPtr signature (void (*)(void))
// This is called by gpio_isr_handler_wrapper
static void IRAM_ATTR irq_handler_wrapper_void(void) {
    irq_handler(NULL);
}

/**
 * Read a single coordinate from XPT2046 via SPI
 * Returns 12-bit value (0-4095)
 * Uses touch screen's dedicated SPI bus: GPIO25(SCLK), GPIO32(MOSI), GPIO39(MISO)
 */
static uint16_t xpt2046_read(uint8_t command) {
    uint16_t data = 0;
    
    // Touch screen uses its own SPI bus with custom pins
    // Bit-bang SPI for touch screen (since it uses different pins than LCD SPI)
    gpio_set_level((gpio_num_t)TOUCH_CS, 0);
    esp_rom_delay_us(1);
    
    // Send command byte (MSB first, SPI mode 0: CPOL=0, CPHA=0)
    uint8_t cmd = command;
    for (int i = 7; i >= 0; i--) {
        gpio_set_level((gpio_num_t)TOUCH_SCLK, 0);
        gpio_set_level((gpio_num_t)TOUCH_MOSI, (cmd >> i) & 0x01);
        esp_rom_delay_us(1);
        gpio_set_level((gpio_num_t)TOUCH_SCLK, 1);
        esp_rom_delay_us(1);
    }
    
    // Read 12-bit data (2 bytes, MSB first)
    uint8_t high_byte = 0;
    uint8_t low_byte = 0;
    
    // Read high byte
    for (int i = 7; i >= 0; i--) {
        gpio_set_level((gpio_num_t)TOUCH_SCLK, 0);
        esp_rom_delay_us(1);
        gpio_set_level((gpio_num_t)TOUCH_SCLK, 1);
        if (gpio_get_level((gpio_num_t)TOUCH_MISO)) {
            high_byte |= (1 << i);
        }
        esp_rom_delay_us(1);
    }
    
    // Read low byte
    for (int i = 7; i >= 0; i--) {
        gpio_set_level((gpio_num_t)TOUCH_SCLK, 0);
        esp_rom_delay_us(1);
        gpio_set_level((gpio_num_t)TOUCH_SCLK, 1);
        if (gpio_get_level((gpio_num_t)TOUCH_MISO)) {
            low_byte |= (1 << i);
        }
        esp_rom_delay_us(1);
    }
    
    gpio_set_level((gpio_num_t)TOUCH_CS, 1);
    
    // Combine bytes: XPT2046 returns 12-bit data
    data = ((high_byte << 8) | low_byte) >> 4;
    
    return data;
}

/**
 * Check if touch is pressed by reading pressure (Z1 and Z2)
 */
static bool xpt2046_is_pressed() {
    uint16_t z1 = xpt2046_read(XPT2046_CMD_Z1);
    uint16_t z2 = xpt2046_read(XPT2046_CMD_Z2);
    
    uint16_t pressure = 0;
    if (z1 > 0 && z2 < 4095) {
        pressure = z1 + (4095 - z2);
    }
    
    bool pressed = (pressure > TOUCH_PRESSURE_THRESHOLD) && 
                   (z1 > 50) && (z1 < 4000) && 
                   (z2 > 50) && (z2 < 4000);
    
    return pressed;
}

/**
 * Read touch coordinates and convert to display coordinates
 */
static void xpt2046_read_coords(int16_t *x, int16_t *y) {
    uint16_t raw_x = xpt2046_read(XPT2046_CMD_X);
    uint16_t raw_y = xpt2046_read(XPT2046_CMD_Y);
    
    int16_t display_x, display_y;
    
    // Map raw coordinates to display coordinates based on rotation
    switch (DISPLAY_ROTATION) {
        case 0: // Portrait
            display_x = map(raw_y, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, DISPLAY_WIDTH);
            display_y = map(raw_x, TOUCH_X_MIN, TOUCH_X_MAX, 0, DISPLAY_HEIGHT);
            break;
        case 1: // Landscape (USB right)
            display_x = map(raw_x, TOUCH_X_MIN, TOUCH_X_MAX, 0, DISPLAY_WIDTH);
            display_y = map(raw_y, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, DISPLAY_HEIGHT);
            break;
        case 2: // Portrait inverted
            display_x = map(raw_y, TOUCH_Y_MIN, TOUCH_Y_MAX, DISPLAY_WIDTH, 0);
            display_y = map(raw_x, TOUCH_X_MIN, TOUCH_X_MAX, DISPLAY_HEIGHT, 0);
            break;
        case 3: // Landscape (USB left)
            display_x = map(raw_x, TOUCH_X_MIN, TOUCH_X_MAX, DISPLAY_WIDTH, 0);
            display_y = map(raw_y, TOUCH_Y_MIN, TOUCH_Y_MAX, DISPLAY_HEIGHT, 0);
            break;
        default:
            display_x = map(raw_x, TOUCH_X_MIN, TOUCH_X_MAX, 0, DISPLAY_WIDTH);
            display_y = map(raw_y, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, DISPLAY_HEIGHT);
            break;
    }
    
    // Clamp to display bounds
    if (display_x < 0) display_x = 0;
    if (display_x >= DISPLAY_WIDTH) display_x = DISPLAY_WIDTH - 1;
    if (display_y < 0) display_y = 0;
    if (display_y >= DISPLAY_HEIGHT) display_y = DISPLAY_HEIGHT - 1;
    
    *x = display_x;
    *y = display_y;
}

void lvgl_touch_init() {
    ESP_LOGI(TAG, "[Touch] Initializing touch controller...");
    
    // Configure CS pin
    gpio_config_t cs_conf = {};
    cs_conf.pin_bit_mask = (1ULL << TOUCH_CS);
    cs_conf.mode = GPIO_MODE_OUTPUT;
    cs_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    cs_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    cs_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&cs_conf);
    gpio_set_level((gpio_num_t)TOUCH_CS, 1);
    ESP_LOGI(TAG, "[Touch] CS pin configured: GPIO%d", TOUCH_CS);
    
    // Initialize touch SPI pins (separate SPI bus: GPIO25, GPIO32, GPIO39)
    gpio_config_t sclk_conf = {};
    sclk_conf.pin_bit_mask = (1ULL << TOUCH_SCLK);
    sclk_conf.mode = GPIO_MODE_OUTPUT;
    sclk_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    sclk_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    sclk_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&sclk_conf);
    
    gpio_config_t mosi_conf = {};
    mosi_conf.pin_bit_mask = (1ULL << TOUCH_MOSI);
    mosi_conf.mode = GPIO_MODE_OUTPUT;
    mosi_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    mosi_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    mosi_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&mosi_conf);
    
    gpio_config_t miso_conf = {};
    miso_conf.pin_bit_mask = (1ULL << TOUCH_MISO);
    miso_conf.mode = GPIO_MODE_INPUT;
    miso_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    miso_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    miso_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&miso_conf);
    
    gpio_set_level((gpio_num_t)TOUCH_SCLK, 1);  // Idle high for SPI mode 0
    ESP_LOGI(TAG, "[Touch] SPI pins configured: SCLK=GPIO%d, MOSI=GPIO%d, MISO=GPIO%d", 
              TOUCH_SCLK, TOUCH_MOSI, TOUCH_MISO);
    
    // Configure IRQ pin
    if (TOUCH_IRQ >= 0) {
        // Check if pin is input-only (GPIO34, GPIO35, GPIO36, GPIO39 on ESP32)
        bool is_input_only = (TOUCH_IRQ == 34 || TOUCH_IRQ == 35 || TOUCH_IRQ == 36 || TOUCH_IRQ == 39);
        
        gpio_config_t irq_conf = {};
        irq_conf.pin_bit_mask = (1ULL << TOUCH_IRQ);
        irq_conf.mode = GPIO_MODE_INPUT;
        if (is_input_only) {
            irq_conf.pull_up_en = GPIO_PULLUP_DISABLE;
            irq_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        } else {
            irq_conf.pull_up_en = GPIO_PULLUP_ENABLE;
            irq_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        }
        irq_conf.intr_type = GPIO_INTR_NEGEDGE;  // FALLING edge
        gpio_config(&irq_conf);
        
        last_irq_state = gpio_get_level((gpio_num_t)TOUCH_IRQ);
        
        // Store handler in gpio_isr_handlers for the wrapper to call
        extern gpio_isr_handler_t gpio_isr_handlers[];
        extern void IRAM_ATTR gpio_isr_handler_wrapper(void* arg);
        
        // Store the wrapper in gpio_isr_handlers so gpio_isr_handler_wrapper can call it
        if (TOUCH_IRQ < GPIO_NUM_MAX) {
            gpio_isr_handlers[TOUCH_IRQ].pin = (gpio_num_t)TOUCH_IRQ;
            gpio_isr_handlers[TOUCH_IRQ].func = irq_handler_wrapper_void;
            gpio_isr_handlers[TOUCH_IRQ].type = GPIO_INTR_NEGEDGE;
        }
        
        // Add ISR handler - use the wrapper function
        gpio_isr_handler_add((gpio_num_t)TOUCH_IRQ, gpio_isr_handler_wrapper, (void*)(intptr_t)TOUCH_IRQ);
        
        ESP_LOGI(TAG, "[Touch] IRQ pin configured: GPIO%d (initial state: %s, FALLING edge)", 
                  TOUCH_IRQ, last_irq_state == 0 ? "LOW (pressed)" : "HIGH (not pressed)");
    } else {
        ESP_LOGW(TAG, "[Touch] WARNING: No IRQ pin configured!");
    }
    
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // Test touch controller by reading initial values
    uint16_t test_x = xpt2046_read(XPT2046_CMD_X);
    uint16_t test_y = xpt2046_read(XPT2046_CMD_Y);
    uint16_t test_z1 = xpt2046_read(XPT2046_CMD_Z1);
    uint16_t test_z2 = xpt2046_read(XPT2046_CMD_Z2);
    ESP_LOGI(TAG, "[Touch] Initial read test: X=%d Y=%d Z1=%d Z2=%d", test_x, test_y, test_z1, test_z2);
    
    // Register LVGL touch input device
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lvgl_touch_read;
    lv_indev_t *indev = lv_indev_drv_register(&indev_drv);
    
    if (indev != NULL) {
        ESP_LOGI(TAG, "[Touch] Touch controller initialized and registered with LVGL");
    } else {
        ESP_LOGE(TAG, "[Touch] ERROR: Failed to register touch input device!");
    }
}

void lvgl_touch_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
    static bool last_pressed = false;
    static uint64_t last_log_time = 0;
    static unsigned long touch_count = 0;
    
    bool pressed = false;
    bool irq_pressed = false;
    bool pressure_pressed = false;
    
    // Check IRQ pin
    if (TOUCH_IRQ >= 0) {
        int irq_state = gpio_get_level((gpio_num_t)TOUCH_IRQ);
        irq_pressed = (irq_state == 0);
        
        // Update IRQ state (don't log state changes - too noisy, especially during BLE activity)
        if (irq_state != last_irq_state) {
            last_irq_state = irq_state;
        }
        
        // Clear interrupt flag - only process if pressure also indicates a press
        // This filters out electrical noise from BLE radio activity
        if (irq_triggered) {
            irq_triggered = false;
            // Don't log IRQ triggers - they're too noisy, especially during BLE provisioning
            // Only process if we also have pressure reading indicating actual touch
        }
    }
    
    // Check pressure
    pressure_pressed = xpt2046_is_pressed();
    
    // Determine if touch is pressed
    // Require BOTH IRQ and pressure for more reliable detection (reduces false positives from BLE noise)
    // If IRQ is triggered but no pressure, it's likely electrical noise from BLE radio activity
    pressed = irq_pressed && pressure_pressed;
    
    // Fallback: if pressure is very strong, accept it even without IRQ (for reliability)
    if (!pressed && pressure_pressed) {
        // Check if pressure is very strong (definitely a real touch)
        uint16_t z1 = xpt2046_read(XPT2046_CMD_Z1);
        uint16_t z2 = xpt2046_read(XPT2046_CMD_Z2);
        uint16_t pressure = 0;
        if (z1 > 0 && z2 < 4095) {
            pressure = z1 + (4095 - z2);
        }
        // If pressure is very strong (>200), accept it as real touch even without IRQ
        if (pressure > 200) {
            pressed = true;
        }
    }
    
    if (pressed) {
        int16_t x, y;
        xpt2046_read_coords(&x, &y);
        
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
        
        touch_x = x;
        touch_y = y;
        touch_pressed = true;
        
        // Log touch detection (throttled to avoid spam)
        uint64_t now = esp_timer_get_time() / 1000ULL;
        if (now - last_log_time > 200 || !last_pressed) {  // Log every 200ms or on state change
            ESP_LOGI(TAG, "[Touch] Pressed: X=%d Y=%d (IRQ=%d, Pressure=%d)", 
                     x, y, irq_pressed, pressure_pressed);
            last_log_time = now;
            touch_count++;
        }
    } else {
        data->point.x = touch_x;
        data->point.y = touch_y;
        data->state = LV_INDEV_STATE_RELEASED;
        
        // Log release (only on state change)
        if (last_pressed) {
                ESP_LOGI(TAG, "[Touch] Released");
        }
        touch_pressed = false;
    }
    
    last_pressed = pressed;
}

void update_touch_state(int16_t x, int16_t y, bool pressed) {
    touch_x = x;
    touch_y = y;
    touch_pressed = pressed;
}

bool get_touch_state(int16_t *x, int16_t *y) {
    if (x) *x = touch_x;
    if (y) *y = touch_y;
    return touch_pressed;
}
