#include "app.h"

#include <stdlib.h>
#include <string.h>

static app_config_t app_config;
static uint8_t *frame_buffer = NULL;
static uint32_t frame_buffer_size = 0U;
static uint8_t button_armed = 0U;

// static const int input_sample[] = {
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -22359,  29555,  16191,  12079, -34696, -47032,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536,  48574,  65022,
//      65022,  65022,  65022,  58340,  36238,  36238,  36238,  36238,
//      36238,  36238,  36238,  36238,  21845, -38808, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -31097,  -6939, -28527,  -6939,  18247,  51144,
//      65022,  50116,  65022,  65022,  65022,  62966,  52172,  65022,
//      65022,   6425, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -56798, -31611, -58340, -31097, -31097,
//     -31097, -35210, -54742,  55770,  65022, -11051, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -22873,  64508,
//      41892, -56284, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -54228,  54228,  65536, -22873, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536,    771,  65022,  56798,
//     -42920, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -35210,  62452,  65022, -33668, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536,   2827,  65022,  30583, -62966,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -60910,
//      39836,  61938, -35724, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536,   -771,  65022,  28013, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -26985,  63480,
//      57826, -36238, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -55770,  48060,  65022,  19789, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -63994,  38808,  65022,  47032,
//     -47546, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -46004,  65022,  65022, -25957, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -49602,  49602,  65022,  -6425, -65022,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536,   2827,
//      65022,  65022, -38808, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -34182,  58854,  65022,  65022, -38808, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536,  -3341,  65022,
//      65022,  47032, -44976, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536,  -3341,  65022,  40864, -56284, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
//     -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536
// };

// static int conv1_out[BATCH_SIZE * CONV1_OUT_CHANNELS * CONV1_OUT_HEIGHT * CONV1_OUT_WIDTH];
// static int pool1_out[BATCH_SIZE * CONV1_OUT_CHANNELS * POOL1_OUT_HEIGHT * POOL1_OUT_WIDTH];
// static int conv2_out[BATCH_SIZE * CONV2_OUT_CHANNELS * CONV2_OUT_HEIGHT * CONV2_OUT_WIDTH];
// static int pool2_out[BATCH_SIZE * CONV2_OUT_CHANNELS * POOL2_OUT_HEIGHT * POOL2_OUT_WIDTH];
// static int linear1_out[BATCH_SIZE * LINEAR1_OUT_FEATURES];
// static int linear2_out[BATCH_SIZE * LINEAR2_OUT_FEATURES];
// static int output[BATCH_SIZE * OUTPUT_DIM];
// static unsigned int class_indices[BATCH_SIZE];
// static camera_resolution_t camera_resolution;

// convnet_forward(input_sample, conv1_out, pool1_out,
//                 conv2_out, pool2_out, linear1_out,
//                 linear2_out, output, class_indices);

void app_init(const app_config_t *config)
{
    camera_config_t camera_config;

    app_config = *config;

    frame_buffer_size = camera_get_frame_buffer_size(app_config.image_resolution);

    if (frame_buffer != NULL) {
        free(frame_buffer);
        frame_buffer = NULL;
    }

    // Allocate once at init so buffer size can follow selected resolution
    frame_buffer = (uint8_t *)malloc(frame_buffer_size);

    if (frame_buffer == NULL) {
#ifdef DEBUG
        serial_print(app_config.uart,
                     "Failed to allocate frame buffer (%u bytes)\r\n",
                     (unsigned int)frame_buffer_size);
#endif
        frame_buffer_size = 0U;
        return;
    }

    camera_config.i2c_handle = app_config.i2c_handle;
    camera_config.dcmi_handle = app_config.dcmi_handle;
    camera_config.uart = app_config.uart;
    camera_config.image_resolution = app_config.image_resolution;
    camera_config.pwdn_gpio_port = app_config.pwdn_gpio_port;
    camera_config.pwdn_gpio_pin = app_config.pwdn_gpio_pin;
    camera_config.reset_gpio_port = app_config.reset_gpio_port;
    camera_config.reset_gpio_pin = app_config.reset_gpio_pin;

    camera_init(&camera_config);

    camera_set_special_effect(CAMERA_SPECIAL_EFFECT_NORMAL);
    camera_set_contrast(CAMERA_CONTRAST_0);
    camera_set_saturation(CAMERA_SATURATION_0);
    camera_set_brightness(CAMERA_BRIGHTNESS_0);
    camera_set_white_balance(WHITE_BALANCE_AUTO);
    camera_set_light_mode(CAMERA_LIGHT_MODE_AUTO);
}

void app_run(void)
{
    if ((frame_buffer == NULL) || (frame_buffer_size == 0U)) {
        return;
    }

    if (HAL_GPIO_ReadPin(app_config.button_gpio_port,
                         app_config.button_gpio_pin) == GPIO_PIN_SET) {

        // Trigger once per physical press and re-arm only after the button is released
        if (button_armed == 1U) {
            uint32_t length;
            uint16_t width = 0U;
            uint16_t height = 0U;

#ifdef DEBUG
            serial_print(app_config.uart, "Button press detected\r\n");
            serial_print(app_config.uart, "Starting snapshot capture\r\n");
#endif
            memset(frame_buffer, 0, frame_buffer_size);

            length = camera_capture_frame(frame_buffer, frame_buffer_size);
            width = camera_get_width(app_config.image_resolution);
            height = camera_get_height(app_config.image_resolution);

#ifdef DEBUG
            serial_print(app_config.uart, "Snapshot finished\r\n");
            serial_print(app_config.uart, "Image size: %u bytes\r\n",
                         (unsigned int)length);
#endif

            if (length > 0U) {

                // TODO: Implement NN inference

#ifdef DEBUG
                HAL_StatusTypeDef status;

                serial_print(app_config.uart,
                             "FRAME_BEGIN %u %u %u\r\n",
                             (unsigned int)width, (unsigned int)height,
                             (unsigned int)length);
                status =
                    serial_send_image(app_config.uart, frame_buffer, length);
                serial_print(app_config.uart, "\r\nFRAME_END status=%d\r\n",
                             (int)status);
#endif
            } else {
#ifdef DEBUG
                serial_print(app_config.uart,
                             "Failed to capture frame\r\n");
#endif
            }

            button_armed = 0U;
        }
    } else {
        button_armed = 1U;
    }
}
