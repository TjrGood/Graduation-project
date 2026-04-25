#ifndef __AI_IMAGE_PROCESS_H
#define __AI_IMAGE_PROCESS_H

#include <stdint.h>

// Convert 240x320 RGB565 to 96x96 Grayscale (int8 for TFLite)
void Image_Process_RGB565_to_Gray96(const uint16_t *src_buf, int8_t *dst_buf);

#endif // __AI_IMAGE_PROCESS_H
