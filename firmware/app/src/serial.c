#include "serial.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// Sends a raw byte buffer over UART after basic argument checks
static void serial_transmit(UART_HandleTypeDef *uart, const char *buffer,
                            size_t length)
{
    // Ignore invalid UART handles, null buffers, or empty payloads
    if ((uart == NULL) || (buffer == NULL) || (length == 0U)) {
        return;
    }

    // HAL_UART_Transmit takes uint16_t length, so clamp oversized payloads
    if (length > (size_t)UINT16_MAX) {
        length = (size_t)UINT16_MAX;
    }

    // Blocking transmit of the prepared payload
    HAL_UART_Transmit(uart, (uint8_t *)buffer, (uint16_t)length,
                      HAL_MAX_DELAY);
}

void serial_print(UART_HandleTypeDef *uart, const char *fmt, ...)
{
    // Temporary buffer where the formatted string is created
    char string[200] = {0};
    int len;
    va_list argp;

    // Nothing to do if UART was not configured or format string is invalid
    if ((uart == NULL) || (fmt == NULL)) {
        return;
    }

    // Build a printf-style message in the local buffer
    va_start(argp, fmt);
    len = vsnprintf(string, sizeof(string), fmt, argp);
    va_end(argp);

    // Transmit only if formatting produced at least one character
    if (len > 0) {
        size_t tx_len = (size_t)len;
        // If truncated, send only the number of bytes that fit in the buffer
        if (tx_len >= sizeof(string)) {
            tx_len = sizeof(string) - 1U;
        }
        // Send the formatted message through the configured UART
        serial_transmit(uart, string, tx_len);
    }
}

HAL_StatusTypeDef serial_send_image(UART_HandleTypeDef *uart,
                                    const uint8_t *buffer, uint16_t size)
{
    HAL_StatusTypeDef status;

    if ((uart == NULL) || (buffer == NULL) || (size == 0U)) {
        return HAL_ERROR;
    }

    status = HAL_UART_Transmit_DMA(uart, (uint8_t *)buffer, size);
    if (status == HAL_OK) {
        while (HAL_UART_GetState(uart) != HAL_UART_STATE_READY) {}
    }

    return status;
}
