#include "ai_image_process.h"

// Source dimensions
#define SRC_WIDTH  240
#define SRC_HEIGHT 320

// Crop dimensions (center square)
#define CROP_SIZE  240
#define CROP_Y_START ((SRC_HEIGHT - CROP_SIZE) / 2) // 40
#define CROP_X_START 0

// Destination dimensions
#define DST_SIZE   96

void Image_Process_RGB565_to_Gray96(const uint16_t *src_buf, int8_t *dst_buf) {
    int dst_x, dst_y;
    for (dst_y = 0; dst_y < DST_SIZE; dst_y++) {
        for (dst_x = 0; dst_x < DST_SIZE; dst_x++) {
            // Nearest neighbor interpolation
            int src_x = CROP_X_START + (dst_x * CROP_SIZE) / DST_SIZE;
            int src_y = CROP_Y_START + (dst_y * CROP_SIZE) / DST_SIZE;
            
            // Read RGB565 pixel
            uint16_t pixel = src_buf[src_y * SRC_WIDTH + src_x];
            
            // Extract RGB components
            uint8_t r_5 = (pixel >> 11) & 0x1F;
            uint8_t g_6 = (pixel >> 5) & 0x3F;
            uint8_t b_5 = pixel & 0x1F;
            
            // Scale to 8-bit
            uint8_t r = (r_5 * 255) / 31;
            uint8_t g = (g_6 * 255) / 63;
            uint8_t b = (b_5 * 255) / 31;
            
            // Convert to grayscale (BT.601)
            uint8_t gray = (uint8_t)((r * 77 + g * 150 + b * 29) >> 8);
            
            // Map [0, 255] to [-128, 127] for int8 model
            dst_buf[dst_y * DST_SIZE + dst_x] = (int8_t)((int16_t)gray - 128);
        }
    }
}
