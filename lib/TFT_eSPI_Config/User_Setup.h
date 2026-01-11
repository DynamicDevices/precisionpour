/**
 * TFT_eSPI User Configuration
 * 
 * This file configures TFT_eSPI for your specific display.
 * Update the driver and pin definitions based on your hardware.
 * 
 * Common drivers: ILI9341, ST7789, ILI9488, ST7735
 */

// Driver selection - uncomment your display driver
#define ILI9341_DRIVER
// #define ST7789_DRIVER
// #define ILI9488_DRIVER
// #define ST7735_DRIVER

// Note: The driver defines (TFT_RAMWR, TFT_INVON, etc.) are included by
// User_Setup_Select.h when ILI9341_DRIVER is defined. If you get errors about
// missing driver constants, ensure ILI9341_DRIVER is defined before
// User_Setup_Select.h processes the driver selection code.

// Display resolution
// Note: TFT_WIDTH and TFT_HEIGHT are defined by the driver (ILI9341_Defines.h)
// The driver defines them as 240x320 (portrait), but TFT_eSPI will swap them
// based on the rotation setting. We use setRotation(1) for landscape (320x240).
// Do not define TFT_WIDTH/TFT_HEIGHT here to avoid redefinition warnings.

// Pin definitions (from config.h)
#define TFT_MOSI 23
#define TFT_MISO 19
#define TFT_SCLK 18
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4

// Backlight control
#define TFT_BL   15
#define TFT_BACKLIGHT_ON HIGH

// Touch controller (XPT2046)
#define TOUCH_CS 21
#define TOUCH_IRQ 22  // Optional, but recommended

// SPI frequency
#define SPI_FREQUENCY  27000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000

// Optional features
#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:.
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
#define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

#define SMOOTH_FONT

// Note: Do NOT define USER_SETUP_LOADED here
// User_Setup_Select.h will include this file, define ILI9341_DRIVER will be seen,
// and then the driver selection code will include the driver defines
