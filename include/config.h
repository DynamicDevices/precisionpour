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
#define DISPLAY_ROTATION 0   // 0, 1, 2, or 3

// Touch settings
#define TOUCH_THRESHOLD 400  // Adjust based on your touch controller

// Serial settings
#define SERIAL_BAUD 115200

#endif // CONFIG_H
