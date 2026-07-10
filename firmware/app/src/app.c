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
 * @brief Integer square root for uint64_t values.
 *
 * @details This is a pure integer Newton iteration. It is deterministic and
 *          portable to MCU targets, avoiding any floating-point dependency in
 *          the distance computation path.
 *
 * @param value Non-negative input.
 * @return floor(sqrt(value)).
 */
static uint32_t app_isqrt(uint64_t value)
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
 * @brief Computes the L2 norm of one center vector.
 *
 * @param center_vector Class center vector in uint8 domain.
 * @param vector_size Number of elements in center vector.
 * @return Integer L2 norm.
 */
static uint32_t app_compute_center_norm(const uint8_t *center_vector,
                                        uint32_t vector_size)
{
    uint64_t norm_sq = 0U;

    for (uint32_t index = 0U; index < vector_size; ++index) {
        const int32_t center_value = (int32_t)center_vector[index];
        norm_sq += (uint64_t)(center_value * center_value);
    }

    return app_isqrt(norm_sq);
}

/**
 * @brief Computes integer cosine distance between input image and class center.
 *
 * @param image Input grayscale image vector (uint8_t) with
 *        DISTANCE_IMAGE_VECTOR_SIZE elements.
 * @param center_vector Class center vector in image domain (uint8_t).
 * @return Integer cosine distance in implicit fixed-point scale.
 */
static int32_t app_compute_image_distance(const uint8_t *image,
                                          const uint8_t *center_vector)
{
    const uint32_t center_norm =
        app_compute_center_norm(center_vector, DISTANCE_IMAGE_VECTOR_SIZE);

    uint64_t dot = 0U;
    uint64_t norm_image_sq = 0U;
    uint32_t image_norm;
    uint64_t denominator;
    int32_t similarity;

    for (uint32_t index = 0U; index < DISTANCE_IMAGE_VECTOR_SIZE; ++index) {
        const int32_t image_value = (int32_t)image[index];
        const int32_t center_value = (int32_t)center_vector[index];

        dot += (uint64_t)(image_value * center_value);
        norm_image_sq += (uint64_t)(image_value * image_value);
    }

    image_norm = app_isqrt(norm_image_sq);
    denominator = (uint64_t)image_norm * (uint64_t)center_norm;

    if (denominator == 0U) {
        similarity = 0;
    } else {
        similarity =
            (int32_t)(((dot << DISTANCE_FRAC_BITS) + (denominator / 2U)) /
                      denominator);
    }

    if (similarity > APP_SIMILARITY_MAX) {
        similarity = APP_SIMILARITY_MAX;
    } else if (similarity < APP_SIMILARITY_MIN) {
        similarity = APP_SIMILARITY_MIN;
    }

    return (APP_SIMILARITY_MAX - similarity);
}

/**
 * @brief Computes integer cosine distance between logits and class center.
 *
 * @param logits_vector Logits output vector in model integer domain.
 * @param center_vector Class center vector in logits domain (int32_t).
 * @return Integer cosine distance in implicit fixed-point scale.
 */
static int32_t app_compute_logits_distance(const int *logits_vector,
                                           const int32_t *center_vector)
{
    int64_t dot = 0;
    uint64_t norm_logits_sq = 0U;
    uint64_t norm_center_sq = 0U;
    uint32_t logits_norm;
    uint32_t center_norm;
    uint64_t denominator;
    int32_t similarity;

    for (uint32_t index = 0U; index < DISTANCE_LOGITS_VECTOR_SIZE; ++index) {
        const int64_t logits_value = (int64_t)logits_vector[index];
        const int64_t center_value = (int64_t)center_vector[index];

        dot += logits_value * center_value;

        norm_logits_sq += (uint64_t)(logits_value * logits_value);
        norm_center_sq += (uint64_t)(center_value * center_value);
    }

    logits_norm = app_isqrt(norm_logits_sq);
    center_norm = app_isqrt(norm_center_sq);
    denominator = (uint64_t)logits_norm * (uint64_t)center_norm;

    if (denominator == 0U) {
        similarity = 0;
    } else if (dot >= 0) {
        similarity = (int32_t)((((uint64_t)dot << DISTANCE_FRAC_BITS) +
                                (denominator / 2U)) /
                               denominator);
    } else {
        similarity = -(int32_t)((((uint64_t)(-dot) << DISTANCE_FRAC_BITS) +
                                 (denominator / 2U)) /
                                denominator);
    }

    if (similarity > APP_SIMILARITY_MAX) {
        similarity = APP_SIMILARITY_MAX;
    } else if (similarity < APP_SIMILARITY_MIN) {
        similarity = APP_SIMILARITY_MIN;
    }

    return (APP_SIMILARITY_MAX - similarity);
}

/**
 * @brief Image+logits class-wise known/unknown gate.
 *
 * @param image Input grayscale image with DISTANCE_IMAGE_VECTOR_SIZE pixels.
 * @param class_id Predicted class index used to select class center/thresholds.
 * @return 1 if input is known-like, 0 if unknown-like.
 */
static uint8_t app_input_is_known(const uint8_t *image, uint32_t class_id)
{
    const int32_t image_distance = app_compute_image_distance(
        image, distance_image_center_vectors[class_id]);

    const int32_t logits_distance = app_compute_logits_distance(
        logits, distance_logits_center_vectors[class_id]);

    const int32_t image_threshold = distance_image_thresholds[class_id];

    const int32_t logits_threshold = distance_logits_thresholds[class_id];

    const uint8_t image_known = (uint8_t)(image_distance <= image_threshold);

    const uint8_t logits_known =
        (uint8_t)(logits_distance <= logits_threshold);

#ifdef DEBUG
    serial_print(app_config.uart,
                 "FILTER_IMAGE class=%u value=%ld threshold=%ld pass=%u\r\n",
                 (unsigned int)class_id, (long)image_distance,
                 (long)image_threshold, (unsigned int)image_known);
    serial_print(app_config.uart,
                 "FILTER_LOGITS class=%u value=%ld threshold=%ld pass=%u\r\n",
                 (unsigned int)class_id, (long)logits_distance,
                 (long)logits_threshold, (unsigned int)logits_known);
#endif

    return (uint8_t)(image_known & logits_known);
}

/**
 * @brief Converts grayscale image pixels to model fixed-point input values.
 *
 * @param src Source grayscale buffer (uint8_t).
 * @param len Number of pixels to convert
 * @param dst Destination model input buffer (fixed-point int).
 */
static void preprocess_image_to_model_input(const uint8_t *src, uint32_t len,
                                            int *dst)
{
    for (uint32_t index = 0U; index < len; ++index) {
        dst[index] = (int)u8_to_q16_lut[src[index]];
    }
}

HAL_StatusTypeDef app_init(const app_config_t *config)
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

    // Initialize the display first so it is blanked even when camera fails
    display_init(&app_config.display_config);

    return camera_init(&camera_config);
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
    uint32_t resized_length = 0U;

    memset(frame_buffer, 0, CAMERA_FRAME_BUFFER_SIZE);

    length = camera_capture_frame(frame_buffer, CAMERA_FRAME_BUFFER_SIZE);

    if (length > 0U) {
        // DCMI hardware crop already delivered the center 120x120 window.
        // Output remains in frame_buffer, compacted at the beginning
        grayscale_length = image_yuv422_to_grayscale(frame_buffer, length, 0U);

        if (grayscale_length > 0U) {
            // Resize 120x120 to 28x28 using bilinear interpolation
            resized_length = image_grayscale_resize(
                frame_buffer, CAMERA_FRAME_WIDTH, CAMERA_FRAME_HEIGHT,
                resized_frame_buffer, OUTPUT_FRAME_WIDTH, OUTPUT_FRAME_HEIGHT);
        }
    }

#ifdef DEBUG
    serial_print(app_config.uart, "Snapshot finished\r\n");
    serial_print(app_config.uart, "Image size: %u bytes\r\n",
                 (unsigned int)length);
    serial_print(app_config.uart, "Grayscale size: %u bytes\r\n",
                 (unsigned int)grayscale_length);
    serial_print(app_config.uart, "Resized size: %u bytes\r\n",
                 (unsigned int)resized_length);
#endif

    if (resized_length > 0U) {
        if (resized_length == NN_INPUT_SIZE) {
            preprocess_image_to_model_input(resized_frame_buffer,
                                            NN_INPUT_SIZE, nn_input);

            convnet_forward(nn_input, conv_1_output, pool_1_output,
                            conv_2_output, pool_2_output, linear_1_output,
                            linear_2_output, logits, predictions);

            const uint8_t is_known =
                app_input_is_known(resized_frame_buffer, predictions[0]);

            if (is_known == 1U) {
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
