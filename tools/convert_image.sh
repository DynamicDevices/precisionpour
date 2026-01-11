#!/bin/bash
# Image Conversion Script for LVGL
# Converts PNG images to LVGL-compatible C arrays
# Requires ImageMagick and LVGL image converter

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
RESOURCES_DIR="$PROJECT_ROOT/resources"
OUTPUT_DIR="$PROJECT_ROOT/include/images"

# Create output directory
mkdir -p "$OUTPUT_DIR"

INPUT_IMAGE="$RESOURCES_DIR/splashscreen.png"
OUTPUT_FILE="$OUTPUT_DIR/splashscreen_img.h"

if [ ! -f "$INPUT_IMAGE" ]; then
    echo "Error: Input image not found: $INPUT_IMAGE"
    exit 1
fi

echo "Converting image for LVGL..."
echo "Input: $INPUT_IMAGE"
echo "Output: $OUTPUT_FILE"

# Check if ImageMagick is installed
if ! command -v convert &> /dev/null; then
    echo "Warning: ImageMagick not found. Install with: sudo apt install imagemagick"
    echo "You can manually convert the image using LVGL's online converter:"
    echo "https://lvgl.io/tools/imageconverter"
    exit 1
fi

# Resize to display resolution (320x240)
TEMP_RESIZED="/tmp/splashscreen_resized.png"
convert "$INPUT_IMAGE" -resize 320x240^ -gravity center -extent 320x240 -format PNG32 "$TEMP_RESIZED"

echo ""
echo "Image resized to 320x240"
echo ""
echo "To convert to LVGL C array, use one of these methods:"
echo ""
echo "1. Online converter (recommended):"
echo "   https://lvgl.io/tools/imageconverter"
echo "   - Upload: $TEMP_RESIZED"
echo "   - Color format: RGB565"
echo "   - Output format: C array"
echo "   - Save as: $OUTPUT_FILE"
echo ""
echo "2. Python script (if you have LVGL tools installed):"
echo "   python3 -m lvgl.img_conv $TEMP_RESIZED -f RGB565 -o $OUTPUT_FILE"
echo ""
echo "3. Copy resized image to data/ folder for filesystem loading:"
echo "   cp $TEMP_RESIZED $PROJECT_ROOT/data/splashscreen.png"
echo "   Then upload filesystem: pio run -t uploadfs"
echo ""

# Clean up
rm -f "$TEMP_RESIZED"
