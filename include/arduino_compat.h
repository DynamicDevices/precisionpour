/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Arduino compatibility layer for ESP-IDF
 * Provides Arduino-like APIs when using ESP-IDF framework
 */

#ifndef ARDUINO_COMPAT_H
#define ARDUINO_COMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

// ESP-IDF includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Arduino compatibility defines
#define HIGH 1
#define LOW 0
#define INPUT GPIO_MODE_INPUT
#define OUTPUT GPIO_MODE_OUTPUT
#define INPUT_PULLUP (GPIO_MODE_INPUT | GPIO_PULLUP_ONLY)
#define RISING GPIO_INTR_POSEDGE
#define FALLING GPIO_INTR_NEGEDGE

// Arduino-like functions
static inline void pinMode(int pin, int mode) {
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << pin);
    if (mode == OUTPUT) {
        io_conf.mode = GPIO_MODE_OUTPUT;
    } else if (mode == INPUT_PULLUP) {
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    } else {
        io_conf.mode = GPIO_MODE_INPUT;
    }
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);
}

static inline void digitalWrite(int pin, int level) {
    gpio_set_level((gpio_num_t)pin, level);
}

static inline int digitalRead(int pin) {
    return gpio_get_level((gpio_num_t)pin);
}

static inline void delay(unsigned long ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

static inline unsigned long millis(void) {
    return (unsigned long)(esp_timer_get_time() / 1000ULL);
}

static inline unsigned long micros(void) {
    return (unsigned long)esp_timer_get_time();
}

// Serial compatibility (simplified - use ESP_LOG for actual logging)
#define Serial ESP_LOGI

// Interrupt handling
typedef void (*voidFuncPtr)(void);
static inline int digitalPinToInterrupt(int pin) {
    return pin;
}

// Note: attachInterrupt implementation would need proper GPIO ISR setup
// This is a simplified version - may need enhancement for actual interrupt handling
#define attachInterrupt(pin, func, mode) \
    do { \
        gpio_set_intr_type((gpio_num_t)(pin), (gpio_int_type_t)(mode)); \
        gpio_install_isr_service(0); \
        gpio_isr_handler_add((gpio_num_t)(pin), (gpio_isr_t)(func), NULL); \
    } while(0)

// ESP32 specific
#define ESP_OK 0
typedef enum {
    ESP_OK_IDF = 0,
} esp_err_t;

// Note: This is a minimal compatibility layer
// Some Arduino libraries may not work directly with ESP-IDF
// Consider using ESP-IDF native APIs where possible

#ifdef __cplusplus
}
#endif

#endif // ARDUINO_COMPAT_H
