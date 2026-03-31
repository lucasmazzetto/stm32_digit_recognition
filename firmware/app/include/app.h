#ifndef APP_H
#define APP_H

#include "app.h"
#include "log.h"

#include "convnet.h"
#include "stm32f4xx_hal.h"

#include <stddef.h>
#include <stdio.h>

typedef struct __UART_HandleTypeDef UART_HandleTypeDef;

typedef struct {
  UART_HandleTypeDef *uart;
} app_config_t;

void run_app(app_config_t *config);

#endif /* APP_H */
