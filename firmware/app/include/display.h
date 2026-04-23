#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

#include "stm32f4xx_hal.h"

typedef enum
{
    DISPLAY_A,
    DISPLAY_B,
    DISPLAY_C,
    DISPLAY_D,
    DISPLAY_E,
    DISPLAY_F,
    DISPLAY_G
} display_segment_t;

#define DISPLAY_SEGMENT_COUNT 7U

typedef struct
{
    GPIO_TypeDef *gpio_port;
    uint16_t gpio_pin;
} display_pin_t;

typedef struct
{
    display_pin_t pins[DISPLAY_SEGMENT_COUNT];
} display_pins_t;

void display_init(const display_pins_t *config);
uint8_t display_decode(uint8_t value);
void display_write_mask(uint8_t segment_mask);
void display_show_digit(int32_t value);

#endif /* DISPLAY_H */
