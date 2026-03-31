#ifndef UTILS_H
#define UTILS_H

#include "stm32f4xx_hal.h"
#include <string.h>

void serial_print(UART_HandleTypeDef *uart, const char *message);

#endif /* UTILS_H */
