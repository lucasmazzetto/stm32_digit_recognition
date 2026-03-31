#include "../include/utils.h"

#include "stm32f4xx_hal.h"

#include <string.h>

void uart_print(UART_HandleTypeDef *uart, const char *message)
{
  size_t len;

  if ((uart == NULL) || (message == NULL)) {
    return;
  }

  len = strlen(message);

  if (len > 0U) {
    HAL_UART_Transmit(uart, (uint8_t *)message, (uint16_t)len, HAL_MAX_DELAY);
  }
}
