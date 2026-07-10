#include "image.h"

uint32_t image_yuv422_to_grayscale(uint8_t *frame_buffer, uint32_t input_size,
                                   uint8_t phase)
{
    // Safe in-place: output_index grows by 1 while input_index grows by 2
    uint32_t output_index = 0U;

    for (uint32_t input_index = phase; input_index < input_size;
         input_index += 2U) {
        frame_buffer[output_index] = frame_buffer[input_index];
        output_index += 1U;
    }

    return output_index;
}

uint32_t image_grayscale_resize(const uint8_t *source_buffer,
                                uint16_t source_width, uint16_t source_height,
                                uint8_t *target_buffer, uint16_t target_width,
                                uint16_t target_height)
{
    const uint32_t source_width_u32 = (uint32_t)source_width;
    const uint32_t source_height_u32 = (uint32_t)source_height;
    const uint32_t target_width_u32 = (uint32_t)target_width;
    const uint32_t target_height_u32 = (uint32_t)target_height;
    const uint32_t target_pixels = target_width_u32 * target_height_u32;

    uint32_t x_denominator = 1U;
    uint32_t y_denominator = 1U;
    uint32_t x_scale = 0U;
    uint32_t y_scale = 0U;

    // Use corner-aligned mapping
    // Target min/max coordinates map to source min/max
    if (target_width_u32 > 1U) {
        x_denominator = target_width_u32 - 1U;
    }

    // Use denominator 1 when target height is 1 to avoid divide-by-zero
    if (target_height_u32 > 1U) {
        y_denominator = target_height_u32 - 1U;
    }

    // Precompute fixed-point source-coordinate scales
    // These are used by target->source mapping
    if (source_width_u32 > 1U) {
        x_scale = (source_width_u32 - 1U) * IMAGE_BILINEAR_SCALE;
    }

    // Precompute fixed-point Y scale for target->source mapping
    // If source has only one row, set scale to 0 so all Y samples use row 0
    if (source_height_u32 > 1U) {
        y_scale = (source_height_u32 - 1U) * IMAGE_BILINEAR_SCALE;
    }

    // Outer loop scans target image row by row
    for (uint16_t target_y = 0U; target_y < target_height; ++target_y) {
        // Fixed-point source Y mapped from current target row
        const uint32_t source_y_fixed =
            ((uint32_t)target_y * y_scale) / y_denominator;

        // Top source row used for interpolation
        const uint16_t source_y0 =
            (uint16_t)(source_y_fixed >> IMAGE_BILINEAR_FRAC_BITS);

        // Bottom source row used for interpolation
        uint16_t source_y1 = source_y0;

        // Clamp bottom sample row so source_y1 never goes past the last row
        if ((uint32_t)source_y0 + 1U < source_height_u32) {
            source_y1 = (uint16_t)(source_y0 + 1U);
        }

        // Fractional Y part used as vertical interpolation weight
        const uint32_t y_fraction =
            source_y_fixed & IMAGE_BILINEAR_FRACTION_MASK;

        // Complementary vertical weight
        const uint32_t y_inverse = IMAGE_BILINEAR_SCALE - y_fraction;

        // Base linear index of top source row
        const uint32_t row0_base = (uint32_t)source_y0 * source_width_u32;

        // Base linear index of bottom source row
        const uint32_t row1_base = (uint32_t)source_y1 * source_width_u32;

        // Pointer to current output row in the target buffer
        uint8_t *const target_row =
            &target_buffer[(uint32_t)target_y * target_width_u32];

        // Inner loop scans columns for the current target row
        for (uint32_t target_x = 0U; target_x < target_width_u32; ++target_x) {
            // Fixed-point source X mapped from current target column
            const uint32_t source_x_fixed =
                (target_x * x_scale) / x_denominator;

            // Left source column used for interpolation
            const uint16_t source_x0 =
                (uint16_t)(source_x_fixed >> IMAGE_BILINEAR_FRAC_BITS);

            // Right source column used for interpolation
            uint16_t source_x1 = source_x0;

            // Clamp right sample column
            // This keeps source_x1 within the source image bounds
            if ((uint32_t)source_x0 + 1U < source_width_u32) {
                source_x1 = (uint16_t)(source_x0 + 1U);
            }

            // Fractional X part used as horizontal interpolation weight
            const uint32_t x_fraction =
                source_x_fixed & IMAGE_BILINEAR_FRACTION_MASK;

            // Complementary horizontal weight
            const uint32_t x_inverse = IMAGE_BILINEAR_SCALE - x_fraction;

            // Sample the 2x2 neighborhood around the mapped source coordinate
            const uint8_t p00 = source_buffer[row0_base + (uint32_t)source_x0];
            const uint8_t p01 = source_buffer[row0_base + (uint32_t)source_x1];
            const uint8_t p10 = source_buffer[row1_base + (uint32_t)source_x0];
            const uint8_t p11 = source_buffer[row1_base + (uint32_t)source_x1];

            // Interpolate horizontally on top and bottom rows in fixed-point
            const uint32_t top_mix =
                ((uint32_t)p00 * x_inverse) + ((uint32_t)p01 * x_fraction);
                
            const uint32_t bottom_mix =
                ((uint32_t)p10 * x_inverse) + ((uint32_t)p11 * x_fraction);

            // Interpolate vertically between top_mix and bottom_mix
            const uint32_t bilinear_mix =
                (top_mix * y_inverse) + (bottom_mix * y_fraction);

            // Convert fixed-point result back to uint8 with rounding
            target_row[target_x] =
                (uint8_t)((bilinear_mix + IMAGE_BILINEAR_ROUNDING) >>
                          (2U * IMAGE_BILINEAR_FRAC_BITS));
        }
    }

    return target_pixels;
}
