# LVGL Setup and Configuration Guide

## Overview

This project uses LVGL (Light and Versatile Graphics Library) version 8.4.0 for UI development. LVGL provides a modern, widget-based framework for creating professional touchscreen interfaces.

## Configuration Files

### `include/lv_conf.h`
Main LVGL configuration file. Controls:
- Memory settings
- Feature enables/disables
- Color depth
- Logging levels
- Widget support

### `lib/TFT_eSPI_Config/User_Setup.h`
TFT_eSPI display driver configuration. Update this based on your display:
- Display driver (ILI9341, ST7789, etc.)
- Pin definitions
- Resolution
- SPI settings

## Display Driver Integration

The display driver is implemented in:
- `include/lvgl_display.h` - Interface
- `src/lvgl_display.cpp` - Implementation

**Key functions:**
- `lvgl_display_init()` - Initialize display and LVGL display driver
- `lvgl_display_flush()` - LVGL callback to update display

The driver uses TFT_eSPI for low-level display operations and provides a flush callback for LVGL's rendering engine.

## Touch Driver Integration

The touch driver is implemented in:
- `include/lvgl_touch.h` - Interface
- `src/lvgl_touch.cpp` - Implementation

**Key functions:**
- `lvgl_touch_init()` - Initialize touch controller
- `lvgl_touch_read()` - LVGL callback to read touch input

**Important:** The touch read function is currently a placeholder. You need to implement it based on your specific touch controller (XPT2046, FT6236, etc.).

## UI Development

### Creating UI Elements

The UI is created in `src/ui.cpp`. Example:

```cpp
// Create a label
lv_obj_t *label = lv_label_create(lv_scr_act());
lv_label_set_text(label, "Hello LVGL");
lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

// Create a button
lv_obj_t *btn = lv_btn_create(lv_scr_act());
lv_obj_set_size(btn, 150, 50);
lv_obj_align(btn, LV_ALIGN_CENTER, 0, 50);

// Add event callback
lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);
```

### Available Widgets

LVGL provides many widgets:
- **Labels** - Text display
- **Buttons** - Clickable buttons
- **Sliders** - Value input
- **Charts** - Data visualization
- **Lists** - Scrollable lists
- **Keyboards** - On-screen keyboards
- **Meters** - Gauge displays
- And many more...

See [LVGL Documentation](https://docs.lvgl.io/) for complete widget reference.

## Memory Management

LVGL uses dynamic memory allocation. The configuration in `lv_conf.h` sets:
- `LV_MEM_SIZE` - Total memory pool (32KB default)
- Display buffer size - Set in `lvgl_display.cpp` (currently 1/10 of screen)

**Tips:**
- Increase `LV_MEM_SIZE` if you get out-of-memory errors
- Reduce display buffer size if memory is tight
- Use PSRAM if available (already enabled in build flags)

## Performance Optimization

### Display Buffer
The display buffer size affects performance:
- Larger buffer = smoother scrolling, more memory used
- Smaller buffer = less memory, more frequent updates

Current setting: `DISPLAY_WIDTH * DISPLAY_HEIGHT / 10`

### Refresh Rate
LVGL refresh period is set in `lv_conf.h`:
```c
#define LV_DISP_DEF_REFR_PERIOD 30  // 30ms = ~33 FPS
```

### Tick Source
LVGL needs a 1ms tick. This is provided by a hardware timer in `main.cpp`:
```cpp
timerAlarmWrite(lvgl_timer, 1000, true);  // 1000us = 1ms
```

## Touch Controller Setup

### XPT2046 (SPI Touch Controller)

If using XPT2046, you'll need to:
1. Install XPT2046 library or implement SPI communication
2. Read X and Y coordinates from touch controller
3. Convert to display coordinates (may need calibration)
4. Update `lvgl_touch_read()` function

Example implementation:
```cpp
void lvgl_touch_read(lv_indev_t *indev, lv_indev_data_t *data) {
    // Read touch coordinates from XPT2046
    int16_t x, y;
    bool touched = read_xpt2046(&x, &y);
    
    if (touched) {
        // Convert to display coordinates
        data->point.x = map(x, TOUCH_X_MIN, TOUCH_X_MAX, 0, DISPLAY_WIDTH);
        data->point.y = map(y, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, DISPLAY_HEIGHT);
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
```

### FT6236 (I2C Touch Controller)

If using FT6236:
1. Use I2C to communicate with touch controller
2. Read touch data from I2C registers
3. Update `lvgl_touch_read()` function

## Troubleshooting

### Display shows nothing
- Check TFT_eSPI configuration in `User_Setup.h`
- Verify pin connections in `config.h`
- Check backlight is enabled
- Verify SPI communication

### Touch not working
- Implement touch read function in `lvgl_touch.cpp`
- Verify touch controller type and pins
- Check touch calibration

### Out of memory errors
- Increase `LV_MEM_SIZE` in `lv_conf.h`
- Reduce display buffer size
- Disable unused LVGL features

### Slow performance
- Increase display buffer size
- Reduce refresh period
- Disable animations if not needed
- Check CPU frequency

## Resources

- [LVGL Documentation](https://docs.lvgl.io/)
- [LVGL Examples](https://github.com/lvgl/lvgl/tree/master/examples)
- [TFT_eSPI Documentation](https://github.com/Bodmer/TFT_eSPI)
- [LVGL Forum](https://forum.lvgl.io/)
