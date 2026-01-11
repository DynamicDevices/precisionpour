/**
 * ESP32 Touchscreen Display Firmware with LVGL
 * 
 * Main entry point for the firmware.
 * Uses LVGL for modern UI development.
 */

#include <Arduino.h>
#include "config.h"
#include "lvgl_display.h"
#include "lvgl_touch.h"
#include "splashscreen.h"
#include "ui.h"

// LVGL tick timer
hw_timer_t *lvgl_timer = NULL;
portMUX_TYPE lvgl_timer_mux = portMUX_INITIALIZER_UNLOCKED;

// LVGL tick handler (called by timer)
void IRAM_ATTR lvgl_tick_handler() {
    portENTER_CRITICAL_ISR(&lvgl_timer_mux);
    lv_tick_inc(1);  // 1ms tick
    portEXIT_CRITICAL_ISR(&lvgl_timer_mux);
}

void setup() {
    // Initialize serial communication
    Serial.begin(SERIAL_BAUD);
    delay(100);
    
    Serial.println("\n\nESP32 Touchscreen Display Firmware");
    Serial.println("=====================================");
    Serial.printf("Chip model: %s\r\n", ESP.getChipModel());
    Serial.printf("Chip revision: %d\r\n", ESP.getChipRevision());
    Serial.printf("CPU frequency: %d MHz\r\n", ESP.getCpuFreqMHz());
    Serial.printf("Flash size: %d bytes\r\n", ESP.getFlashChipSize());
    Serial.printf("Free heap: %d bytes\r\n", ESP.getFreeHeap());
    
    // Initialize backlight early so we can see the display
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    
    // Initialize LVGL
    lv_init();
    Serial.println("LVGL initialized");
    
    // Initialize display driver
    lvgl_display_init();
    
    // Set up LVGL tick timer (1ms)
    lvgl_timer = timerBegin(0, 80, true);  // 80MHz / 80 = 1MHz, 1 tick = 1us
    timerAttachInterrupt(lvgl_timer, &lvgl_tick_handler, true);
    timerAlarmWrite(lvgl_timer, 1000, true);  // 1000us = 1ms
    timerAlarmEnable(lvgl_timer);
    
    // Show splashscreen early (must be after display and timer are ready)
    splashscreen_init();
    splashscreen_set_progress(10);
    splashscreen_set_status("Starting up...");
    delay(200);
    
    // Initialize touch driver (30%)
    lvgl_touch_init();
    splashscreen_set_progress(30);
    splashscreen_set_status("Touch initialized");
    delay(150);
    
    // System information gathering (40%)
    splashscreen_set_progress(40);
    splashscreen_set_status("System ready");
    delay(150);
    
    // Hardware initialization complete (50%)
    splashscreen_set_progress(50);
    splashscreen_set_status("Hardware ready");
    delay(200);
    
    // Prepare main UI (70%)
    splashscreen_set_progress(70);
    splashscreen_set_status("Loading UI...");
    delay(200);
    
    // Initialize main UI (90%)
    ui_init();
    splashscreen_set_progress(90);
    splashscreen_set_status("UI ready");
    delay(300);
    
    // Finalize (100%)
    splashscreen_set_progress(100);
    splashscreen_set_status("Ready!");
    delay(500);  // Show 100% for a moment
    
    // Remove splashscreen and show main UI
    splashscreen_remove();
    
    Serial.println("Setup complete!");
    Serial.printf("Free heap after setup: %d bytes\r\n", ESP.getFreeHeap());
}

void loop() {
    // Handle LVGL tasks (should be called every few milliseconds)
    lv_timer_handler();
    
    // Update UI if needed
    ui_update();
    
    // Small delay to prevent watchdog issues
    delay(5);
}
