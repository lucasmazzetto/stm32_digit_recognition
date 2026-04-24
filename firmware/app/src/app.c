#include "app.h"
#include "distance.h"

#ifdef DEBUG
static volatile uint8_t send_frame_request = 0U;
#endif

static app_config_t app_config;

// Keep buffers 4-byte aligned for safer DMA/peripheral access and efficient word reads/writes
static uint8_t frame_buffer[CAMERA_FRAME_BUFFER_SIZE]
    __attribute__((aligned(4)));

static uint8_t resized_frame_buffer[OUTPUT_FRAME_SIZE]
    __attribute__((aligned(4)));

static int nn_input[INPUT_FLAT_SIZE];

static int conv_1_output[BATCH_SIZE * CONV_1_OUT_CHANNELS * CONV_1_OUT_HEIGHT *
                         CONV_1_OUT_WIDTH];

static int pool_1_output[BATCH_SIZE * CONV_1_OUT_CHANNELS * POOL_1_OUT_HEIGHT *
                         POOL_1_OUT_WIDTH];

static int conv_2_output[BATCH_SIZE * CONV_2_OUT_CHANNELS * CONV_2_OUT_HEIGHT *
                         CONV_2_OUT_WIDTH];

static int pool_2_output[BATCH_SIZE * CONV_2_OUT_CHANNELS * POOL_2_OUT_HEIGHT *
                         POOL_2_OUT_WIDTH];

static int linear_1_output[BATCH_SIZE * LINEAR_1_OUT_FEATURES];

static int linear_2_output[BATCH_SIZE * LINEAR_2_OUT_FEATURES];

static int logits[BATCH_SIZE * OUTPUT_DIM];

static unsigned int predictions[BATCH_SIZE];

/**
 * Cosine similarity and distance are represented in an implicit fixed-point
 * scale with DISTANCE_FRAC_BITS fractional bits:
 * - similarity ~= 1.0 maps to APP_SIMILARITY_MAX
 * - distance = APP_SIMILARITY_MAX - similarity
 * - DISTANCE_THRESHOLD is stored in the same scale
 */
#define APP_SIMILARITY_SCALE (1 << DISTANCE_FRAC_BITS)
#define APP_SIMILARITY_MAX (APP_SIMILARITY_SCALE - 1)
#define APP_SIMILARITY_MIN (-APP_SIMILARITY_SCALE)

#if DISTANCE_VECTOR_SIZE != OUTPUT_FRAME_SIZE
#error "DISTANCE_VECTOR_SIZE must match OUTPUT_FRAME_SIZE"
#endif

/**
 * @brief Integer square root for uint64_t values.
 *
 * @details This is a pure integer Newton iteration. It is deterministic and
 *          portable to MCU targets, avoiding any floating-point dependency in
 *          the distance computation path.
 *
 * @param value Non-negative input.
 * @return floor(sqrt(value)).
 */
static uint32_t app_isqrt_u64(uint64_t value)
{
    uint64_t x;
    uint64_t y;

    if (value == 0U) {
        return 0U;
    }

    x = value;
    y = (x + 1U) / 2U;

    while (y < x) {
        x = y;
        y = (x + (value / x)) / 2U;
    }

    return (uint32_t)x;
}

/**
 * @brief Computes the L2 norm of the generated distance center vector.
 *
 * @details This norm is shared by all frame distance computations, because the
 *          center vector is constant for a given generated distance file.
 *
 * @return Integer norm of distance_center_vector.
 */
static uint32_t app_compute_center_norm(void)
{
    uint64_t norm_sq = 0U;

    for (uint32_t index = 0U; index < DISTANCE_VECTOR_SIZE; ++index) {
        const int32_t center_value = (int32_t)distance_center_vector[index];
        norm_sq += (uint64_t)(center_value * center_value);
    }

    return app_isqrt_u64(norm_sq);
}

/**
 * @brief Computes integer cosine distance between input image and center.
 *
 * @details The full computation is integer-only:
 *          1) dot product and squared norms
 *          2) integer sqrt for both norms
 *          3) scaled cosine similarity using rounded integer division
 *          4) distance = max_similarity - similarity
 *
 *          The returned value uses the same implicit fixed-point scale as
 *          DISTANCE_THRESHOLD, so it can be compared directly.
 *
 * @param image Input grayscale image with DISTANCE_VECTOR_SIZE pixels.
 * @return Integer cosine distance in implicit fixed-point scale.
 */
static int32_t app_compute_cosine_distance(const uint8_t *image)
{
    uint64_t dot = 0U;
    uint64_t norm_image_sq = 0U;
    const uint32_t center_norm = app_compute_center_norm();
    uint32_t image_norm;
    uint64_t denominator;
    int32_t similarity;

    for (uint32_t index = 0U; index < DISTANCE_VECTOR_SIZE; ++index) {
        const int32_t image_value = (int32_t)image[index];
        const int32_t center_value = (int32_t)distance_center_vector[index];

        // Dot product term for cosine numerator.
        dot += (uint64_t)(image_value * center_value);
        // Squared norm term for input vector magnitude.
        norm_image_sq += (uint64_t)(image_value * image_value);
    }

    image_norm = app_isqrt_u64(norm_image_sq);
    denominator = (uint64_t)image_norm * (uint64_t)center_norm;

    if (denominator == 0U) {
        // Degenerate case: return neutral similarity, resulting in max distance.
        similarity = 0;
    } else {
        similarity = (int32_t)(((dot << DISTANCE_FRAC_BITS) +
                                (denominator / 2U)) /
                               denominator);
    }

    // Clamp to the configured similarity range represented in this fixed scale.
    if (similarity > APP_SIMILARITY_MAX) {
        similarity = APP_SIMILARITY_MAX;
    } else if (similarity < APP_SIMILARITY_MIN) {
        similarity = APP_SIMILARITY_MIN;
    }

    return (APP_SIMILARITY_MAX - similarity);
}

/**
 * @brief Integer cosine-distance known/unknown gate.
 *
 * @details If computed distance is less than or equal to DISTANCE_THRESHOLD,
 *          input is considered known-like. Otherwise it is unknown-like.
 *
 * @param image Input grayscale image with DISTANCE_VECTOR_SIZE pixels.
 * @return 1 if input is known-like, 0 if unknown-like.
 */
static uint8_t app_input_is_known(const uint8_t *image)
{
    const int32_t distance = app_compute_cosine_distance(image);

#ifdef DEBUG
    serial_print(app_config.uart,
                 "FILTER_DISTANCE value=%ld threshold=%d\r\n",
                 (long)distance, (int)DISTANCE_THRESHOLD);
#endif

    return (uint8_t)(distance <= DISTANCE_THRESHOLD);
}

/**
 * @brief Converts uint8 grayscale pixels to Q16 fixed-point input values
 *
 * @param src Source uint8 grayscale buffer
 * @param len Number of pixels to convert
 * @param dst Destination fixed-point buffer
 */
static void preprocess_u8_to_q16(const uint8_t *src, uint32_t len, int *dst)
{
    for (uint32_t index = 0U; index < len; ++index) {
        dst[index] = (int)u8_to_q16_lut[src[index]];
    }
}

void app_init(const app_config_t *config)
{
    camera_config_t camera_config;

    app_config = *config;

    camera_config.i2c_handle = app_config.i2c_handle;
    camera_config.dcmi_handle = app_config.dcmi_handle;
    camera_config.uart = app_config.uart;
    camera_config.pwdn_gpio_port = app_config.pwdn_gpio_port;
    camera_config.pwdn_gpio_pin = app_config.pwdn_gpio_pin;
    camera_config.reset_gpio_port = app_config.reset_gpio_port;
    camera_config.reset_gpio_pin = app_config.reset_gpio_pin;

    camera_init(&camera_config);
    display_init(&app_config.display_config);
}

#ifdef DEBUG
void app_request_frame_send_from_isr(void)
{
    send_frame_request = 1U;
}
#endif

void app_run(void)
{
    uint32_t length;
    uint32_t grayscale_length = 0U;
    uint32_t cropped_length = 0U;
    uint32_t resized_length = 0U;

    memset(frame_buffer, 0, CAMERA_FRAME_BUFFER_SIZE);

    length = camera_capture_frame(frame_buffer, CAMERA_FRAME_BUFFER_SIZE);

    if (length > 0U) {
        // Output remains in frame_buffer, compacted at the beginning
        grayscale_length = image_yuv422_to_grayscale(frame_buffer, length, 0U);

        if (grayscale_length > 0U) {
            // Crop 160x120 grayscale to a square 120x120 image
            cropped_length = image_grayscale_crop_center(
                frame_buffer, CAMERA_FRAME_WIDTH, CAMERA_FRAME_HEIGHT,
                CROP_FRAME_WIDTH, CROP_FRAME_HEIGHT);
        }

        if (cropped_length > 0U) {
            // Resize 120x120 to 28x28 using bilinear interpolation
            resized_length = image_grayscale_resize(
                frame_buffer, CROP_FRAME_WIDTH, CROP_FRAME_HEIGHT,
                resized_frame_buffer, OUTPUT_FRAME_WIDTH, OUTPUT_FRAME_HEIGHT);
        }
    }

#ifdef DEBUG
    serial_print(app_config.uart, "Snapshot finished\r\n");
    serial_print(app_config.uart, "Image size: %u bytes\r\n",
                 (unsigned int)length);
    serial_print(app_config.uart, "Grayscale size: %u bytes\r\n",
                 (unsigned int)grayscale_length);
    serial_print(app_config.uart, "Cropped size: %u bytes\r\n",
                 (unsigned int)cropped_length);
    serial_print(app_config.uart, "Resized size: %u bytes\r\n",
                 (unsigned int)resized_length);
#endif

    if (resized_length > 0U) {
        // Validate input before NN and run only for known-like inputs
        if (resized_length == NN_INPUT_SIZE) {
            if (app_input_is_known(resized_frame_buffer) == 1U) {
                preprocess_u8_to_q16(resized_frame_buffer, NN_INPUT_SIZE,
                                     nn_input);

                convnet_forward(nn_input, conv_1_output, pool_1_output,
                                conv_2_output, pool_2_output, linear_1_output,
                                linear_2_output, logits, predictions);
                
                display_show_digit(predictions[0]);
#ifdef DEBUG
                serial_print(app_config.uart, "NN_PRED %u\r\n",
                             (unsigned int)predictions[0]);
#endif
            } else {
                display_show_digit(-1);
#ifdef DEBUG
                serial_print(app_config.uart, "NN_PRED unknown\r\n");
#endif
            }
        }
    } else {
#ifdef DEBUG
        serial_print(app_config.uart, "Failed to capture frame\r\n");
#endif
    }

#ifdef DEBUG
    if (send_frame_request == 1U) {
        HAL_StatusTypeDef status;

        send_frame_request = 0U;

        serial_print(app_config.uart, "FRAME_BEGIN %u %u %u\r\n",
                     (unsigned int)OUTPUT_FRAME_WIDTH,
                     (unsigned int)OUTPUT_FRAME_HEIGHT,
                     (unsigned int)OUTPUT_FRAME_SIZE);
        // Send cached 28x28 grayscale payload
        status = serial_send_image(app_config.uart, resized_frame_buffer,
                                   OUTPUT_FRAME_SIZE);

        serial_print(app_config.uart, "\r\nFRAME_END status=%d\r\n",
                     (int)status);
    }
#endif
}
