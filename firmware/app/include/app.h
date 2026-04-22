#ifndef APP_H
#define APP_H

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "stm32f4xx_hal.h"

#include "camera.h"
#include "convnet.h"
#include "image.h"
#include "lut.h"
#include "serial.h"

#define CROP_FRAME_HEIGHT 120U
#define CROP_FRAME_WIDTH 120U

#define INPUT_FILTER_DARK_MAX_THRESHOLD 60U
#define INPUT_FILTER_MEDIUM_MAX_THRESHOLD 150U

#define INPUT_FILTER_DARK_COUNT_MIN 40U
#define INPUT_FILTER_DARK_COUNT_MAX 150U

#define INPUT_FILTER_MEDIUM_COUNT_MIN 250U
#define INPUT_FILTER_MEDIUM_COUNT_MAX 420U

#define INPUT_FILTER_LIGHT_COUNT_MIN 280U
#define INPUT_FILTER_LIGHT_COUNT_MAX 420U

#define NN_INPUT_SIZE ((uint32_t)INPUT_FLAT_SIZE)
#define OUTPUT_FRAME_HEIGHT 28U
#define OUTPUT_FRAME_WIDTH 28U
#define OUTPUT_FRAME_SIZE (OUTPUT_FRAME_WIDTH * OUTPUT_FRAME_HEIGHT)

/**
 * @brief Runtime configuration passed to the application module
 *
 * @details This struct groups the HAL peripheral handles and GPIO mapping
 *          required by `app_init` to configure camera capture
 */
typedef struct
{
    I2C_HandleTypeDef *i2c_handle;    // I2C handle for camera register access
    DCMI_HandleTypeDef *dcmi_handle;  // DCMI handle used for frame capture
    GPIO_TypeDef *pwdn_gpio_port;     // Camera PWDN GPIO port
    uint16_t pwdn_gpio_pin;           // Camera PWDN GPIO pin
    GPIO_TypeDef *reset_gpio_port;    // Camera RESET GPIO port
    uint16_t reset_gpio_pin;          // Camera RESET GPIO pin
    UART_HandleTypeDef *uart;         // UART handle used for streaming
} app_config_t;

/**
 * @brief Initializes application state and camera subsystem
 *
 * @param config Pointer to runtime configuration with HAL handles and GPIO mapping
 */
void app_init(const app_config_t *config);

/**
 * @brief Executes one application cycle
 *
 * @details Captures one frame, runs preprocessing/inference pipeline, and
 *          streams debug outputs over UART when enabled
 */
void app_run(void);

#ifdef DEBUG
/**
 * @brief Requests cached frame transfer from ISR context
 *
 * @details This function is intended to be called from EXTI callback
 */
void app_request_frame_send_from_isr(void);
#endif

#endif /* APP_H */
