/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * ESP-IDF GPIO ISR Wrapper Implementation
 */

#ifdef ESP_PLATFORM
#include "system/esp_idf_compat.h"
#include "driver/gpio.h"

// GPIO ISR handler storage
gpio_isr_handler_t gpio_isr_handlers[GPIO_NUM_MAX];

// ISR wrapper function
void IRAM_ATTR gpio_isr_handler_wrapper(void* arg) {
    gpio_num_t gpio = (gpio_num_t)(intptr_t)arg;
    if (gpio < GPIO_NUM_MAX && gpio_isr_handlers[gpio].func) {
        // Call the handler - handlers that accept void* arg will receive NULL
        // Handlers that don't need args can ignore the parameter
        gpio_isr_handlers[gpio].func();
    }
}
#endif
