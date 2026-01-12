#!/usr/bin/env python3
#
# Copyright (c) 2026 Dynamic Devices Ltd
# All rights reserved.
#
# Convert Precision Pour logo image to LVGL C array format (RGB565)
#

from PIL import Image
import os
import sys


def convert_to_lvgl_rgb565(input_path, output_path, var_name="precision_pour_logo"):
    """
    Convert PNG logo image to LVGL RGB565 C array format
    
    Args:
        input_path: Path to input PNG image
        output_path: Path to output C header file
        var_name: Variable name for the C array
    
    Returns:
        tuple: (width, height) of the converted image
    """
    try:
        # Open and convert image
        img = Image.open(input_path)
        
        # Convert to RGB if needed
        if img.mode != 'RGB':
            img = img.convert('RGB')
        
        width, height = img.size
        print(f"Converting logo: {width}x{height} pixels")
        
        # Get pixel data
        pixels = img.load()
        
        # Convert to RGB565 format
        # RGB565 format: RRRRR GGGGGG BBBBB (bits 15-11: R, bits 10-5: G, bits 4-0: B)
        # Note: WiFi icon colors are correct, so display driver is fine. Logo needs G and B swapped.
        # Swap G and B during conversion to fix blue/green reversal in logo image
        # When swapping: RRRRR GGGGGG BBBBB -> RRRRR BBBBB GGGGGG
        # G is 6 bits, B is 5 bits. When swapping:
        #   B (5 bits) goes to G position (6 bits wide) - pad with 0
        #   G (6 bits) goes to B position (5 bits wide) - truncate to 5 bits
        rgb565_data = []
        for y in range(height):
            for x in range(width):
                r, g, b = pixels[x, y]
                # Convert RGB888 to RGB565 with G and B swapped
                # R: 5 bits in position 15-11
                # B: 5 bits in G position (10-5, padded to 6 bits with 0)
                # G: 6 bits truncated to 5 bits in B position (4-0)
                rgb565 = ((r >> 3) << 11) | ((b >> 3) << 5) | ((g >> 3) & 0x1F)
                # Store as little-endian (2 bytes)
                rgb565_data.append(rgb565 & 0xFF)  # Low byte
                rgb565_data.append((rgb565 >> 8) & 0xFF)  # High byte
        
        # Generate C header file
        with open(output_path, 'w') as f:
            f.write("/**\n")
            f.write(f" * Precision Pour Logo\n")
            f.write(f" * Generated from: {os.path.basename(input_path)}\n")
            f.write(f" * Format: RGB565, Size: {width}x{height}\n")
            f.write(" */\n\n")
            f.write(f"#ifndef {var_name.upper()}_H\n")
            f.write(f"#define {var_name.upper()}_H\n\n")
            f.write("#include <lvgl.h>\n\n")
            f.write("#pragma GCC diagnostic push\n")
            f.write("#pragma GCC diagnostic ignored \"-Wmissing-field-initializers\"\n\n")
            
            # Write image data array
            f.write(f"const uint8_t {var_name}_data[] = {{\n")
            
            # Write data in rows of 16 bytes for readability
            for i in range(0, len(rgb565_data), 16):
                row_data = rgb565_data[i:i+16]
                hex_values = ', '.join(f'0x{b:02X}' for b in row_data)
                if i + 16 < len(rgb565_data):
                    f.write(f"    {hex_values},\n")
                else:
                    f.write(f"    {hex_values}\n")
            
            f.write("};\n\n")
            
            # Write LVGL image descriptor
            f.write(f"const lv_img_dsc_t {var_name} = {{\n")
            f.write("    .header = {\n")
            f.write("        .cf = LV_IMG_CF_TRUE_COLOR,  // RGB565 format\n")
            f.write(f"        .w = {width},\n")
            f.write(f"        .h = {height},\n")
            f.write("        // always_zero and reserved fields are automatically zero-initialized\n")
            f.write("    },\n")
            f.write(f"    .data_size = {len(rgb565_data)},\n")
            f.write(f"    .data = {var_name}_data,\n")
            f.write("};\n\n")
            f.write("#pragma GCC diagnostic pop\n\n")
            f.write(f"#endif // {var_name.upper()}_H\n")
        
        print(f"✓ Converted to: {output_path}")
        print(f"  Size: {len(rgb565_data)} bytes ({width}x{height} RGB565)")
        
        # Calculate space saved compared to full splashscreen
        splashscreen_size = 153600  # 320x240x2 bytes
        if len(rgb565_data) < splashscreen_size:
            saved = splashscreen_size - len(rgb565_data)
            percent = (saved / splashscreen_size) * 100
            print(f"  Original splashscreen: {splashscreen_size} bytes")
            print(f"  Logo size: {len(rgb565_data)} bytes")
            print(f"  Space saved: {saved} bytes ({percent:.1f}%)")
        
        return width, height
    
    except Exception as e:
        print(f"Error converting image: {e}", file=sys.stderr)
        sys.exit(1)


def main():
    """Main entry point"""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    
    input_image = os.path.join(project_root, "resources", "precision_pour_logo.png")
    output_header = os.path.join(project_root, "include", "images", "precision_pour_logo.h")
    
    # Create output directory
    os.makedirs(os.path.dirname(output_header), exist_ok=True)
    
    if not os.path.exists(input_image):
        print(f"Error: Input image not found: {input_image}", file=sys.stderr)
        print(f"Run extract_logo.py first to create the logo image", file=sys.stderr)
        sys.exit(1)
    
    convert_to_lvgl_rgb565(input_image, output_header)


if __name__ == "__main__":
    main()
