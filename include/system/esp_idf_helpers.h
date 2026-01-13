/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * ESP-IDF Helper Functions
 * Native ESP-IDF replacements for common operations
 */

#ifndef ESP_IDF_HELPERS_H
#define ESP_IDF_HELPERS_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include <stdint.h>

// Timing helpers
static inline void delay_ms(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

static inline uint64_t get_time_ms(void) {
    return esp_timer_get_time() / 1000ULL;
}

// GPIO helpers
#define HIGH 1
#define LOW 0

static inline void gpio_setup_output(gpio_num_t pin) {
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << pin);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);
}

static inline void gpio_setup_input(gpio_num_t pin, bool pullup) {
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << pin);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = pullup ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);
}

// delayMicroseconds is provided by esp_idf_compat.h
// Don't redefine it here to avoid ambiguity

#endif // ESP_IDF_HELPERS_H
