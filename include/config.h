/**
 * Hardware Configuration
 * 
 * Pin definitions and hardware settings for the ESP32 touchscreen display.
 * Update these values based on your specific hardware.
 */

#ifndef CONFIG_H
#define CONFIG_H

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

// I2C pins (not used for this display - touch uses SPI)
// #define I2C_SDA 21
// #define I2C_SCL 22

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

// Include secrets (WiFi and MQTT credentials)
// secrets.h is gitignored - copy secrets.h.example to secrets.h and fill in your credentials
#include "secrets.h"

// WiFi Configuration
#define WIFI_RECONNECT_DELAY 5000          // Delay between reconnection attempts (ms)

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

// Wokwi simulator workaround
// Wokwi's XPT2046 simulation doesn't respond to SPI, so we use IRQ pin simulation
// In Wokwi, clicking the display should trigger the IRQ pin
#define WOKWI_SIMULATOR 0  // Set to 1 when running in Wokwi, 0 for real hardware

// Future hardware pins (placeholders for flow meter and RFID/NFC)
#define FLOW_METER_PIN 25  // GPIO pin for flow meter interrupt
#define RFID_CS_PIN 26     // GPIO pin for RFID/NFC chip select
#define RFID_RST_PIN 32    // GPIO pin for RFID/NFC reset (changed from 27, which is now TFT_BL)

#endif // CONFIG_H
