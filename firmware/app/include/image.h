#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>

/**
 * @brief Converts YUV422 in-place to grayscale using the selected byte phase.
 *
 * For packed YUV422, valid phases are:
 * - `0`: bytes `0,2,4,...`
 * - `1`: bytes `1,3,5,...`
 *
 * The grayscale output is compacted at the beginning of the same buffer.
 *
 * @param frame_buffer Input/output frame buffer.
 * @param input_size Input buffer size in bytes.
 * @param phase Byte phase selector (`0` or `1`). Any other value returns 0.
 * @return Number of grayscale bytes written, or 0 on invalid arguments.
 */
uint32_t image_yuv422_to_grayscale(uint8_t *frame_buffer,
                                   uint32_t input_size,
                                   uint8_t phase);

#endif /* IMAGE_H */
