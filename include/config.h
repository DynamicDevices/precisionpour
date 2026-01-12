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

// Cost configuration (for pouring mode)
#define COST_PER_UNIT_DEFAULT 5.00  // Default cost per liter in currency (e.g., £5.00 per liter)

// Currency symbol configuration
// Options: "GBP " (pound - text prefix since £ symbol not in font), "$" (dollar), or any other string
// Note: £ symbol (U+00A3) is not in the Montserrat font, so we use "GBP " text prefix instead
#define CURRENCY_SYMBOL "GBP "  // Default currency symbol (text prefix for pounds)

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

// RFID/NFC (if needed):
#define RFID_CS_PIN 26     // GPIO pin for RFID/NFC chip select (GPIO26 available - was audio DAC, but can be repurposed)
#define RFID_RST_PIN 32    // GPIO pin for RFID/NFC reset (GPIO32 available)

// RGB LED pins (common anode: LOW=on, HIGH=off)
#define LED_R_PIN 22  // Red LED control - GPIO22
#define LED_G_PIN 16  // Green LED control - GPIO16
#define LED_B_PIN 17  // Blue LED control - GPIO17

#endif // CONFIG_H
