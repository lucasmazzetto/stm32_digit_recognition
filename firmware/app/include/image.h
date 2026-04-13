#ifndef IMAGE_H
#define IMAGE_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Number of fractional bits in fixed-point coordinates: 10 bits balances precision and MCU cost
#define IMAGE_BILINEAR_FRAC_BITS 10U

// Fixed-point unit scale (2^frac_bits): used to represent interpolation weights without float math
#define IMAGE_BILINEAR_SCALE (1U << IMAGE_BILINEAR_FRAC_BITS)

// Mask to extract only the fractional part from a fixed-point coordinate
#define IMAGE_BILINEAR_FRACTION_MASK (IMAGE_BILINEAR_SCALE - 1U)

// Half-LSB at bilinear accumulator scale: added before right shift for rounded output
#define IMAGE_BILINEAR_ROUNDING (1U << ((2U * IMAGE_BILINEAR_FRAC_BITS) - 1U))

/**
 * @brief Converts packed YUV422 to grayscale in-place using a selected phase.
 *
 * This routine assumes all arguments are valid.
 *
 * @param frame_buffer Input/output frame buffer.
 * @param input_size Input size in bytes.
 * @param phase Byte phase selector (`0` or `1`).
 * @return Number of grayscale bytes written.
 */
uint32_t image_yuv422_to_grayscale(uint8_t *frame_buffer, uint32_t input_size,
                                   uint8_t phase);

/**
 * @brief Center-crops an in-memory grayscale image in-place.
 *
 * The input buffer must store at least `source_width * source_height` bytes in
 * row-major order. The cropped output is compacted at the beginning of the
 * same buffer.
 * This routine assumes all arguments are valid.
 *
 * @param frame_buffer Input/output grayscale buffer.
 * @param source_width Source image width in pixels.
 * @param source_height Source image height in pixels.
 * @param crop_width Target crop width in pixels.
 * @param crop_height Target crop height in pixels.
 * @return Number of cropped bytes written.
 */
uint32_t image_grayscale_crop_center(uint8_t *frame_buffer,
                                     uint16_t source_width,
                                     uint16_t source_height,
                                     uint16_t crop_width,
                                     uint16_t crop_height);

/**
 * @brief Resizes a grayscale image with integer bilinear interpolation.
 *
 * This routine reads pixels from `source_buffer` and writes the resized image
 * into `target_buffer`, both in row-major order and one byte per pixel.
 *
 * It uses fixed-point interpolation (no floating point), which is a good fit
 * for MCU targets while still producing smoother results than nearest-neighbor.
 * This routine assumes all arguments are valid.
 *
 * @param source_buffer Source grayscale image.
 * @param source_width Source image width in pixels.
 * @param source_height Source image height in pixels.
 * @param target_buffer Destination buffer for resized image.
 * @param target_width Destination image width in pixels.
 * @param target_height Destination image height in pixels.
 * @return Number of bytes written in target buffer.
 */
uint32_t image_grayscale_resize(const uint8_t *source_buffer,
                                uint16_t source_width, uint16_t source_height,
                                uint8_t *target_buffer, uint16_t target_width,
                                uint16_t target_height);

#endif /* IMAGE_H */
