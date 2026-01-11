# Hardware Setup Guide

## Identifying Your Display

To properly configure the firmware, you need to identify:

1. **Display Driver IC** (e.g., ILI9341, ST7789, ILI9488)
2. **Touch Controller** (e.g., XPT2046, FT6236, GT911)
3. **Interface Type** (SPI or I2C for both display and touch)
4. **Resolution** (e.g., 320x240, 480x320, 800x480)
5. **Pin Connections** (check your display's documentation)

## Common ESP32 Touchscreen Display Configurations

### Typical SPI Display Pins
- **MOSI** (Master Out Slave In): GPIO 23
- **MISO** (Master In Slave Out): GPIO 19
- **SCLK** (Serial Clock): GPIO 18
- **CS** (Chip Select): GPIO 5
- **DC** (Data/Command): GPIO 2
- **RST** (Reset): GPIO 4
- **BL** (Backlight): GPIO 15

### Typical Touch Controller Pins (SPI)
- **TOUCH_CS**: GPIO 21
- **TOUCH_IRQ**: GPIO 22 (optional interrupt pin)

### Typical Touch Controller Pins (I2C)
- **SDA**: GPIO 21
- **SCL**: GPIO 22
- **RST**: GPIO 4 (optional)
- **IRQ**: GPIO 5 (optional interrupt pin)

## Finding Your Display Specifications

1. Check the product listing on Amazon for specifications
2. Look for a product manual or datasheet
3. Check the manufacturer's website
4. Common manufacturers: VIEWESMART, 4D Systems, Adafruit, Waveshare

## Configuration Steps

1. **Update `platformio.ini`**:
   - Uncomment and add the appropriate libraries
   - Adjust partition scheme if needed

2. **Update `src/main.cpp`**:
   - Set correct pin definitions
   - Uncomment and configure display initialization
   - Uncomment and configure touch initialization

3. **If using TFT_eSPI**:
   - Edit `~/.platformio/lib/TFT_eSPI/User_Setup.h`
   - Configure driver, pins, and resolution

4. **If using LVGL**:
   - Configure display and touch drivers in your code
   - Set up display buffer and flush callback

## Testing Your Setup

1. Upload a simple test sketch that:
   - Fills the screen with colors
   - Displays text
   - Reads and prints touch coordinates

2. Verify:
   - Display shows output correctly
   - Touch coordinates are accurate
   - Backlight works
   - Colors are correct

## Troubleshooting

- **Display shows nothing**: Check pin connections, backlight, power supply
- **Touch not working**: Verify touch controller type and pin connections
- **Colors wrong**: Check display driver configuration
- **Screen flickers**: May need to adjust SPI speed or add capacitors
