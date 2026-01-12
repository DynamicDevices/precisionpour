#!/bin/bash
#
# Copyright (c) 2026 Dynamic Devices Ltd
# All rights reserved.
#
# Image Conversion Script for LVGL
# Converts PNG images to LVGL-compatible C arrays
# Requires ImageMagick and LVGL image converter
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
RESOURCES_DIR="${PROJECT_ROOT}/resources"
OUTPUT_DIR="${PROJECT_ROOT}/include/images"

# Create output directory
mkdir -p "$OUTPUT_DIR"

INPUT_IMAGE="${RESOURCES_DIR}/splashscreen.png"
OUTPUT_FILE="${OUTPUT_DIR}/splashscreen_img.h"
TEMP_RESIZED="/tmp/splashscreen_resized.png"

# Check if input image exists
if [ ! -f "$INPUT_IMAGE" ]; then
    echo "Error: Input image not found: $INPUT_IMAGE" >&2
    exit 1
fi

echo "Converting image for LVGL..."
echo "Input: $INPUT_IMAGE"
echo "Output: $OUTPUT_FILE"
echo ""

# Check if ImageMagick is installed
if ! command -v convert &> /dev/null; then
    echo "Error: ImageMagick not found." >&2
    echo "Install with: sudo apt install imagemagick" >&2
    echo "" >&2
    echo "You can manually convert the image using LVGL's online converter:" >&2
    echo "https://lvgl.io/tools/imageconverter" >&2
    exit 1
fi

# Resize to display resolution (320x240)
echo "Resizing image to 320x240..."
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
