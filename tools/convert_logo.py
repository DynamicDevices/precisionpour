#!/usr/bin/env python3
#
# Copyright (c) 2026 Dynamic Devices Ltd
# All rights reserved.
#
# Convert Precision Pour logo image to LVGL C array format (RGB565)
# Supports optional RLE compression
#

from PIL import Image
import os
import sys
import argparse


def trim_black_borders(img, threshold=10):
    """
    Trim black borders from image
    
    Args:
        img: PIL Image object
        threshold: RGB threshold below which pixels are considered black (default: 10)
    
    Returns:
        PIL Image: Trimmed image
    """
    # Convert to RGB if needed
    if img.mode != 'RGB':
        img = img.convert('RGB')
    
    # Get image dimensions
    width, height = img.size
    pixels = img.load()
    
    # Find top border
    top = 0
    for y in range(height):
        for x in range(width):
            r, g, b = pixels[x, y]
            if r > threshold or g > threshold or b > threshold:
                top = y
                break
        else:
            continue
        break
    
    # Find bottom border
    bottom = height - 1
    for y in range(height - 1, -1, -1):
        for x in range(width):
            r, g, b = pixels[x, y]
            if r > threshold or g > threshold or b > threshold:
                bottom = y
                break
        else:
            continue
        break
    
    # Find left border
    left = 0
    for x in range(width):
        for y in range(height):
            r, g, b = pixels[x, y]
            if r > threshold or g > threshold or b > threshold:
                left = x
                break
        else:
            continue
        break
    
    # Find right border
    right = width - 1
    for x in range(width - 1, -1, -1):
        for y in range(height):
            r, g, b = pixels[x, y]
            if r > threshold or g > threshold or b > threshold:
                right = x
                break
        else:
            continue
        break
    
    # Crop the image
    if left < right and top < bottom:
        trimmed = img.crop((left, top, right + 1, bottom + 1))
        print(f"  Trimmed borders: {width}x{height} -> {trimmed.size[0]}x{trimmed.size[1]}")
        print(f"  Removed: {width - trimmed.size[0]}px width, {height - trimmed.size[1]}px height")
        return trimmed
    else:
        print(f"  No trimming needed (image is all black or empty)")
        return img


def rle_compress(data):
    """
    Simple RLE compression for RGB565 data
    Compresses runs of identical bytes
    Uses 0xFF 0x00 as escape sequence for literal 0xFF
    
    Args:
        data: List of bytes to compress
    
    Returns:
        List of compressed bytes
    """
    if not data:
        return []
    
    compressed = []
    i = 0
    
    while i < len(data):
        # Count consecutive identical bytes
        byte_val = data[i]
        count = 1
        
        # Count run (max 255 for single byte encoding)
        while i + count < len(data) and data[i + count] == byte_val and count < 255:
            count += 1
        
        if count >= 4:  # Only compress if we save at least 1 byte (4 bytes -> 3 bytes)
            # Use RLE encoding: 0xFF marker, count, value
            compressed.append(0xFF)  # RLE marker
            compressed.append(count)
            compressed.append(byte_val)
            i += count
        else:
            # Store literal bytes (escape 0xFF as 0xFF 0x00)
            for j in range(count):
                if data[i + j] == 0xFF:
                    compressed.append(0xFF)
                    compressed.append(0x00)  # Escape sequence
                else:
                    compressed.append(data[i + j])
            i += count
    
    return compressed


def convert_to_lvgl_rgb565(input_path, output_path, var_name="precision_pour_logo", trim_borders=True, use_rle=False):
    """
    Convert PNG logo image to LVGL RGB565 C array format
    
    Args:
        input_path: Path to input PNG image
        output_path: Path to output C header file
        var_name: Variable name for the C array
        trim_borders: If True, trim black borders from the image
        use_rle: If True, apply RLE compression to the image data
    
    Returns:
        tuple: (width, height) of the converted image
    """
    try:
        # Open and convert image
        img = Image.open(input_path)
        
        # Convert to RGB if needed
        if img.mode != 'RGB':
            img = img.convert('RGB')
        
        original_width, original_height = img.size
        print(f"Original logo: {original_width}x{original_height} pixels")
        
        # Trim black borders if requested
        if trim_borders:
            img = trim_black_borders(img)
        
        width, height = img.size
        print(f"Converting logo: {width}x{height} pixels")
        
        # Get pixel data
        pixels = img.load()
        
        # Convert to RGB565 format
        # RGB565 format: RRRRR GGGGGG BBBBB (bits 15-11: R, bits 10-5: G, bits 4-0: B)
        # Display is configured for BGR mode, so colors display correctly without swapping
        rgb565_data = []
        for y in range(height):
            for x in range(width):
                r, g, b = pixels[x, y]
                # Convert RGB888 to RGB565 (standard conversion, no G/B swap)
                # R: 5 bits in position 15-11
                # G: 6 bits in position 10-5
                # B: 5 bits in position 4-0
                rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
                # Store as little-endian (2 bytes)
                rgb565_data.append(rgb565 & 0xFF)  # Low byte
                rgb565_data.append((rgb565 >> 8) & 0xFF)  # High byte
        
        # Apply RLE compression if requested
        original_size = len(rgb565_data)
        is_compressed = False
        if use_rle:
            compressed_data = rle_compress(rgb565_data)
            if len(compressed_data) < original_size:
                rgb565_data = compressed_data
                is_compressed = True
                savings = original_size - len(compressed_data)
                savings_pct = (savings / original_size) * 100
                print(f"  RLE compression: {original_size} bytes -> {len(compressed_data)} bytes")
                print(f"  Space saved: {savings} bytes ({savings_pct:.1f}% reduction)")
            else:
                print(f"  RLE compression would increase size, using uncompressed data")
                is_compressed = False
        
        # Generate C header file
        with open(output_path, 'w') as f:
            f.write("/**\n")
            f.write(f" * Precision Pour Logo\n")
            f.write(f" * Generated from: {os.path.basename(input_path)}\n")
            f.write(f" * Format: RGB565, Size: {width}x{height}\n")
            if is_compressed:
                f.write(f" * Compression: RLE (compressed size: {len(rgb565_data)} bytes, original: {original_size} bytes)\n")
            f.write(" */\n\n")
            f.write(f"#ifndef {var_name.upper()}_H\n")
            f.write(f"#define {var_name.upper()}_H\n\n")
            f.write("#include <lvgl.h>\n\n")
            if is_compressed:
                f.write("// RLE-compressed image data\n")
                f.write("// Use rle_decompress_image() to decompress before use\n")
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
            
            if is_compressed:
                # Store original uncompressed size for decompression
                f.write(f"// Original uncompressed size (for decompression)\n")
                f.write(f"#define {var_name.upper()}_UNCOMPRESSED_SIZE ({original_size})\n\n")
                f.write(f"// Compression flag\n")
                f.write(f"#define {var_name.upper()}_IS_COMPRESSED (1)\n\n")
            else:
                f.write(f"#define {var_name.upper()}_IS_COMPRESSED (0)\n\n")
            
            # Write LVGL image descriptor
            # Note: For compressed images, data_size is the compressed size
            # The actual decompressed size is stored in the define above
            f.write(f"const lv_img_dsc_t {var_name} = {{\n")
            f.write("    .header = {\n")
            f.write("        .cf = LV_IMG_CF_TRUE_COLOR,  // RGB565 format\n")
            f.write(f"        .w = {width},\n")
            f.write(f"        .h = {height},\n")
            f.write("        // always_zero and reserved fields are automatically zero-initialized\n")
            f.write("    },\n")
            if is_compressed:
                f.write(f"    .data_size = {len(rgb565_data)},  // Compressed size\n")
            else:
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
    parser = argparse.ArgumentParser(
        description='Convert PNG logo image to LVGL RGB565 C array format',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s                    # Convert without compression
  %(prog)s --rle               # Convert with RLE compression
  %(prog)s --no-trim           # Convert without border trimming
  %(prog)s --rle --no-trim     # RLE compression without trimming
        """
    )
    parser.add_argument(
        '--rle',
        action='store_true',
        help='Apply RLE compression to reduce memory usage'
    )
    parser.add_argument(
        '--no-trim',
        action='store_true',
        help='Skip automatic black border trimming'
    )
    parser.add_argument(
        '--input',
        type=str,
        help='Input PNG image path (default: resources/precision_pour_logo.png)'
    )
    parser.add_argument(
        '--output',
        type=str,
        help='Output C header file path (default: include/images/precision_pour_logo.h)'
    )
    
    args = parser.parse_args()
    
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    
    input_image = args.input or os.path.join(project_root, "resources", "precision_pour_logo.png")
    output_header = args.output or os.path.join(project_root, "include", "images", "precision_pour_logo.h")
    
    # Create output directory
    os.makedirs(os.path.dirname(output_header), exist_ok=True)
    
    if not os.path.exists(input_image):
        print(f"Error: Input image not found: {input_image}", file=sys.stderr)
        print(f"Run extract_logo.py first to create the logo image", file=sys.stderr)
        sys.exit(1)
    
    convert_to_lvgl_rgb565(
        input_image, 
        output_header,
        trim_borders=not args.no_trim,
        use_rle=args.rle
    )


if __name__ == "__main__":
    main()
