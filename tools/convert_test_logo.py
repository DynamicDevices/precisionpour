#!/usr/bin/env python3
#
# Copyright (c) 2026 Dynamic Devices Ltd
# All rights reserved.
#
# Convert test logo image to LVGL C array format (RGB565)
# This is a copy of convert_logo.py but for the test logo
#

from PIL import Image
import os
import sys

# Import the conversion function from convert_logo.py
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from convert_logo import convert_to_lvgl_rgb565


def main():
    """Main entry point"""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    
    input_image = os.path.join(project_root, "resources", "test_logo.png")
    output_header = os.path.join(project_root, "include", "images", "test_logo.h")
    
    # Create output directory
    os.makedirs(os.path.dirname(output_header), exist_ok=True)
    
    if not os.path.exists(input_image):
        print(f"Error: Input image not found: {input_image}", file=sys.stderr)
        print(f"Run create_test_logo.py first to create the test logo image", file=sys.stderr)
        sys.exit(1)
    
    convert_to_lvgl_rgb565(input_image, output_header, "test_logo")


if __name__ == "__main__":
    main()
