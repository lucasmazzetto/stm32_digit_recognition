#ifndef CAMERA_H_
#define CAMERA_H_

#include "main.h"

/**
 * @brief OV2640 image effects supported by this driver.
 */
typedef enum camera_special_effect
{
    CAMERA_SPECIAL_EFFECT_ANTIQUE = 0,
    CAMERA_SPECIAL_EFFECT_BLUISH = 1,
    CAMERA_SPECIAL_EFFECT_GREENISH = 2,
    CAMERA_SPECIAL_EFFECT_REDDISH = 3,
    CAMERA_SPECIAL_EFFECT_BLACK_WHITE = 4,
    CAMERA_SPECIAL_EFFECT_NEGATIVE = 5,
    CAMERA_SPECIAL_EFFECT_NEGATIVE_BLACK_WHITE = 6,
    CAMERA_SPECIAL_EFFECT_NORMAL = 7
} camera_special_effect_t;

/**
 * @brief Resolution options mapped to legacy driver constants.
 */
typedef enum camera_resolution
{
    CAMERA_RES_160X120 = 15533,
    CAMERA_RES_320X240 = 15534,
    CAMERA_RES_640X480 = 15535,
    CAMERA_RES_800X600 = 25535,
    CAMERA_RES_1024X768 = 45535,
    CAMERA_RES_1280X960 = 65535
} camera_resolution_t;

/**
 * @brief OV2640 contrast presets.
 */
typedef enum camera_contrast
{
    CAMERA_CONTRAST_0 = 0,
    CAMERA_CONTRAST_1 = 1,
    CAMERA_CONTRAST_2 = 2,
    CAMERA_CONTRAST_3 = 3,
    CAMERA_CONTRAST_4 = 4
} camera_contrast_t;

/**
 * @brief OV2640 saturation presets.
 */
typedef enum camera_saturation
{
    CAMERA_SATURATION_0 = 0,
    CAMERA_SATURATION_1 = 1,
    CAMERA_SATURATION_2 = 2,
    CAMERA_SATURATION_3 = 3,
    CAMERA_SATURATION_4 = 4
} camera_saturation_t;

/**
 * @brief OV2640 brightness presets.
 */
typedef enum camera_brightness
{
    CAMERA_BRIGHTNESS_0 = 0,
    CAMERA_BRIGHTNESS_1 = 1,
    CAMERA_BRIGHTNESS_2 = 2,
    CAMERA_BRIGHTNESS_3 = 3,
    CAMERA_BRIGHTNESS_4 = 4
} camera_brightness_t;

/**
 * @brief OV2640 white-balance/light-mode presets.
 */
typedef enum camera_light_mode
{
    CAMERA_LIGHT_MODE_AUTO = 0,
    CAMERA_LIGHT_MODE_SUNNY = 1,
    CAMERA_LIGHT_MODE_CLOUDY = 2,
    CAMERA_LIGHT_MODE_OFFICE = 3,
    CAMERA_LIGHT_MODE_HOME = 4
} camera_light_mode_t;

/**
 * @brief OV2640 white-balance algorithm mode.
 */
typedef enum camera_white_balance_mode
{
    WHITE_BALANCE_SIMPLE = 0,
    WHITE_BALANCE_AUTO = 1
} camera_white_balance_mode_t;

/**
 * @brief Runtime configuration for the camera driver.
 */
typedef struct
{
    I2C_HandleTypeDef *i2c_handle;
    DCMI_HandleTypeDef *dcmi_handle;
    UART_HandleTypeDef *uart;
    camera_resolution_t image_resolution;
    GPIO_TypeDef *pwdn_gpio_port;
    uint16_t pwdn_gpio_pin;
    GPIO_TypeDef *reset_gpio_port;
    uint16_t reset_gpio_pin;
} camera_config_t;

/**
 * @brief Returns image width for a given resolution enum.
 *
 * If an invalid value is provided, defaults to 160.
 *
 * @param resolution Value from @ref camera_resolution.
 * @return Image width in pixels.
 */
uint16_t camera_get_width(camera_resolution_t resolution);

/**
 * @brief Returns image height for a given resolution enum.
 *
 * If an invalid value is provided, defaults to 120.
 *
 * @param resolution Value from @ref camera_resolution.
 * @return Image height in pixels.
 */
uint16_t camera_get_height(camera_resolution_t resolution);

/**
 * @brief Returns raw YUV422 frame buffer size in bytes for a resolution.
 *
 * @param resolution Value from @ref camera_resolution.
 * @return Buffer size in bytes for YUV422 (2 bytes per pixel).
 */
uint32_t camera_get_frame_buffer_size(camera_resolution_t resolution);

/**
 * @brief Initializes the OV2640 hardware control and SCCB interfaces.
 *
 * Performs PWDN/RESET sequencing, issues a software reset, and clears DCMI
 * state before the first capture.
 *
 * @param config Pointer to camera runtime configuration.
 */
void camera_init(const camera_config_t *config);

/**
 * @brief Applies one of the predefined resolution pipelines.
 *
 * @param resolution Resolution constant from @ref camera_resolution.
 */
void camera_set_resolution(const camera_resolution_t resolution);

/**
 * @brief Starts one DCMI snapshot DMA transfer into the frame buffer.
 *
 * This routine currently waits a fixed delay before suspend/stop.
 *
 * @param frame_buffer Destination frame buffer base address.
 * @param transfer_length Transfer length forwarded to HAL_DCMI_Start_DMA().
 * @return Captured frame size in bytes, or 0 on error.
 */
uint32_t camera_capture_frame(uint8_t *frame_buffer,
                              uint32_t transfer_length);

/**
 * @brief Applies one special-effect preset.
 *
 * @param effect Value from @ref camera_special_effect.
 */
void camera_set_special_effect(const camera_special_effect_t effect);

/**
 * @brief Applies one contrast preset.
 *
 * @param contrast_level Value from @ref camera_contrast.
 */
void camera_set_contrast(const camera_contrast_t contrast_level);

/**
 * @brief Applies one saturation preset.
 *
 * @param saturation_level Value from @ref camera_saturation.
 */
void camera_set_saturation(const camera_saturation_t saturation_level);

/**
 * @brief Applies one brightness preset.
 *
 * @param brightness_level Value from @ref camera_brightness.
 */
void camera_set_brightness(const camera_brightness_t brightness_level);

/**
 * @brief Applies one light/white-balance preset.
 *
 * @param white_balance_mode Value from @ref camera_light_mode.
 */
void camera_set_light_mode(const camera_light_mode_t white_balance_mode);

/**
 * @brief Selects OV2640 white-balance algorithm mode.
 *
 * @param white_balance_mode Value from @ref camera_white_balance_mode.
 */
void camera_set_white_balance(
    const camera_white_balance_mode_t white_balance_mode);

#endif /* CAMERA_H_ */
