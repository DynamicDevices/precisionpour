# Splashscreen Setup Guide

## Overview

The splashscreen displays the "Precision Pour" image during firmware startup. The image is located at `resources/splashscreen.png`.

## Image Specifications

- **Original**: 1024x1024 PNG (RGBA)
- **Display**: 320x240 (RGB565)
- **File size**: ~1.2MB (original)

## Conversion Methods

### Method 1: Online Converter (Recommended)

1. **Resize the image first** (optional, but recommended):
   ```bash
   convert resources/splashscreen.png -resize 320x240^ -gravity center -extent 320x240 resources/splashscreen_320x240.png
   ```

2. **Use LVGL Image Converter**:
   - Go to: https://lvgl.io/tools/imageconverter
   - Upload: `resources/splashscreen_320x240.png`
   - Settings:
     - Color format: **RGB565**
     - Output format: **C array**
   - Download the generated `.h` file
   - Save as: `include/images/splashscreen_img.h`

3. **Update code**:
   - In `src/splashscreen.cpp`, uncomment the embedded image code:
   ```cpp
   extern const lv_img_dsc_t splashscreen_img_dsc;
   lv_img_set_src(splashscreen_img, &splashscreen_img_dsc);
   ```

### Method 2: Filesystem Loading

1. **Resize and copy to data folder**:
   ```bash
   convert resources/splashscreen.png -resize 320x240^ -gravity center -extent 320x240 -format PNG32 data/splashscreen.png
   ```

2. **Upload filesystem**:
   ```bash
   pio run -t uploadfs
   ```

3. **Update code**:
   - In `src/splashscreen.cpp`, uncomment:
   ```cpp
   lv_img_set_src(splashscreen_img, "S:/splashscreen.png");
   ```

### Method 3: Python Script (Advanced)

If you have LVGL tools installed:
```bash
python3 -m lvgl.img_conv resources/splashscreen.png -f RGB565 -o include/images/splashscreen_img.h
```

## Integration

The splashscreen is integrated in `src/main.cpp`:

```cpp
void setup() {
    // ... initialization ...
    
    splashscreen_init();  // Show splashscreen
    
    // ... other setup ...
    
    // After initialization complete:
    delay(2000);  // Show splashscreen for 2 seconds
    splashscreen_remove();  // Remove and show main UI
    ui_init();  // Initialize main UI
}
```

## Performance Considerations

- **Embedded (C array)**: Fastest, uses flash memory, no filesystem needed
- **Filesystem**: Slower load time, uses RAM, allows dynamic updates
- **Recommended**: Use embedded for splashscreen (shown once at startup)

## Troubleshooting

### Image not displaying
- Check image format (must be RGB565 for embedded)
- Verify file path if using filesystem
- Check LVGL memory allocation in `lv_conf.h`

### Image too large
- Resize to match display resolution (320x240)
- Use RGB565 color format (reduces size by 50% vs RGBA)
- Consider compression if using filesystem

### Out of memory
- Reduce image resolution
- Use RGB565 instead of RGBA
- Increase `LV_MEM_SIZE` in `lv_conf.h`

## Current Status

The splashscreen code is ready but uses a placeholder (black background with text). To use the actual image:

1. Convert `resources/splashscreen.png` using Method 1 or 2 above
2. Update `src/splashscreen.cpp` to load the converted image
3. Rebuild and upload
