/**
 * Hardware Configuration
 * 
 * Pin definitions and hardware settings for the ESP32 touchscreen display.
 * Update these values based on your specific hardware.
 */

#ifndef CONFIG_H
#define CONFIG_H

// Display pins (SPI)
#define TFT_MOSI 23
#define TFT_MISO 19
#define TFT_SCLK 18
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4
#define TFT_BL   15  // Backlight

// Touch pins (SPI or I2C - depends on your touch controller)
#define TOUCH_CS 21
#define TOUCH_IRQ 22  // Optional interrupt pin

// I2C pins (if using I2C for touch)
#define I2C_SDA 21
#define I2C_SCL 22

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
#define MQTT_KEEPALIVE 60                    // MQTT keepalive interval (seconds)

// Wokwi simulator workaround
// Wokwi's XPT2046 simulation doesn't respond to SPI, so we use IRQ pin simulation
// In Wokwi, clicking the display should trigger the IRQ pin
#define WOKWI_SIMULATOR 1  // Set to 1 when running in Wokwi, 0 for real hardware

// Future hardware pins (placeholders for flow meter and RFID/NFC)
#define FLOW_METER_PIN 25  // GPIO pin for flow meter interrupt
#define RFID_CS_PIN 26     // GPIO pin for RFID/NFC chip select
#define RFID_RST_PIN 27    // GPIO pin for RFID/NFC reset

#endif // CONFIG_H
