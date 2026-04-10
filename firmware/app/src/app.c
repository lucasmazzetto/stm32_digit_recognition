#include "app.h"
#include "image.h"

static app_config_t app_config;
static uint8_t frame_buffer[CAMERA_FRAME_BUFFER_SIZE] __attribute__((aligned(4)));
static uint8_t button_armed = 0U;

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
    if (HAL_GPIO_ReadPin(app_config.button_gpio_port,
                         app_config.button_gpio_pin) == GPIO_PIN_SET) {

        // Trigger once per physical press and re-arm only after the button is released
        if (button_armed == 1U) {
            uint32_t length;
            uint32_t grayscale_length = 0U;

#ifdef DEBUG
            serial_print(app_config.uart, "Button press detected\r\n");
            serial_print(app_config.uart, "Starting snapshot capture\r\n");
#endif
            memset(frame_buffer, 0, CAMERA_FRAME_BUFFER_SIZE);

            length = camera_capture_frame(frame_buffer, CAMERA_FRAME_BUFFER_SIZE);

            grayscale_length = image_yuv422_to_grayscale(
                frame_buffer, length, 0U);

#ifdef DEBUG
            serial_print(app_config.uart, "Snapshot finished\r\n");
            serial_print(app_config.uart, "Image size: %u bytes\r\n",
                         (unsigned int)length);
            serial_print(app_config.uart, "Grayscale size: %u bytes\r\n",
                         (unsigned int)grayscale_length);
#endif

            if (grayscale_length > 0U) {
#ifdef DEBUG
                HAL_StatusTypeDef status;

                serial_print(app_config.uart,
                             "FRAME_BEGIN %u %u %u\r\n",
                             (unsigned int)CAMERA_FRAME_WIDTH,
                             (unsigned int)CAMERA_FRAME_HEIGHT,
                             (unsigned int)grayscale_length);
                // Send grayscale payload compacted in-place from the captured YUV422 frame.
                status = serial_send_image(app_config.uart,
                                           frame_buffer,
                                           grayscale_length);

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
