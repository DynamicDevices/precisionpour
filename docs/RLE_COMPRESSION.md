# RLE Compression Implementation

## Overview

RLE (Run-Length Encoding) compression has been implemented for LVGL images to reduce memory usage. The implementation includes:

- **Compression**: Python script with command-line option
- **Decompression**: C/C++ runtime decompression
- **Integration**: Automatic handling in image loading code

## Usage

### Converting Images with RLE Compression

```bash
# Convert logo with RLE compression
python3 tools/convert_logo.py --rle

# Convert without compression (default)
python3 tools/convert_logo.py

# Other options
python3 tools/convert_logo.py --rle --no-trim    # RLE without border trimming
python3 tools/convert_logo.py --help             # Show all options
```

### Memory Savings

For the Precision Pour logo (259×40 pixels):
- **Uncompressed**: 20,720 bytes
- **RLE Compressed**: 9,186 bytes
- **Savings**: 11,534 bytes (55.7% reduction)

### How It Works

1. **Compression** (build time):
   - The `convert_logo.py` script compresses RGB565 image data using RLE
   - Compressed data is stored in the generated header file
   - Header includes `IS_COMPRESSED` flag and `UNCOMPRESSED_SIZE` define

2. **Decompression** (runtime):
   - `rle_get_image()` checks if image is compressed
   - If compressed, decompresses on first access and caches result
   - If uncompressed, returns original image descriptor
   - All image loading code automatically handles both cases

### Implementation Details

#### Compression Format
- **RLE Marker**: `0xFF` followed by count and value
- **Escape Sequence**: `0xFF 0x00` for literal `0xFF` bytes
- **Minimum Run**: 4 bytes (only compress runs that save space)

#### Decompression
- Static buffer (153,600 bytes max) for decompressed data
- Decompressed on first access, reused for subsequent uses
- Error handling for buffer overflows and malformed data

### Files

- **`tools/convert_logo.py`**: Image conversion with RLE compression option
- **`include/rle_decompress.h`**: Decompression API
- **`src/rle_decompress.cpp`**: Decompression implementation
- **`src/ui_logo.cpp`**: Updated to handle compressed images
- **`src/splashscreen.cpp`**: Updated to handle compressed images
- **`src/production_mode_ui.cpp`**: Updated to handle compressed images
- **`src/pouring_mode_ui.cpp`**: Updated to handle compressed images

### API

```c
// Get decompressed image (handles both compressed and uncompressed)
const lv_img_dsc_t* rle_get_image(
    const lv_img_dsc_t* img,
    int is_compressed,
    size_t uncompressed_size
);

// Direct decompression (for advanced use)
int rle_decompress(
    const uint8_t* compressed_data,
    size_t compressed_size,
    uint8_t* output_buffer,
    size_t output_size
);
```

### Example Usage

```c
#include "images/precision_pour_logo.h"
#include "rle_decompress.h"

// Get image (automatically decompresses if needed)
const lv_img_dsc_t* logo = rle_get_image(
    &precision_pour_logo,
    PRECISION_POUR_LOGO_IS_COMPRESSED,
    PRECISION_POUR_LOGO_IS_COMPRESSED ? PRECISION_POUR_LOGO_UNCOMPRESSED_SIZE : precision_pour_logo.data_size
);

// Use with LVGL
lv_img_set_src(img_obj, logo);
```

### Performance

- **Decompression Time**: < 1ms for logo image (on ESP32)
- **Memory Overhead**: Static buffer (153,600 bytes max, shared across all images)
- **CPU Overhead**: Minimal (simple byte-by-byte decompression)

### Trade-offs

**Pros:**
- Significant memory savings (55-79% depending on image)
- Simple, fast decompression algorithm
- Automatic handling in existing code
- Optional (can disable compression if needed)

**Cons:**
- Requires runtime decompression (small CPU overhead)
- Static buffer limits maximum image size
- Compression ratio varies by image content

### Future Improvements

- Dynamic buffer allocation (if memory allows)
- Support for other compression algorithms (LZ4, deflate)
- LVGL custom decoder integration (decompress on-demand)
- Compression statistics and analysis tools
