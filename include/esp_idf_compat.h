/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * ESP-IDF Compatibility Layer
 * Provides Arduino-like APIs when using ESP-IDF framework
 */

#ifndef ESP_IDF_COMPAT_H
#define ESP_IDF_COMPAT_H

// Use ESP_PLATFORM for detection (ESP-IDF always defines this)
#ifdef ESP_PLATFORM
// ESP-IDF Framework - provide compatibility layer

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdarg.h>
#include <string>

// Logging
#define TAG_MAIN "main"
#define TAG_FLOW "flow"
#define TAG_WIFI "wifi"
#define TAG_MQTT "mqtt"
#define TAG_DISPLAY "display"
#define TAG_TOUCH "touch"

// Serial compatibility - use ESP_LOG
class SerialClass {
public:
    void begin(int baud) {
        // UART already initialized by ESP-IDF, just set baud if needed
        // For now, we'll use ESP_LOG which doesn't need initialization
    }
    void print(const char* str) { ESP_LOGI(TAG_MAIN, "%s", str); }
    void println(const char* str) { ESP_LOGI(TAG_MAIN, "%s", str); }
    void println() { ESP_LOGI(TAG_MAIN, ""); }
    void printf(const char* format, ...) {
        va_list args;
        va_start(args, format);
        char buffer[512];
        vsnprintf(buffer, sizeof(buffer), format, args);
        ESP_LOGI(TAG_MAIN, "%s", buffer);
        va_end(args);
    }
    void flush() { fflush(stdout); }
};

extern SerialClass Serial;

// GPIO compatibility
#define HIGH 1
#define LOW 0
#define INPUT GPIO_MODE_INPUT
#define OUTPUT GPIO_MODE_OUTPUT
// INPUT_PULLUP - use a separate flag since we can't bitwise OR enum types
#define INPUT_PULLUP_FLAG 0x100
#define RISING GPIO_INTR_POSEDGE
#define FALLING GPIO_INTR_NEGEDGE

static inline void pinMode(int pin, int mode) {
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << pin);
    if (mode == OUTPUT) {
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    } else if (mode & INPUT_PULLUP_FLAG) {
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    } else {
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    }
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);
}

static inline void digitalWrite(int pin, int level) {
    gpio_set_level((gpio_num_t)pin, level);
}

static inline int digitalRead(int pin) {
    return gpio_get_level((gpio_num_t)pin);
}

// Timing functions
static inline void delay(unsigned long ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

static inline void delayMicroseconds(unsigned int us) {
    esp_rom_delay_us(us);
}

static inline unsigned long millis(void) {
    return (unsigned long)(esp_timer_get_time() / 1000ULL);
}

static inline unsigned long micros(void) {
    return (unsigned long)esp_timer_get_time();
}

// Interrupt handling
typedef void (*voidFuncPtr)(void);

static inline int digitalPinToInterrupt(int pin) {
    return pin;
}

// Interrupt service routine handler structure
typedef struct {
    gpio_num_t pin;
    voidFuncPtr func;
    gpio_int_type_t type;
} gpio_isr_handler_t;

// Global ISR handler storage
extern gpio_isr_handler_t gpio_isr_handlers[GPIO_NUM_MAX];

static inline void attachInterrupt(int pin, voidFuncPtr func, int mode) {
    gpio_num_t gpio = (gpio_num_t)pin;
    
    // Configure GPIO for interrupt
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << pin);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = (gpio_int_type_t)mode;
    gpio_config(&io_conf);
    
    // Store handler
    if (gpio < GPIO_NUM_MAX) {
        gpio_isr_handlers[gpio].pin = gpio;
        gpio_isr_handlers[gpio].func = func;
        gpio_isr_handlers[gpio].type = (gpio_int_type_t)mode;
    }
    
    // Install ISR service if not already installed
    static bool isr_service_installed = false;
    if (!isr_service_installed) {
        gpio_install_isr_service(0);
        isr_service_installed = true;
    }
    
    // Add ISR handler - use a wrapper function
    // Forward declare the wrapper (defined in esp_idf_gpio_isr.cpp)
    extern void IRAM_ATTR gpio_isr_handler_wrapper(void* arg);
    gpio_isr_handler_add(gpio, gpio_isr_handler_wrapper, (void*)(intptr_t)gpio);
}

static inline void detachInterrupt(int pin) {
    gpio_isr_handler_remove((gpio_num_t)pin);
}

// Critical section - need a static mutex for each call site
// For simplicity, use a global mutex (not thread-safe but works for single-threaded Arduino code)
static portMUX_TYPE critical_mux = portMUX_INITIALIZER_UNLOCKED;

static inline void noInterrupts(void) {
    portENTER_CRITICAL(&critical_mux);
}

static inline void interrupts(void) {
    portEXIT_CRITICAL(&critical_mux);
}

// ESP32 specific functions
#define ESP_OK 0
typedef int esp_err_t;

// String compatibility - use typedef to avoid conflicts with typedef in headers
#ifndef String
typedef std::string String;
#endif
#define F(x) x

// Timer compatibility (Arduino hw_timer_t -> ESP-IDF esp_timer)
typedef struct {
    esp_timer_handle_t timer_handle;
    void (*callback)(void);
} hw_timer_t;

static inline hw_timer_t* timerBegin(uint8_t timer_num, uint16_t divider, bool countUp) {
    hw_timer_t* timer = (hw_timer_t*)malloc(sizeof(hw_timer_t));
    if (!timer) return NULL;
    
    esp_timer_create_args_t timer_args = {};
    timer_args.callback = [](void* arg) {
        hw_timer_t* t = (hw_timer_t*)arg;
        if (t && t->callback) {
            t->callback();
        }
    };
    timer_args.arg = timer;
    timer_args.name = "lvgl_timer";
    
    esp_timer_create(&timer_args, &timer->timer_handle);
    return timer;
}

static inline void timerAttachInterrupt(hw_timer_t* timer, void (*callback)(), bool edge) {
    if (timer) {
        timer->callback = callback;
    }
}

static inline void timerAlarmWrite(hw_timer_t* timer, uint64_t alarm_value, bool autoreload) {
    if (timer && timer->timer_handle) {
        esp_timer_stop(timer->timer_handle);
        esp_timer_start_periodic(timer->timer_handle, alarm_value);
    }
}

static inline void timerAlarmEnable(hw_timer_t* timer) {
    if (timer && timer->timer_handle) {
        // Already started in timerAlarmWrite
    }
}

// Port MUX compatibility - already defined by FreeRTOS, don't redefine
// typedef portMUX_TYPE portMUX_TYPE;  // Already defined
// #define portMUX_INITIALIZER_UNLOCKED portMUX_INITIALIZER_UNLOCKED  // Already defined

// IRAM_ATTR compatibility - already defined by ESP-IDF, don't redefine
// #define IRAM_ATTR IRAM_ATTR  // Already defined

// Byte type
typedef uint8_t byte;

#endif // ESP_PLATFORM

#endif // ESP_IDF_COMPAT_H
