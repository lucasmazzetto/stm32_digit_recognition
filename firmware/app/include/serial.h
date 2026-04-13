#ifndef SERIAL_H
#define SERIAL_H

#include "stm32f4xx_hal.h"

/**
 * @brief Prints a formatted message to the provided UART
 *
 * This function behaves like printf(), but sends the formatted text over UART.
 * If uart or fmt is NULL, no data is transmitted.
 *
 * @param uart Pointer to an initialized UART handle
 * @param fmt printf-style format string. Must not be NULL.
 * @param ... Arguments required by fmt.
 */
void serial_print(UART_HandleTypeDef *uart, const char *fmt, ...);

/**
 * @brief Sends image bytes over UART using DMA and waits for completion
 *
 * @param uart Pointer to an initialized UART handle
 * @param buffer Image buffer to send
 * @param size Number of bytes to send
 * @return HAL status from HAL_UART_Transmit_DMA
 */
HAL_StatusTypeDef serial_send_image(UART_HandleTypeDef *uart,
                                    const uint8_t *buffer, uint32_t size);

#endif /* SERIAL_H */
