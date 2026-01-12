#!/usr/bin/env python3
#
# Copyright (c) 2026 Dynamic Devices Ltd
# All rights reserved.
#
# Create a test logo with color bars for color configuration testing
#

from PIL import Image
import os
import sys

def create_test_logo(output_path, width=280, height=80):
    """
    Create a test logo with color bars for testing color configuration
    
    Args:
        output_path: Path to output PNG image
        width: Image width in pixels
        height: Image height in pixels
    """
    try:
        # Create a new image with RGB mode
        img = Image.new('RGB', (width, height), color=(0, 0, 0))
        pixels = img.load()
        
        # Define color bars (RGB values)
        # Red, Green, Blue, Yellow, Cyan, Magenta, White, Black
        colors = [
            (255, 0, 0),      # Red
            (0, 255, 0),      # Green
            (0, 0, 255),      # Blue
            (255, 255, 0),    # Yellow
            (0, 255, 255),    # Cyan
            (255, 0, 255),    # Magenta
            (255, 255, 255),  # White
            (0, 0, 0),        # Black
        ]
        
        color_names = ["Red", "Green", "Blue", "Yellow", "Cyan", "Magenta", "White", "Black"]
        num_colors = len(colors)
        bar_width = width // num_colors
        
        # Draw color bars
        for i, color in enumerate(colors):
            x_start = i * bar_width
            x_end = (i + 1) * bar_width if i < num_colors - 1 else width
            for x in range(x_start, x_end):
                for y in range(height):
                    pixels[x, y] = color
        
        # Save the image
        img.save(output_path, 'PNG')
        print(f"✓ Created test logo: {output_path}")
        print(f"  Size: {width}x{height} pixels")
        print(f"  Colors: {', '.join(color_names)}")
        
        return width, height
    
    except Exception as e:
        print(f"Error creating test logo: {e}", file=sys.stderr)
        sys.exit(1)


def main():
    """Main entry point"""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    
    output_image = os.path.join(project_root, "resources", "test_logo.png")
    
    # Create output directory if it doesn't exist
    os.makedirs(os.path.dirname(output_image), exist_ok=True)
    
    create_test_logo(output_image)
    print(f"\nNext step: Run 'python3 tools/convert_logo.py' with test_logo.png")
    print(f"Or modify convert_logo.py to accept test_logo.png as input")


if __name__ == "__main__":
    main()
