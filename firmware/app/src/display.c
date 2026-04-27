#include "display.h"

#include <stddef.h>

static display_config_t display_config;
static uint8_t display_initialized = 0U;

uint8_t display_decode(uint8_t value)
{
    static const uint8_t masks[10] = {
        (uint8_t)((1U << DISPLAY_A) | (1U << DISPLAY_B) | (1U << DISPLAY_C) |
                  (1U << DISPLAY_D) | (1U << DISPLAY_E) | (1U << DISPLAY_F)),
        (uint8_t)((1U << DISPLAY_B) | (1U << DISPLAY_C)),
        (uint8_t)((1U << DISPLAY_A) | (1U << DISPLAY_B) | (1U << DISPLAY_D) |
                  (1U << DISPLAY_E) | (1U << DISPLAY_G)),
        (uint8_t)((1U << DISPLAY_A) | (1U << DISPLAY_B) | (1U << DISPLAY_C) |
                  (1U << DISPLAY_D) | (1U << DISPLAY_G)),
        (uint8_t)((1U << DISPLAY_B) | (1U << DISPLAY_C) | (1U << DISPLAY_F) |
                  (1U << DISPLAY_G)),
        (uint8_t)((1U << DISPLAY_A) | (1U << DISPLAY_C) | (1U << DISPLAY_D) |
                  (1U << DISPLAY_F) | (1U << DISPLAY_G)),
        (uint8_t)((1U << DISPLAY_A) | (1U << DISPLAY_C) | (1U << DISPLAY_D) |
                  (1U << DISPLAY_E) | (1U << DISPLAY_F) | (1U << DISPLAY_G)),
        (uint8_t)((1U << DISPLAY_A) | (1U << DISPLAY_B) | (1U << DISPLAY_C)),
        (uint8_t)((1U << DISPLAY_A) | (1U << DISPLAY_B) | (1U << DISPLAY_C) |
                  (1U << DISPLAY_D) | (1U << DISPLAY_E) | (1U << DISPLAY_F) |
                  (1U << DISPLAY_G)),
        (uint8_t)((1U << DISPLAY_A) | (1U << DISPLAY_B) | (1U << DISPLAY_C) |
                  (1U << DISPLAY_D) | (1U << DISPLAY_F) | (1U << DISPLAY_G)),
    };

    if (value > 9U) {
        return 0U;
    }

    return masks[value];
}

void display_write_mask(uint8_t segment_mask)
{
    if (display_initialized == 0U) {
        return;
    }

    for (uint32_t index = 0U; index < DISPLAY_SEGMENT_COUNT; ++index) {
        GPIO_PinState pin_state;

        if ((segment_mask & (1U << index)) != 0U) {
            pin_state = GPIO_PIN_SET;
        } else {
            pin_state = GPIO_PIN_RESET;
        }

        if ((display_config.pins[index].gpio_port != NULL) &&
            (display_config.pins[index].gpio_pin != 0U)) {
            HAL_GPIO_WritePin(display_config.pins[index].gpio_port,
                              display_config.pins[index].gpio_pin, pin_state);
        }
    }
}

void display_init(const display_config_t *config)
{
    if (config == NULL) {
        display_initialized = 0U;
        return;
    }

    display_config = *config;
    display_initialized = 1U;

    display_write_mask(0U);
}

void display_show_digit(int32_t value)
{
    if (value < 0) {
        display_write_mask(0U);
        return;
    }

    display_write_mask(display_decode((uint8_t)value));
}
