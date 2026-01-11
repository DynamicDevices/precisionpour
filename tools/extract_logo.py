#!/usr/bin/env python3
"""
Extract Precision Pour logo from full-screen image and create a compact logo version
The logo is in the center-top area of the image, we'll crop it and add black background
"""

from PIL import Image
import os
import sys

def extract_logo(input_path, output_path, var_name="precision_pour_logo"):
    """Extract logo from full-screen image and create compact version"""
    
    # Open original image
    img = Image.open(input_path)
    width, height = img.size
    print(f"Original image: {width}x{height} pixels")
    
    # The logo is in the center-top area
    # Based on the image description, the logo with text is roughly in the top 40% of the image
    # We'll crop the top portion and center it
    
    # Crop the top portion where the logo is (approximately top 40% of height)
    logo_height = int(height * 0.4)  # Top 40% of image
    logo_width = width  # Full width
    
    # Crop from top-left: (0, 0) to (width, logo_height)
    logo_crop = img.crop((0, 0, logo_width, logo_height))
    
    print(f"Cropped logo area: {logo_width}x{logo_height}")
    
    # Create a new image with black background
    # Make it a reasonable size for a logo (e.g., 200px wide, maintain aspect ratio)
    target_width = 200
    target_height = int((logo_height * target_width) / logo_width)
    
    # Resize the cropped logo
    logo_resized = logo_crop.resize((target_width, target_height), Image.Resampling.LANCZOS)
    
    print(f"Resized logo: {target_width}x{target_height}")
    
    # Create final image with black background (slightly larger for padding)
    padding = 10
    final_width = target_width + (padding * 2)
    final_height = target_height + (padding * 2)
    
    final_img = Image.new('RGB', (final_width, final_height), (0, 0, 0))  # Black background
    
    # Paste the logo in the center
    paste_x = padding
    paste_y = padding
    final_img.paste(logo_resized, (paste_x, paste_y))
    
    print(f"Final logo image: {final_width}x{final_height} (with {padding}px padding)")
    
    # Save the logo image
    final_img.save(output_path, 'PNG')
    print(f"✓ Logo saved to: {output_path}")
    
    return final_width, final_height

if __name__ == "__main__":
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    
    input_image = os.path.join(project_root, "resources", "precision_pour_original.png")
    output_image = os.path.join(project_root, "resources", "precision_pour_logo.png")
    
    if not os.path.exists(input_image):
        print(f"Error: Input image not found: {input_image}")
        sys.exit(1)
    
    extract_logo(input_image, output_image)
    print(f"\nNext step: Convert logo to LVGL format:")
    print(f"  python3 {os.path.join(script_dir, 'convert_logo.py')}")
