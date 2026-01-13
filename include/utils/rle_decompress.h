/**
 * RLE (Run-Length Encoding) Decompression Utility
 * 
 * Decompresses RLE-compressed image data for LVGL images
 * Format: 0xFF marker, count, value for runs >= 4 bytes
 *         0xFF 0x00 escape sequence for literal 0xFF
 */

#ifndef RLE_DECOMPRESS_H
#define RLE_DECOMPRESS_H

#include <stdint.h>
#include <stddef.h>
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Decompress RLE-encoded data
 * 
 * @param compressed_data Pointer to compressed data
 * @param compressed_size Size of compressed data in bytes
 * @param output_buffer Buffer to store decompressed data (must be pre-allocated)
 * @param output_size Size of output buffer (must match uncompressed size)
 * @return 0 on success, -1 on error
 */
int rle_decompress(const uint8_t* compressed_data, size_t compressed_size,
                   uint8_t* output_buffer, size_t output_size);

/**
 * Decompress RLE image and create LVGL image descriptor
 * 
 * This function decompresses RLE-compressed image data and creates a new
 * LVGL image descriptor with the decompressed data. The decompressed data
 * is stored in a static buffer (decompressed on first call, reused on subsequent calls).
 * 
 * @param compressed_img Pointer to compressed LVGL image descriptor
 * @param uncompressed_size Expected uncompressed size in bytes
 * @return Pointer to decompressed LVGL image descriptor, or NULL on error
 */
const lv_img_dsc_t* rle_decompress_image(const lv_img_dsc_t* compressed_img, size_t uncompressed_size);

/**
 * Get decompressed image descriptor (handles both compressed and uncompressed images)
 * 
 * This is a convenience function that checks if an image is compressed and
 * decompresses it if needed. For uncompressed images, returns the original descriptor.
 * 
 * @param img Pointer to LVGL image descriptor (may be compressed or uncompressed)
 * @param is_compressed Flag indicating if image is RLE-compressed (from header define)
 * @param uncompressed_size Uncompressed size in bytes (only used if is_compressed is true)
 * @return Pointer to decompressed/uncompressed LVGL image descriptor, or NULL on error
 */
const lv_img_dsc_t* rle_get_image(const lv_img_dsc_t* img, int is_compressed, size_t uncompressed_size);

/**
 * Initialize RLE decoder for LVGL
 * 
 * Registers a custom LVGL decoder that decompresses RLE images on-demand,
 * line-by-line, to minimize RAM usage.
 * 
 * Call this once during initialization (e.g., in lvgl_display_init).
 */
void rle_decoder_init(void);

/**
 * Decompress a specific region of an RLE-compressed image
 * 
 * This function decompresses only the requested region (line/area) from the
 * compressed image, using a small temporary buffer.
 * 
 * @param compressed_data Pointer to compressed data
 * @param compressed_size Size of compressed data
 * @param output_buffer Buffer to store decompressed region (must be pre-allocated)
 * @param output_size Size of output buffer (size of requested region)
 * @param start_offset Byte offset in uncompressed image where region starts
 * @return 0 on success, -1 on error
 */
int rle_decompress_region(const uint8_t* compressed_data, size_t compressed_size,
                         uint8_t* output_buffer, size_t output_size, size_t start_offset);

#ifdef __cplusplus
}
#endif

#endif // RLE_DECOMPRESS_H
