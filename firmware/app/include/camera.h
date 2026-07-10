#ifndef CAMERA_H_
#define CAMERA_H_

#include <stdint.h>

#include "stm32f4xx_hal.h"

#include "serial.h"

#define CAMERA_CAPTURE_TIMEOUT_MS 2000U
#define CAMERA_DMA_DRAIN_TIMEOUT_MS 100U
#define CAMERA_FRAME_BYTES_PER_PIXEL 2U
#define CAMERA_FRAME_HEIGHT 120U
#define CAMERA_FRAME_WIDTH 160U
#define CAMERA_FRAME_BUFFER_SIZE \
    (CAMERA_FRAME_WIDTH * CAMERA_FRAME_HEIGHT * CAMERA_FRAME_BYTES_PER_PIXEL)
#define SCCB_MAX_RETRIES 3U
#define SCCB_RECOVERY_DELAY_MS 2U
#define SCCB_WRITE_SETTLE_DELAY_MS 2U

typedef struct
{
    I2C_HandleTypeDef *i2c_handle;
    DCMI_HandleTypeDef *dcmi_handle;
    UART_HandleTypeDef *uart;
    GPIO_TypeDef *pwdn_gpio_port;
    uint16_t pwdn_gpio_pin;
    GPIO_TypeDef *reset_gpio_port;
    uint16_t reset_gpio_pin;
} camera_config_t;

/**
 * @brief Initializes the OV2640 hardware control and SCCB interfaces.
 *
 * Performs PWDN/RESET sequencing, issues a software reset, and clears DCMI
 * state before the first capture.
 *
 * @param config Pointer to camera runtime configuration.
 * @return HAL_OK when every configuration write succeeded, HAL_ERROR when any
 *         SCCB write failed after retries.
 */
HAL_StatusTypeDef camera_init(const camera_config_t *config);

/**
 * @brief Starts one DCMI snapshot DMA transfer into the frame buffer.
 *
 * This routine waits until frame-complete (or timeout/error) before stop.
 *
 * @param frame_buffer Destination frame buffer base address.
 * @param transfer_length Requested capture size in bytes.
 * @return Captured frame size in bytes, or 0 on error.
 */
uint32_t camera_capture_frame(uint8_t *frame_buffer, uint32_t transfer_length);

#endif /* CAMERA_H_ */
