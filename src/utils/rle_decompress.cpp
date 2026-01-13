/**
 * RLE (Run-Length Encoding) Decompression Implementation
 */

#include "utils/rle_decompress.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>
#include <lvgl.h>

static const char* TAG = "RLE";

int rle_decompress(const uint8_t* compressed_data, size_t compressed_size,
                   uint8_t* output_buffer, size_t output_size) {
    if (compressed_data == NULL || output_buffer == NULL) {
        ESP_LOGE(TAG, "Invalid parameters: NULL pointer");
        return -1;
    }
    
    size_t output_pos = 0;
    size_t input_pos = 0;
    
    while (input_pos < compressed_size && output_pos < output_size) {
        if (compressed_data[input_pos] == 0xFF) {
            if (input_pos + 1 < compressed_size && compressed_data[input_pos + 1] == 0x00) {
                // Escaped literal 0xFF
                if (output_pos >= output_size) {
                    ESP_LOGE(TAG, "Output buffer overflow (escaped 0xFF)");
                    return -1;
                }
                output_buffer[output_pos++] = 0xFF;
                input_pos += 2;
            } else if (input_pos + 2 < compressed_size) {
                // RLE sequence: 0xFF, count, value
                uint8_t count = compressed_data[input_pos + 1];
                uint8_t value = compressed_data[input_pos + 2];
                
                if (output_pos + count > output_size) {
                    ESP_LOGE(TAG, "Output buffer overflow (RLE run: %d bytes)", count);
                    return -1;
                }
                
                // Write the run
                memset(&output_buffer[output_pos], value, count);
                output_pos += count;
                input_pos += 3;
            } else {
                // Malformed, treat as literal
                if (output_pos >= output_size) {
                    ESP_LOGE(TAG, "Output buffer overflow (malformed)");
                    return -1;
                }
                output_buffer[output_pos++] = compressed_data[input_pos++];
            }
        } else {
            // Literal byte
            if (output_pos >= output_size) {
                ESP_LOGE(TAG, "Output buffer overflow (literal)");
                return -1;
            }
            output_buffer[output_pos++] = compressed_data[input_pos++];
        }
    }
    
    if (output_pos != output_size) {
        ESP_LOGE(TAG, "Decompression size mismatch: expected %zu, got %zu", output_size, output_pos);
        return -1;
    }
    
    if (input_pos != compressed_size) {
        ESP_LOGW(TAG, "Not all compressed data consumed: %zu bytes remaining", compressed_size - input_pos);
    }
    
    return 0;
}

const lv_img_dsc_t* rle_decompress_image(const lv_img_dsc_t* compressed_img, size_t uncompressed_size) {
    if (compressed_img == NULL) {
        ESP_LOGE(TAG, "Invalid compressed image: NULL pointer");
        return NULL;
    }
    
    if (compressed_img->data == NULL) {
        ESP_LOGE(TAG, "Invalid compressed image: NULL data pointer");
        return NULL;
    }
    
    // Allocate buffer dynamically based on actual image size
    // This avoids wasting RAM for large static buffers
    // For the logo (259x40), this is only 20,720 bytes instead of 153,600 bytes
    static uint8_t* decompressed_buffer = NULL;
    static size_t buffer_size = 0;
    static lv_img_dsc_t decompressed_img;
    
    // Reallocate buffer if needed (grow only, never shrink to avoid fragmentation)
    if (decompressed_buffer == NULL || buffer_size < uncompressed_size) {
        if (decompressed_buffer != NULL) {
            free(decompressed_buffer);
        }
        decompressed_buffer = (uint8_t*)malloc(uncompressed_size);
        if (decompressed_buffer == NULL) {
            ESP_LOGE(TAG, "Failed to allocate %zu bytes for decompressed image", uncompressed_size);
            return NULL;
        }
        buffer_size = uncompressed_size;
        ESP_LOGI(TAG, "Allocated %zu bytes (%.1f KB) for decompressed image buffer", 
                 uncompressed_size, uncompressed_size / 1024.0);
    }
    
    // Decompress the data
    int result = rle_decompress(
        compressed_img->data,
        compressed_img->data_size,
        decompressed_buffer,
        uncompressed_size
    );
    
    if (result != 0) {
        ESP_LOGE(TAG, "Failed to decompress image data");
        return NULL;
    }
    
    // Create decompressed image descriptor
    decompressed_img.header.cf = compressed_img->header.cf;
    decompressed_img.header.w = compressed_img->header.w;
    decompressed_img.header.h = compressed_img->header.h;
    decompressed_img.header.always_zero = 0;
    decompressed_img.header.reserved = 0;
    decompressed_img.data_size = uncompressed_size;
    decompressed_img.data = decompressed_buffer;
    
    ESP_LOGI(TAG, "Decompressed image: %zu bytes -> %zu bytes (%.1f%% reduction, %.1f KB RAM)",
             compressed_img->data_size, uncompressed_size,
             (1.0 - (float)compressed_img->data_size / uncompressed_size) * 100.0,
             uncompressed_size / 1024.0);
    
    return &decompressed_img;
}

// Structure to hold streaming decompression state
typedef struct {
    const uint8_t* compressed_data;
    size_t compressed_size;
    size_t input_pos;      // Current position in compressed stream
    size_t output_pos;     // Current position in decompressed stream (virtual)
} rle_stream_ctx_t;

// Initialize streaming decompression context
static void rle_stream_init(rle_stream_ctx_t* ctx, const uint8_t* compressed_data, size_t compressed_size) {
    ctx->compressed_data = compressed_data;
    ctx->compressed_size = compressed_size;
    ctx->input_pos = 0;
    ctx->output_pos = 0;
}

// Decompress up to a target output position, returning data in buffer
// This allows us to decompress line-by-line by seeking to each line's start position
static int rle_stream_decompress_to(rle_stream_ctx_t* ctx, uint8_t* output_buffer, 
                                    size_t target_output_pos, size_t output_size) {
    // If we're already past the target, we can't seek backwards with RLE
    // So we need to decompress from the beginning each time
    // This is inefficient but memory-efficient
    
    // Reset if we need to go backwards (or if first call)
    if (ctx->output_pos > target_output_pos) {
        ctx->input_pos = 0;
        ctx->output_pos = 0;
    }
    
    // Decompress until we reach the target position
    while (ctx->output_pos < target_output_pos + output_size) {
        if (ctx->input_pos >= ctx->compressed_size) {
            break;  // End of compressed data
        }
        
        if (ctx->compressed_data[ctx->input_pos] == 0xFF) {
            if (ctx->input_pos + 1 < ctx->compressed_size && 
                ctx->compressed_data[ctx->input_pos + 1] == 0x00) {
                // Escaped literal 0xFF
                if (ctx->output_pos >= target_output_pos && 
                    ctx->output_pos < target_output_pos + output_size) {
                    output_buffer[ctx->output_pos - target_output_pos] = 0xFF;
                }
                ctx->output_pos++;
                ctx->input_pos += 2;
            } else if (ctx->input_pos + 2 < ctx->compressed_size) {
                // RLE sequence: 0xFF, count, value
                uint8_t count = ctx->compressed_data[ctx->input_pos + 1];
                uint8_t value = ctx->compressed_data[ctx->input_pos + 2];
                
                // Write the run (only if in target range)
                for (uint8_t i = 0; i < count; i++) {
                    if (ctx->output_pos >= target_output_pos && 
                        ctx->output_pos < target_output_pos + output_size) {
                        output_buffer[ctx->output_pos - target_output_pos] = value;
                    }
                    ctx->output_pos++;
                }
                ctx->input_pos += 3;
            } else {
                // Malformed, treat as literal
                if (ctx->output_pos >= target_output_pos && 
                    ctx->output_pos < target_output_pos + output_size) {
                    output_buffer[ctx->output_pos - target_output_pos] = ctx->compressed_data[ctx->input_pos];
                }
                ctx->output_pos++;
                ctx->input_pos++;
            }
        } else {
            // Literal byte
            if (ctx->output_pos >= target_output_pos && 
                ctx->output_pos < target_output_pos + output_size) {
                output_buffer[ctx->output_pos - target_output_pos] = ctx->compressed_data[ctx->input_pos];
            }
            ctx->output_pos++;
            ctx->input_pos++;
        }
    }
    
    return 0;
}

const lv_img_dsc_t* rle_get_image(const lv_img_dsc_t* img, int is_compressed, size_t uncompressed_size) {
    if (img == NULL) {
        ESP_LOGE(TAG, "Invalid image: NULL pointer");
        return NULL;
    }
    
    if (is_compressed) {
        // Decompress the full image
        // Note: True line-by-line decompression would require a custom LVGL decoder
        // which is more complex. For the logo (20KB), full decompression is acceptable.
        // The buffer is allocated dynamically and only uses the exact size needed.
        return rle_decompress_image(img, uncompressed_size);
    } else {
        // Image is not compressed, return as-is
        return img;
    }
}

int rle_decompress_region(const uint8_t* compressed_data, size_t compressed_size,
                         uint8_t* output_buffer, size_t output_size, size_t start_offset) {
    rle_stream_ctx_t ctx;
    rle_stream_init(&ctx, compressed_data, compressed_size);
    return rle_stream_decompress_to(&ctx, output_buffer, start_offset, output_size);
}

void rle_decoder_init(void) {
    // Decoder initialization - for future use with custom LVGL decoder
    ESP_LOGI(TAG, "RLE decoder support initialized");
}
