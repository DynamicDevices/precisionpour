# Resources Directory

This directory contains image and asset files for the ESP32 touchscreen firmware.

## Splashscreen

Place your splashscreen image here as `splashscreen.png`.

The image will be used during firmware initialization to display a startup screen.

## Image Requirements

For best results with LVGL and ESP32:
- **Format**: PNG (recommended) or BMP
- **Color depth**: 16-bit RGB565 (for best performance)
- **Size**: Match your display resolution (320x240 default)
- **File size**: Keep under 100KB if loading from filesystem

## Usage

The splashscreen can be:
1. **Embedded in firmware** (compiled as C array) - Recommended for small images
2. **Loaded from filesystem** (SPIFFS/LittleFS) - For larger images or dynamic updates

See `src/ui.cpp` for splashscreen implementation.
