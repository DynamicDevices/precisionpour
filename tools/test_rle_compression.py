#!/usr/bin/env python3
#
# Test RLE compression on logo image to estimate space savings
#

from PIL import Image
import os
import sys

def rle_compress(data):
    """
    Simple RLE compression for RGB565 data
    Compresses runs of identical bytes
    Uses 0xFF 0x00 as escape sequence for literal 0xFF
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

def rle_decompress(compressed):
    """
    Decompress RLE-encoded data
    """
    decompressed = []
    i = 0
    
    while i < len(compressed):
        if compressed[i] == 0xFF:
            if i + 1 < len(compressed) and compressed[i + 1] == 0x00:
                # Escaped literal 0xFF
                decompressed.append(0xFF)
                i += 2
            elif i + 2 < len(compressed):
                # RLE sequence: 0xFF, count, value
                count = compressed[i + 1]
                value = compressed[i + 2]
                decompressed.extend([value] * count)
                i += 3
            else:
                # Malformed, treat as literal
                decompressed.append(compressed[i])
                i += 1
        else:
            # Literal byte
            decompressed.append(compressed[i])
            i += 1
    
    return decompressed

def analyze_image_compression(image_path):
    """
    Analyze compression potential for an image
    """
    try:
        img = Image.open(image_path)
        if img.mode != 'RGB':
            img = img.convert('RGB')
        
        width, height = img.size
        pixels = img.load()
        
        # Convert to RGB565 format (same as conversion script)
        rgb565_data = []
        for y in range(height):
            for x in range(width):
                r, g, b = pixels[x, y]
                rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
                rgb565_data.append(rgb565 & 0xFF)  # Low byte
                rgb565_data.append((rgb565 >> 8) & 0xFF)  # High byte
        
        original_size = len(rgb565_data)
        print(f"Original size: {original_size} bytes ({width}x{height} RGB565)")
        
        # Test RLE compression
        compressed = rle_compress(rgb565_data)
        compressed_size = len(compressed)
        
        # Verify decompression works
        decompressed = rle_decompress(compressed)
        if decompressed != rgb565_data:
            print("ERROR: Decompression failed!", file=sys.stderr)
            return
        
        # Calculate savings
        savings = original_size - compressed_size
        ratio = (compressed_size / original_size) * 100 if original_size > 0 else 0
        
        print(f"Compressed size: {compressed_size} bytes")
        print(f"Space saved: {savings} bytes ({100 - ratio:.1f}% reduction)")
        print(f"Compression ratio: {ratio:.1f}% of original")
        
        # Analyze run lengths
        runs = 0
        literals = 0
        i = 0
        while i < len(compressed):
            if compressed[i] == 0xFF:
                runs += 1
                i += 3
            else:
                literals += 1
                i += 1
        
        print(f"\nCompression stats:")
        print(f"  RLE runs: {runs}")
        print(f"  Literal bytes: {literals}")
        
        return {
            'original': original_size,
            'compressed': compressed_size,
            'savings': savings,
            'ratio': ratio
        }
        
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return None

def analyze_trimmed_logo():
    """
    Analyze compression on the actual trimmed logo (259x40)
    """
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    logo_path = os.path.join(project_root, "resources", "precision_pour_logo.png")
    
    if not os.path.exists(logo_path):
        return None
    
    # Load and trim the image (same as conversion script)
    img = Image.open(logo_path)
    if img.mode != 'RGB':
        img = img.convert('RGB')
    
    # Trim black borders (threshold=10, same as conversion script)
    width, height = img.size
    pixels = img.load()
    threshold = 10
    
    # Find borders
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
    
    if left < right and top < bottom:
        img = img.crop((left, top, right + 1, bottom + 1))
    
    # Convert to RGB565
    width, height = img.size
    pixels = img.load()
    rgb565_data = []
    for y in range(height):
        for x in range(width):
            r, g, b = pixels[x, y]
            rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
            rgb565_data.append(rgb565 & 0xFF)
            rgb565_data.append((rgb565 >> 8) & 0xFF)
    
    original_size = len(rgb565_data)
    compressed = rle_compress(rgb565_data)
    compressed_size = len(compressed)
    
    # Verify
    decompressed = rle_decompress(compressed)
    if decompressed != rgb565_data:
        print("ERROR: Decompression failed!", file=sys.stderr)
        return None
    
    savings = original_size - compressed_size
    ratio = (compressed_size / original_size) * 100 if original_size > 0 else 0
    
    return {
        'original': original_size,
        'compressed': compressed_size,
        'savings': savings,
        'ratio': ratio,
        'size': f"{width}x{height}"
    }

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    
    # Test on original logo image
    logo_path = os.path.join(project_root, "resources", "precision_pour_logo.png")
    
    if not os.path.exists(logo_path):
        print(f"Error: Logo image not found: {logo_path}", file=sys.stderr)
        sys.exit(1)
    
    print("=" * 60)
    print("RLE Compression Analysis")
    print("=" * 60)
    
    # Test 1: Original logo
    print(f"\n1. Original logo (280x80):")
    print(f"   Analyzing: {os.path.basename(logo_path)}")
    result1 = analyze_image_compression(logo_path)
    
    # Test 2: Trimmed logo (current)
    print(f"\n2. Trimmed logo (current 259x40):")
    result2 = analyze_trimmed_logo()
    if result2:
        print(f"   Size: {result2['size']}")
        print(f"   Original: {result2['original']:,} bytes")
        print(f"   Compressed: {result2['compressed']:,} bytes")
        print(f"   Savings: {result2['savings']:,} bytes ({100 - result2['ratio']:.1f}% reduction)")
        print(f"   Compression ratio: {result2['ratio']:.1f}% of original")
    
    # Summary
    print("\n" + "=" * 60)
    print("Summary:")
    if result1:
        print(f"  Original logo (280x80):")
        print(f"    {result1['original']:,} bytes -> {result1['compressed']:,} bytes")
        print(f"    Savings: {result1['savings']:,} bytes ({100 - result1['ratio']:.1f}%)")
    if result2:
        print(f"  Trimmed logo ({result2['size']}):")
        print(f"    {result2['original']:,} bytes -> {result2['compressed']:,} bytes")
        print(f"    Savings: {result2['savings']:,} bytes ({100 - result2['ratio']:.1f}%)")
    print("=" * 60)

if __name__ == "__main__":
    main()
