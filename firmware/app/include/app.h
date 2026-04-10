#ifndef APP_H
#define APP_H

#include "camera.h"
#include "convnet.h"
#include "serial.h"
#include "stm32f4xx_hal.h"

#include <string.h>
#include <stddef.h>
#include <stdio.h>

typedef struct
{
    I2C_HandleTypeDef *i2c_handle;
    DCMI_HandleTypeDef *dcmi_handle;
    GPIO_TypeDef *pwdn_gpio_port;
    uint16_t pwdn_gpio_pin;
    GPIO_TypeDef *reset_gpio_port;
    uint16_t reset_gpio_pin;
    GPIO_TypeDef *button_gpio_port;
    uint16_t button_gpio_pin;
    UART_HandleTypeDef *uart;
} app_config_t;

void app_init(const app_config_t *config);
void app_run(void);

#endif /* APP_H */
