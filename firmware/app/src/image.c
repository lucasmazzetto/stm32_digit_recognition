#include "image.h"

#include <stddef.h>

/**
 * @brief Converts packed YUV422 to grayscale in-place using a selected phase.
 *
 * @param frame_buffer Input/output frame buffer.
 * @param input_size Input size in bytes (must be even).
 * @param phase Byte phase selector (0 or 1).
 * @return Number of grayscale bytes written, or 0 on invalid input.
 */
uint32_t image_yuv422_to_grayscale(uint8_t *frame_buffer,
                                   uint32_t input_size,
                                   uint8_t phase)
{
    uint32_t input_index;
    uint32_t output_index;

    if ((frame_buffer == NULL) || (input_size == 0U)) {
        return 0U;
    }

    if ((input_size & 0x1U) != 0U) {
        // YUV422 must have an even number of bytes
        return 0U;
    }

    if ((phase != 0U) && (phase != 1U)) {
        return 0U;
    }

    // Compact selected phase at the start of the same buffer
    // Safe in-place: output_index grows by 1 while input_index grows by 2
    output_index = 0U;

    for (input_index = phase; input_index < input_size; input_index += 2U) {
        frame_buffer[output_index] = frame_buffer[input_index];
        output_index += 1U;
    }

    return output_index;
}
