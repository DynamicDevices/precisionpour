/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * Hardware Configuration
 * 
 * Pin definitions and hardware settings for the ESP32 touchscreen display.
 * 
 * This file supports both ESP-IDF (KConfig) and Arduino framework:
 * - ESP-IDF: Uses CONFIG_* values from KConfig (idf.py menuconfig)
 * - Arduino: Uses #define fallbacks (edit this file directly)
 */

#ifndef CONFIG_H
#define CONFIG_H

// Check if we're using ESP-IDF with KConfig
// ESP-IDF always defines ESP_PLATFORM
#ifdef ESP_PLATFORM
    // ESP-IDF framework - include sdkconfig.h
    // sdkconfig.h is generated during build in the build directory
    // PlatformIO automatically adds build/include to include path
    #ifdef __has_include
        #if __has_include("sdkconfig.h")
            #include "sdkconfig.h"
        #elif __has_include("build/include/sdkconfig.h")
            #include "build/include/sdkconfig.h"
        #endif
    #else
        // Fallback: try to include sdkconfig.h directly
        // ESP-IDF build system should have this in the include path
        #include "sdkconfig.h"
    #endif
#endif

#ifdef CONFIG_TFT_MOSI
    // ESP-IDF: Use KConfig values
    #define TFT_MOSI CONFIG_TFT_MOSI
    #define TFT_MISO CONFIG_TFT_MISO
    #define TFT_SCLK CONFIG_TFT_SCLK
    #define TFT_CS CONFIG_TFT_CS
    #define TFT_DC CONFIG_TFT_DC
    #define TFT_RST CONFIG_TFT_RST
    #define TFT_BL CONFIG_TFT_BL
    
    #define TOUCH_SCLK CONFIG_TOUCH_SCLK
    #define TOUCH_MOSI CONFIG_TOUCH_MOSI
    #define TOUCH_MISO CONFIG_TOUCH_MISO
    #define TOUCH_CS CONFIG_TOUCH_CS
    #define TOUCH_IRQ CONFIG_TOUCH_IRQ
    
    #define FLOW_METER_PIN CONFIG_FLOW_METER_PIN
    
    #define LED_R_PIN CONFIG_LED_R_PIN
    #define LED_G_PIN CONFIG_LED_G_PIN
    #define LED_B_PIN CONFIG_LED_B_PIN
    
    #define RFID_CS_PIN CONFIG_RFID_CS_PIN
    #define RFID_RST_PIN CONFIG_RFID_RST_PIN
    
    #define DISPLAY_WIDTH CONFIG_DISPLAY_WIDTH
    #define DISPLAY_HEIGHT CONFIG_DISPLAY_HEIGHT
    #define DISPLAY_ROTATION CONFIG_DISPLAY_ROTATION
    #define TOUCH_THRESHOLD CONFIG_TOUCH_THRESHOLD
    
    #define SERIAL_BAUD CONFIG_SERIAL_BAUD
    
    #define TEST_MODE CONFIG_TEST_MODE_ENABLED
    
    // WiFi Configuration
    // KConfig values: CONFIG_WIFI_SSID and CONFIG_WIFI_PASSWORD (from sdkconfig.h)
    // secrets.h will also define WIFI_SSID and WIFI_PASSWORD as fallback
    // Code will check CONFIG_WIFI_SSID at runtime to decide which to use
    // Note: CONFIG_WIFI_SSID is a string config from KConfig, available via sdkconfig.h
    #define WIFI_RECONNECT_DELAY CONFIG_WIFI_RECONNECT_DELAY
    #define USE_IMPROV_WIFI CONFIG_USE_IMPROV_WIFI
    #ifdef CONFIG_IMPROV_START_BY_DEFAULT
        #define IMPROV_START_BY_DEFAULT CONFIG_IMPROV_START_BY_DEFAULT
    #else
        #define IMPROV_START_BY_DEFAULT 0  // Default: disabled
    #endif
    #ifdef CONFIG_IMPROV_WIFI_TIMEOUT_MS
        #define IMPROV_WIFI_TIMEOUT_MS CONFIG_IMPROV_WIFI_TIMEOUT_MS
    #else
        #define IMPROV_WIFI_TIMEOUT_MS 300000  // Default: 5 minutes
    #endif
    #define USE_SAVED_CREDENTIALS CONFIG_USE_SAVED_CREDENTIALS
    
    // MQTT Configuration
    // KConfig values are available as CONFIG_MQTT_SERVER from sdkconfig.h
    // secrets.h will also define MQTT_SERVER as fallback
    // Code will check CONFIG_MQTT_SERVER at runtime to decide which to use
    #define MQTT_SERVER_KCONFIG CONFIG_MQTT_SERVER
    #define MQTT_PORT CONFIG_MQTT_PORT
    #define MQTT_CLIENT_ID_PREFIX CONFIG_MQTT_CLIENT_ID_PREFIX
    #define MQTT_TOPIC_PREFIX CONFIG_MQTT_TOPIC_PREFIX
    #define MQTT_RECONNECT_DELAY CONFIG_MQTT_RECONNECT_DELAY
    #define MQTT_KEEPALIVE CONFIG_MQTT_KEEPALIVE
    
    #define ENABLE_WATCHDOG CONFIG_ENABLE_WATCHDOG
    #define WATCHDOG_TIMEOUT_SEC CONFIG_WATCHDOG_TIMEOUT_SEC
    #define MAX_CONSECUTIVE_ERRORS CONFIG_MAX_CONSECUTIVE_ERRORS
    #define ERROR_RESET_DELAY_MS CONFIG_ERROR_RESET_DELAY_MS
    
    // Development Options
    #ifdef CONFIG_DEBUG_QR_TAP_TO_POUR
        #define DEBUG_QR_TAP_TO_POUR CONFIG_DEBUG_QR_TAP_TO_POUR
    #else
        #define DEBUG_QR_TAP_TO_POUR 0
    #endif
    
    #ifdef CONFIG_DEBUG_POURING_TAP_TO_FINISHED
        #define DEBUG_POURING_TAP_TO_FINISHED CONFIG_DEBUG_POURING_TAP_TO_FINISHED
    #else
        #define DEBUG_POURING_TAP_TO_FINISHED 0
    #endif
    
    #ifdef CONFIG_DEBUG_FINISHED_TAP_TO_QR
        #define DEBUG_FINISHED_TAP_TO_QR CONFIG_DEBUG_FINISHED_TAP_TO_QR
    #else
        #define DEBUG_FINISHED_TAP_TO_QR 0
    #endif
    
    // Note: CONFIG_COST_PER_UNIT_DEFAULT is a string in KConfig
    // We'll need to parse it at runtime where it's used, or use a helper function
    // For now, define both string and a default numeric value
    #define COST_PER_UNIT_DEFAULT_STR CONFIG_COST_PER_UNIT_DEFAULT
    // Default numeric value (will be parsed from string at runtime if needed)
    #define COST_PER_UNIT_DEFAULT 5.00  // Fallback, actual value from CONFIG_COST_PER_UNIT_DEFAULT
    #define CURRENCY_SYMBOL CONFIG_CURRENCY_SYMBOL
    #ifdef CONFIG_FINISHED_SCREEN_TIMEOUT_SEC
        #define FINISHED_SCREEN_TIMEOUT_SEC CONFIG_FINISHED_SCREEN_TIMEOUT_SEC
    #else
        #define FINISHED_SCREEN_TIMEOUT_SEC 5  // Default fallback
    #endif
    
    
#else
    // Arduino Framework: Use #define fallbacks
    // Display pins (SPI) - 2.8" ESP32-32E Display pinout from hardware documentation
    #define TFT_MOSI 13  // LCD SPI bus write data signal - GPIO13
    #define TFT_MISO 12  // LCD SPI bus read data signal - GPIO12
    #define TFT_SCLK 14  // LCD SPI bus clock signal - GPIO14
    #define TFT_CS   15  // LCD screen selection control signal (low level effective) - GPIO15
    #define TFT_DC   2   // LCD command/data selection (high=data, low=command) - GPIO2
    #define TFT_RST  4   // LCD reset control signal (low level reset, shared with ESP32 EN pin) - GPIO4
    #define TFT_BL   21  // LCD backlight control (high=on, low=off) - GPIO21 (was GPIO27 - WRONG!)

    // Touch pins (XPT2046 resistive touchscreen) - Has its own SPI bus!
    #define TOUCH_SCLK 25  // Touch SPI bus clock signal - GPIO25
    #define TOUCH_MOSI 32  // Touch SPI bus write data signal - GPIO32
    #define TOUCH_MISO 39  // Touch SPI bus read data signal - GPIO39
    #define TOUCH_CS 33    // Touch chip selection control signal (low level effective) - GPIO33
    #define TOUCH_IRQ 36   // Touch interrupt signal (low level = touch event) - GPIO36

    // Display settings
    #define DISPLAY_WIDTH  320   // Update based on your display
    #define DISPLAY_HEIGHT 240   // Update based on your display
    #define DISPLAY_ROTATION 1   // 0=portrait, 1=landscape (USB right), 2=portrait, 3=landscape (USB left)

    // Touch settings
    #define TOUCH_THRESHOLD 100  // Adjust based on your touch controller (lower = more sensitive)

    // Serial settings
    #define SERIAL_BAUD 115200

    // Operating mode
    // Set to 1 for test mode (hardware testing), 0 for production mode (PrecisionPour branding)
    #define TEST_MODE 0

    // WiFi Configuration
    #define WIFI_RECONNECT_DELAY 5000          // Delay between reconnection attempts (ms)

    // Error Recovery Configuration
    #define ENABLE_WATCHDOG 1                  // Enable ESP32 watchdog timer (1=enabled, 0=disabled)
    #define WATCHDOG_TIMEOUT_SEC 60            // Watchdog timeout in seconds (reset if not fed)
    #define MAX_CONSECUTIVE_ERRORS 10         // Maximum consecutive errors before reset
    #define ERROR_RESET_DELAY_MS 5000         // Delay before reset after max errors (ms)

    // MQTT Configuration
    #define MQTT_PORT 1883                     // MQTT port (1883 for non-TLS, 8883 for TLS)
    #define MQTT_CLIENT_ID_PREFIX "precisionpour" // Prefix for MQTT client ID
    #define MQTT_TOPIC_PREFIX "precisionpour"     // Prefix for MQTT topics
    #define MQTT_RECONNECT_DELAY 5000            // Delay between MQTT reconnection attempts (ms)
    // MQTT_KEEPALIVE: Undefine library default (15) and set our own (60 seconds)
    #ifdef MQTT_KEEPALIVE
    #undef MQTT_KEEPALIVE
    #endif
    #define MQTT_KEEPALIVE 60                    // MQTT keepalive interval (seconds)

    // Flow meter pin (YF-S201 Hall Effect Flow Sensor)
    // Changed from GPIO25 to GPIO26 to avoid conflict with TOUCH_SCLK
    // GPIO26 is interrupt-capable and available (was Audio DAC, can be repurposed)
    #define FLOW_METER_PIN 26  // GPIO pin for flow meter interrupt

    // WiFi Provisioning Configuration
    #define USE_IMPROV_WIFI 0  // Set to 1 to enable Improv WiFi BLE provisioning, 0 to disable (Arduino: edit this file)
    #define IMPROV_START_BY_DEFAULT 0  // Set to 1 to start Improv by default, 0 to start only on connection failure (Arduino: edit this file)
    #define IMPROV_WIFI_TIMEOUT_MS 300000  // 5 minutes timeout for Improv WiFi provisioning (Arduino: edit this file)
    #define USE_SAVED_CREDENTIALS 1  // Set to 1 to use saved credentials from EEPROM, 0 to always use secrets.h

    // Cost configuration (for pouring mode)
    #define COST_PER_UNIT_DEFAULT 5.00  // Default cost per liter in currency (e.g., £5.00 per liter)

    // Currency symbol configuration
    // Options: "GBP " (pound - text prefix since £ symbol not in font), "$" (dollar), or any other string
    // Note: £ symbol (U+00A3) is not in the Montserrat font, so we use "GBP " text prefix instead
    #define CURRENCY_SYMBOL "GBP "  // Default currency symbol (text prefix for pounds)

    // Finished screen timeout
    #define FINISHED_SCREEN_TIMEOUT_SEC 5  // Default timeout in seconds before returning to QR code screen

    // Development Options
    #define DEBUG_QR_TAP_TO_POUR 0  // Set to 1 to enable QR code tap to pour for debugging
    #define DEBUG_POURING_TAP_TO_FINISHED 0  // Set to 1 to enable pouring screen tap to finished for debugging
    #define DEBUG_FINISHED_TAP_TO_QR 0  // Set to 1 to enable finished screen tap to QR code for debugging

    // RGB LED pins (common anode: LOW=on, HIGH=off)
    #define LED_R_PIN 22  // Red LED control - GPIO22
    #define LED_G_PIN 16  // Green LED control - GPIO16
    #define LED_B_PIN 17  // Blue LED control - GPIO17

    // RFID/NFC (if needed):
    #define RFID_CS_PIN 26     // GPIO pin for RFID/NFC chip select (GPIO26 available - was audio DAC, but can be repurposed)
    #define RFID_RST_PIN 32    // GPIO pin for RFID/NFC reset (GPIO32 available)
#endif

// Include secrets (WiFi and MQTT credentials)
// secrets.h is gitignored - copy secrets.h.example to secrets.h and fill in your credentials
// For ESP-IDF: KConfig values take priority, secrets.h is fallback if KConfig values are empty
// For Arduino: secrets.h is always used
// Always include secrets.h (for MQTT_SERVER and as fallback for WiFi if KConfig is empty)
#include "secrets.h"

// For ESP-IDF: Override with KConfig values if they are non-empty
#ifdef ESP_PLATFORM
    // KConfig values are already defined above, but if they're empty strings,
    // the secrets.h values will be used (they're defined after this include)
    // The code will check strlen() at runtime to determine which to use
#endif

// Available pins from hardware documentation:
// MicroSD Card (if needed):
//   GPIO5:  SD card select signal (CS), low level effective
//   GPIO23: SD card SPI bus write data (MOSI) - shared with MicroSD and SPI peripheral
//   GPIO18: SD card SPI bus clock (SCLK) - shared with MicroSD and SPI peripheral  
//   GPIO19: SD card SPI bus read data (MISO) - shared with MicroSD and SPI peripheral
// Note: GPIO18, GPIO19, GPIO23 are used by LCD SPI, so MicroSD would need different pins or sharing

// Audio (if needed):
//   GPIO4:  Audio enable signal (low level enable, high level disable)
//   GPIO26: Audio signal DAC output signal
// Note: GPIO4 is used for TFT_RST, so audio enable would conflict

// KEY:
//   GPIO0:  Download mode select button (press and hold to power on, then release to enter download mode)
//   EN:     ESP32-32E reset button, low level reset (shared with LCD reset)

// Serial Port (if not using USB serial):
//   GPIO3 (RXD0): ESP32-32E serial port receiving signal (can be used as ordinary IO if serial port not used)
//   GPIO1 (TXD0): ESP32-32E serial port sends signals (can be used as ordinary IO if serial port not used)

// Battery (if needed):
//   GPIO34: Battery voltage ADC value Get Signal (input only, ADC1_CH6)

// SPI Peripheral (shared with MicroSD, can be used as GPIO if not needed):
//   GPIO27: SPI peripheral chip selection signal (CS), low level effective (can be used as ordinary IO if SPI device not used)
//   GPIO18: SPI bus clock pin for SPI peripherals (shared with MicroSD, currently used for LCD SCLK)
//   GPIO19: SPI bus read data pin (shared with MicroSD, currently used for LCD MISO)
//   GPIO23: SPI bus write data pin (shared with MicroSD, currently used for LCD MOSI)
// Note: GPIO18, GPIO19, GPIO23 are currently used for LCD SPI, so SPI peripherals would need different pins or sharing

// NC (Not Connected):
//   GPIO35: Can only be used as input IO (ADC1_CH7)

#endif // CONFIG_H
