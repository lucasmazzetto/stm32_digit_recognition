#ifndef APP_H
#define APP_H

#include "camera.h"
#include "convnet.h"
#include "serial.h"
#include "stm32f4xx_hal.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Runtime configuration passed to the application module
 *
 * @details This struct groups the HAL peripheral handles and GPIO mapping
 *          required by `app_init` to configure camera capture and user input
 */
typedef struct
{
    I2C_HandleTypeDef *i2c_handle; // SCCB/I2C handle for camera register access
    DCMI_HandleTypeDef *dcmi_handle; // DCMI handle used for frame capture
    GPIO_TypeDef *pwdn_gpio_port; // Camera PWDN GPIO port
    uint16_t pwdn_gpio_pin; // Camera PWDN GPIO pin
    GPIO_TypeDef *reset_gpio_port; // Camera RESET GPIO port
    uint16_t reset_gpio_pin; // Camera RESET GPIO pin
    GPIO_TypeDef *button_gpio_port; // User button GPIO port
    uint16_t button_gpio_pin; // User button GPIO pin
    UART_HandleTypeDef *uart; // UART handle used for debug/image streaming
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
 * @details Polls the user button, captures one frame on press, processes the
 *          image pipeline, and streams the result over UART when successful
 */
void app_run(void);

#endif /* APP_H */
