/**
 * Copyright (c) 2026 Dynamic Devices Ltd
 * All rights reserved.
 * 
 * Proprietary and confidential software.
 * See LICENSE file for full license terms.
 */

/**
 * Touch Controller Interface
 * 
 * Abstract interface for touch input operations.
 * Implement this for your specific touch controller (XPT2046, FT6236, etc.)
 */

#ifndef TOUCH_H
#define TOUCH_H

#ifdef ESP_PLATFORM
    #include <stdint.h>
    #include <stdbool.h>
#else
    #include <Arduino.h>
#endif

struct TouchPoint {
    int16_t x;
    int16_t y;
    bool pressed;
};

class TouchController {
public:
    virtual ~TouchController() = default;
    
    // Initialization
    virtual bool begin() = 0;
    
    // Touch reading
    virtual bool touched() = 0;
    virtual TouchPoint getPoint() = 0;
    
    // Calibration (if supported)
    virtual void setCalibration(int16_t xMin, int16_t xMax, int16_t yMin, int16_t yMax) {}
    
    // Interrupt handling (if supported)
    virtual void enableInterrupt() {}
    virtual void disableInterrupt() {}
};

#endif // TOUCH_H
