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

#include "lvgl_touch.h"
#include "config.h"
#include <SPI.h>
#include <Arduino.h>

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
static volatile unsigned long last_irq_time = 0;
static int last_irq_state = -1;
static const unsigned long IRQ_DEBOUNCE_MS = 50;  // Debounce time: ignore interrupts within 50ms

// IRQ interrupt handler
void IRAM_ATTR irq_handler() {
    unsigned long now = millis();
    // Debounce: only set flag if enough time has passed since last interrupt
    if (now - last_irq_time > IRQ_DEBOUNCE_MS) {
        irq_triggered = true;
        last_irq_time = now;
    }
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
    digitalWrite(TOUCH_CS, LOW);
    delayMicroseconds(1);
    
    // Send command byte (MSB first, SPI mode 0: CPOL=0, CPHA=0)
    uint8_t cmd = command;
    for (int i = 7; i >= 0; i--) {
        digitalWrite(TOUCH_SCLK, LOW);
        digitalWrite(TOUCH_MOSI, (cmd >> i) & 0x01);
        delayMicroseconds(1);
        digitalWrite(TOUCH_SCLK, HIGH);
        delayMicroseconds(1);
    }
    
    // Read 12-bit data (2 bytes, MSB first)
    uint8_t high_byte = 0;
    uint8_t low_byte = 0;
    
    // Read high byte
    for (int i = 7; i >= 0; i--) {
        digitalWrite(TOUCH_SCLK, LOW);
        delayMicroseconds(1);
        digitalWrite(TOUCH_SCLK, HIGH);
        if (digitalRead(TOUCH_MISO)) {
            high_byte |= (1 << i);
        }
        delayMicroseconds(1);
    }
    
    // Read low byte
    for (int i = 7; i >= 0; i--) {
        digitalWrite(TOUCH_SCLK, LOW);
        delayMicroseconds(1);
        digitalWrite(TOUCH_SCLK, HIGH);
        if (digitalRead(TOUCH_MISO)) {
            low_byte |= (1 << i);
        }
        delayMicroseconds(1);
    }
    
    digitalWrite(TOUCH_CS, HIGH);
    
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
    Serial.println("[Touch] Initializing touch controller...");
    
    // Configure CS pin
    pinMode(TOUCH_CS, OUTPUT);
    digitalWrite(TOUCH_CS, HIGH);
    Serial.printf("[Touch] CS pin configured: GPIO%d\r\n", TOUCH_CS);
    
    // Initialize touch SPI pins (separate SPI bus: GPIO25, GPIO32, GPIO39)
    pinMode(TOUCH_SCLK, OUTPUT);
    pinMode(TOUCH_MOSI, OUTPUT);
    pinMode(TOUCH_MISO, INPUT);
    digitalWrite(TOUCH_SCLK, HIGH);  // Idle high for SPI mode 0
    Serial.printf("[Touch] SPI pins configured: SCLK=GPIO%d, MOSI=GPIO%d, MISO=GPIO%d\r\n", 
                  TOUCH_SCLK, TOUCH_MOSI, TOUCH_MISO);
    
    // Configure IRQ pin
    if (TOUCH_IRQ >= 0) {
        pinMode(TOUCH_IRQ, INPUT_PULLUP);
        last_irq_state = digitalRead(TOUCH_IRQ);
        // Use FALLING edge only (LOW = pressed) to reduce false triggers
        attachInterrupt(digitalPinToInterrupt(TOUCH_IRQ), irq_handler, FALLING);
        Serial.printf("[Touch] IRQ pin configured: GPIO%d (initial state: %s, FALLING edge)\r\n", 
                      TOUCH_IRQ, last_irq_state == LOW ? "LOW (pressed)" : "HIGH (not pressed)");
    } else {
        Serial.println("[Touch] WARNING: No IRQ pin configured!");
    }
    
    delay(10);
    
    // Test touch controller by reading initial values
    uint16_t test_x = xpt2046_read(XPT2046_CMD_X);
    uint16_t test_y = xpt2046_read(XPT2046_CMD_Y);
    uint16_t test_z1 = xpt2046_read(XPT2046_CMD_Z1);
    uint16_t test_z2 = xpt2046_read(XPT2046_CMD_Z2);
    Serial.printf("[Touch] Initial read test: X=%d Y=%d Z1=%d Z2=%d\r\n", test_x, test_y, test_z1, test_z2);
    
    // Register LVGL touch input device
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lvgl_touch_read;
    lv_indev_t *indev = lv_indev_drv_register(&indev_drv);
    
    if (indev != NULL) {
        Serial.println("[Touch] Touch controller initialized and registered with LVGL");
    } else {
        Serial.println("[Touch] ERROR: Failed to register touch input device!");
    }
}

void lvgl_touch_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
    static bool last_pressed = false;
    static unsigned long last_log_time = 0;
    static unsigned long touch_count = 0;
    
    bool pressed = false;
    bool irq_pressed = false;
    bool pressure_pressed = false;
    
    // Check IRQ pin
    if (TOUCH_IRQ >= 0) {
        int irq_state = digitalRead(TOUCH_IRQ);
        irq_pressed = (irq_state == LOW);
        
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
    pressed = irq_pressed || pressure_pressed;
    
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
        unsigned long now = millis();
        if (now - last_log_time > 200 || !last_pressed) {  // Log every 200ms or on state change
            Serial.printf("[Touch] Pressed: X=%d Y=%d (IRQ=%d, Pressure=%d)\r\n", 
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
            Serial.println("[Touch] Released");
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
