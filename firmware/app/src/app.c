#include "app.h"
#include "image.h"
#include "lut.h"

#define CROP_FRAME_WIDTH 120U
#define CROP_FRAME_HEIGHT 120U
#define OUTPUT_FRAME_WIDTH 28U
#define OUTPUT_FRAME_HEIGHT 28U
#define OUTPUT_FRAME_SIZE (OUTPUT_FRAME_WIDTH * OUTPUT_FRAME_HEIGHT)
#define NN_INPUT_SIZE ((uint32_t)INPUT_FLAT_SIZE)

_Static_assert(OUTPUT_FRAME_SIZE == NN_INPUT_SIZE,
               "Output frame size must match model input size");

static app_config_t app_config;
static uint8_t frame_buffer[CAMERA_FRAME_BUFFER_SIZE]
    __attribute__((aligned(4)));
static uint8_t resized_frame_buffer[OUTPUT_FRAME_SIZE]
    __attribute__((aligned(4)));
static int nn_input[INPUT_FLAT_SIZE] __attribute__((aligned(4)));
static uint8_t button_armed = 0U;


static int conv1_out[BATCH_SIZE * CONV1_OUT_CHANNELS * CONV1_OUT_HEIGHT * CONV1_OUT_WIDTH];
static int pool1_out[BATCH_SIZE * CONV1_OUT_CHANNELS * POOL1_OUT_HEIGHT * POOL1_OUT_WIDTH];
static int conv2_out[BATCH_SIZE * CONV2_OUT_CHANNELS * CONV2_OUT_HEIGHT * CONV2_OUT_WIDTH];
static int pool2_out[BATCH_SIZE * CONV2_OUT_CHANNELS * POOL2_OUT_HEIGHT * POOL2_OUT_WIDTH];
static int linear1_out[BATCH_SIZE * LINEAR1_OUT_FEATURES];
static int linear2_out[BATCH_SIZE * LINEAR2_OUT_FEATURES];
static int output[BATCH_SIZE * OUTPUT_DIM];
static unsigned int class_indices[BATCH_SIZE];

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
}

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
                resized_frame_buffer, OUTPUT_FRAME_WIDTH,
                OUTPUT_FRAME_HEIGHT);
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
        uint8_t nn_prediction_valid = 0U;
        unsigned int predicted_digit = 0U;

        if (resized_length == OUTPUT_FRAME_SIZE) {
            preprocess_u8_to_q16(resized_frame_buffer, NN_INPUT_SIZE, nn_input);
        }

        // Run NN only when resized payload matches expected model input size
        if (resized_length == NN_INPUT_SIZE) {
            convnet_forward(nn_input, conv1_out, pool1_out,
                            conv2_out, pool2_out, linear1_out,
                            linear2_out, output, class_indices);
            predicted_digit = class_indices[0];
            nn_prediction_valid = 1U;
        }

#ifdef DEBUG
        if (nn_prediction_valid == 1U) {
            serial_print(app_config.uart, "NN_PRED %u\r\n",
                         (unsigned int)predicted_digit);
        }
#endif
    } else {
#ifdef DEBUG
        serial_print(app_config.uart, "Failed to capture frame\r\n");
#endif
    }

    if (HAL_GPIO_ReadPin(app_config.button_gpio_port,
                         app_config.button_gpio_pin) == GPIO_PIN_SET) {
        // Trigger once per physical press and re-arm only after release
        if (button_armed == 1U) {
#ifdef DEBUG
            HAL_StatusTypeDef status;

            serial_print(app_config.uart, "FRAME_BEGIN %u %u %u\r\n",
                         (unsigned int)OUTPUT_FRAME_WIDTH,
                         (unsigned int)OUTPUT_FRAME_HEIGHT,
                         (unsigned int)OUTPUT_FRAME_SIZE);
            // Send cached 28x28 grayscale payload
            status = serial_send_image(
                app_config.uart, resized_frame_buffer, OUTPUT_FRAME_SIZE);

            serial_print(app_config.uart, "\r\nFRAME_END status=%d\r\n",
                         (int)status);
#endif
            button_armed = 0U;
        }
    } else {
        // Re-arm only on release transition and announce ready state once
        if (button_armed == 0U) {
            button_armed = 1U;
#ifdef DEBUG
            serial_print(app_config.uart, "READY\r\n");
#endif
        }
    }
}
