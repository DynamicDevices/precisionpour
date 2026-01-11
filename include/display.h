/**
 * Display Driver Interface
 * 
 * Abstract interface for display operations.
 * Implement this for your specific display library (TFT_eSPI, LVGL, etc.)
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

class Display {
public:
    virtual ~Display() = default;
    
    // Initialization
    virtual bool begin() = 0;
    
    // Basic drawing
    virtual void fillScreen(uint16_t color) = 0;
    virtual void setRotation(uint8_t rotation) = 0;
    
    // Text
    virtual void setTextColor(uint16_t color, uint16_t bgColor = 0) = 0;
    virtual void setTextSize(uint8_t size) = 0;
    virtual void setCursor(int16_t x, int16_t y) = 0;
    virtual void print(const char* text) = 0;
    virtual void println(const char* text) = 0;
    
    // Graphics
    virtual void drawPixel(int16_t x, int16_t y, uint16_t color) = 0;
    virtual void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) = 0;
    virtual void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) = 0;
    virtual void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) = 0;
    
    // Dimensions
    virtual int16_t width() = 0;
    virtual int16_t height() = 0;
};

#endif // DISPLAY_H
