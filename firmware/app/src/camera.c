#include "camera.h"
#include "serial.h"

#define SCCB_MAX_RETRIES 3U
#define SCCB_WRITE_SETTLE_DELAY_MS 2U
#define SCCB_RECOVERY_DELAY_MS 2U

static camera_config_t camera_config;

static const unsigned char camera_reg_jpeg_init[][2] = {
    {0xff, 0x00}, {0x2c, 0xff}, {0x2e, 0xdf}, {0xff, 0x01}, {0x3c, 0x32},
    {0x11, 0x00}, {0x09, 0x02}, {0x04, 0x28}, {0x13, 0xe5}, {0x14, 0x48},
    {0x2c, 0x0c}, {0x33, 0x78}, {0x3a, 0x33}, {0x3b, 0xfB}, {0x3e, 0x00},
    {0x43, 0x11}, {0x16, 0x10}, {0x39, 0x92}, {0x35, 0xda}, {0x22, 0x1a},
    {0x37, 0xc3}, {0x23, 0x00}, {0x34, 0xc0}, {0x36, 0x1a}, {0x06, 0x88},
    {0x07, 0xc0}, {0x0d, 0x87}, {0x0e, 0x41}, {0x4c, 0x00}, {0x48, 0x00},
    {0x5B, 0x00}, {0x42, 0x03}, {0x4a, 0x81}, {0x21, 0x99}, {0x24, 0x40},
    {0x25, 0x38}, {0x26, 0x82}, {0x5c, 0x00}, {0x63, 0x00}, {0x61, 0x70},
    {0x62, 0x80}, {0x7c, 0x05}, {0x20, 0x80}, {0x28, 0x30}, {0x6c, 0x00},
    {0x6d, 0x80}, {0x6e, 0x00}, {0x70, 0x02}, {0x71, 0x94}, {0x73, 0xc1},
    {0x12, 0x40}, {0x17, 0x11}, {0x18, 0x43}, {0x19, 0x00}, {0x1a, 0x4b},
    {0x32, 0x09}, {0x37, 0xc0}, {0x4f, 0x60}, {0x50, 0xa8}, {0x6d, 0x00},
    {0x3d, 0x38}, {0x46, 0x3f}, {0x4f, 0x60}, {0x0c, 0x3c}, {0xff, 0x00},
    {0xe5, 0x7f}, {0xf9, 0xc0}, {0x41, 0x24}, {0xe0, 0x14}, {0x76, 0xff},
    {0x33, 0xa0}, {0x42, 0x20}, {0x43, 0x18}, {0x4c, 0x00}, {0x87, 0xd5},
    {0x88, 0x3f}, {0xd7, 0x03}, {0xd9, 0x10}, {0xd3, 0x82}, {0xc8, 0x08},
    {0xc9, 0x80}, {0x7c, 0x00}, {0x7d, 0x00}, {0x7c, 0x03}, {0x7d, 0x48},
    {0x7d, 0x48}, {0x7c, 0x08}, {0x7d, 0x20}, {0x7d, 0x10}, {0x7d, 0x0e},
    {0x90, 0x00}, {0x91, 0x0e}, {0x91, 0x1a}, {0x91, 0x31}, {0x91, 0x5a},
    {0x91, 0x69}, {0x91, 0x75}, {0x91, 0x7e}, {0x91, 0x88}, {0x91, 0x8f},
    {0x91, 0x96}, {0x91, 0xa3}, {0x91, 0xaf}, {0x91, 0xc4}, {0x91, 0xd7},
    {0x91, 0xe8}, {0x91, 0x20}, {0x92, 0x00}, {0x93, 0x06}, {0x93, 0xe3},
    {0x93, 0x05}, {0x93, 0x05}, {0x93, 0x00}, {0x93, 0x04}, {0x93, 0x00},
    {0x93, 0x00}, {0x93, 0x00}, {0x93, 0x00}, {0x93, 0x00}, {0x93, 0x00},
    {0x93, 0x00}, {0x96, 0x00}, {0x97, 0x08}, {0x97, 0x19}, {0x97, 0x02},
    {0x97, 0x0c}, {0x97, 0x24}, {0x97, 0x30}, {0x97, 0x28}, {0x97, 0x26},
    {0x97, 0x02}, {0x97, 0x98}, {0x97, 0x80}, {0x97, 0x00}, {0x97, 0x00},
    {0xc3, 0xed}, {0xa4, 0x00}, {0xa8, 0x00}, {0xc5, 0x11}, {0xc6, 0x51},
    {0xbf, 0x80}, {0xc7, 0x10}, {0xb6, 0x66}, {0xb8, 0xA5}, {0xb7, 0x64},
    {0xb9, 0x7C}, {0xb3, 0xaf}, {0xb4, 0x97}, {0xb5, 0xFF}, {0xb0, 0xC5},
    {0xb1, 0x94}, {0xb2, 0x0f}, {0xc4, 0x5c}, {0xc0, 0x64}, {0xc1, 0x4B},
    {0x8c, 0x00}, {0x86, 0x3D}, {0x50, 0x00}, {0x51, 0xC8}, {0x52, 0x96},
    {0x53, 0x00}, {0x54, 0x00}, {0x55, 0x00}, {0x5a, 0xC8}, {0x5b, 0x96},
    {0x5c, 0x00}, {0xd3, 0x00}, {0xc3, 0xed}, {0x7f, 0x00}, {0xda, 0x00},
    {0xe5, 0x1f}, {0xe1, 0x67}, {0xe0, 0x00}, {0xdd, 0x7f}, {0x05, 0x00},
    {0x12, 0x40}, {0xd3, 0x04}, {0xc0, 0x16}, {0xC1, 0x12}, {0x8c, 0x00},
    {0x86, 0x3d}, {0x50, 0x00}, {0x51, 0x2C}, {0x52, 0x24}, {0x53, 0x00},
    {0x54, 0x00}, {0x55, 0x00}, {0x5A, 0x2c}, {0x5b, 0x24}, {0x5c, 0x00},
    {0xff, 0xff},
};

static const unsigned char camera_reg_yuv422[][2] = {
    {0xFF, 0x00}, {0x05, 0x00}, {0xDA, 0x10}, {0xD7, 0x03}, {0xDF, 0x00},
    {0x33, 0x80}, {0x3C, 0x40}, {0xe1, 0x77}, {0x00, 0x00}, {0xff, 0xff},
};

static const unsigned char camera_reg_jpeg[][2] = {
    {0xe0, 0x14}, {0xe1, 0x77}, {0xe5, 0x1f}, {0xd7, 0x03}, {0xda, 0x10},
    {0xe0, 0x00}, {0xFF, 0x01}, {0x04, 0x08}, {0xff, 0xff},
};

static const unsigned char camera_reg_resolution_160x120_jpeg[][2] = {
    {0xFF, 0x01}, {0x12, 0x40}, {0x17, 0x11}, {0x18, 0x43}, {0x19, 0x00},
    {0x1a, 0x4b}, {0x32, 0x09}, {0x4f, 0xca}, {0x50, 0xa8}, {0x5a, 0x23},
    {0x6d, 0x00}, {0x39, 0x12}, {0x35, 0xda}, {0x22, 0x1a}, {0x37, 0xc3},
    {0x23, 0x00}, {0x34, 0xc0}, {0x36, 0x1a}, {0x06, 0x88}, {0x07, 0xc0},
    {0x0d, 0x87}, {0x0e, 0x41}, {0x4c, 0x00}, {0xFF, 0x00}, {0xe0, 0x04},
    {0xc0, 0x64}, {0xc1, 0x4b}, {0x86, 0x35}, {0x50, 0x92}, {0x51, 0xc8},
    {0x52, 0x96}, {0x53, 0x00}, {0x54, 0x00}, {0x55, 0x00}, {0x57, 0x00},
    {0x5a, 0x2c}, {0x5b, 0x24}, {0x5c, 0x00}, {0xe0, 0x00}, {0xff, 0xff}};

static const unsigned char camera_reg_resolution_320x240_jpeg[][2] = {
    {0xff, 0x01}, {0x12, 0x40}, {0x17, 0x11}, {0x18, 0x43}, {0x19, 0x00},
    {0x1a, 0x4b}, {0x32, 0x09}, {0x4f, 0xca}, {0x50, 0xa8}, {0x5a, 0x23},
    {0x6d, 0x00}, {0x39, 0x12}, {0x35, 0xda}, {0x22, 0x1a}, {0x37, 0xc3},
    {0x23, 0x00}, {0x34, 0xc0}, {0x36, 0x1a}, {0x06, 0x88}, {0x07, 0xc0},
    {0x0d, 0x87}, {0x0e, 0x41}, {0x4c, 0x00}, {0xff, 0x00}, {0xe0, 0x04},
    {0xc0, 0x64}, {0xc1, 0x4b}, {0x86, 0x35}, {0x50, 0x89}, {0x51, 0xc8},
    {0x52, 0x96}, {0x53, 0x00}, {0x54, 0x00}, {0x55, 0x00}, {0x57, 0x00},
    {0x5a, 0x50}, {0x5b, 0x3c}, {0x5c, 0x00}, {0xe0, 0x00}, {0xff, 0xff},
};

static const unsigned char camera_reg_resolution_640x480_jpeg[][2] = {
    {0xff, 0x01}, {0x11, 0x01}, {0x12, 0x00}, {0x17, 0x11}, {0x18, 0x75},
    {0x32, 0x36}, {0x19, 0x01}, {0x1a, 0x97}, {0x03, 0x0f}, {0x37, 0x40},
    {0x4f, 0xbb}, {0x50, 0x9c}, {0x5a, 0x57}, {0x6d, 0x80}, {0x3d, 0x34},
    {0x39, 0x02}, {0x35, 0x88}, {0x22, 0x0a}, {0x37, 0x40}, {0x34, 0xa0},
    {0x06, 0x02}, {0x0d, 0xb7}, {0x0e, 0x01}, {0xff, 0x00}, {0xe0, 0x04},
    {0xc0, 0xc8}, {0xc1, 0x96}, {0x86, 0x3d}, {0x50, 0x89}, {0x51, 0x90},
    {0x52, 0x2c}, {0x53, 0x00}, {0x54, 0x00}, {0x55, 0x88}, {0x57, 0x00},
    {0x5a, 0xa0}, {0x5b, 0x78}, {0x5c, 0x00}, {0xd3, 0x04}, {0xe0, 0x00},
    {0xff, 0xff},
};

static const unsigned char camera_reg_resolution_800x600_jpeg[][2] = {
    {0xFF, 0x01}, {0x11, 0x01}, {0x12, 0x00}, {0x17, 0x11}, {0x18, 0x75},
    {0x32, 0x36}, {0x19, 0x01}, {0x1a, 0x97}, {0x03, 0x0f}, {0x37, 0x40},
    {0x4f, 0xbb}, {0x50, 0x9c}, {0x5a, 0x57}, {0x6d, 0x80}, {0x3d, 0x34},
    {0x39, 0x02}, {0x35, 0x88}, {0x22, 0x0a}, {0x37, 0x40}, {0x34, 0xa0},
    {0x06, 0x02}, {0x0d, 0xb7}, {0x0e, 0x01}, {0xFF, 0x00}, {0xe0, 0x04},
    {0xc0, 0xc8}, {0xc1, 0x96}, {0x86, 0x35}, {0x50, 0x89}, {0x51, 0x90},
    {0x52, 0x2c}, {0x53, 0x00}, {0x54, 0x00}, {0x55, 0x88}, {0x57, 0x00},
    {0x5a, 0xc8}, {0x5b, 0x96}, {0x5c, 0x00}, {0xd3, 0x02}, {0xe0, 0x00},
    {0xff, 0xff}};

static const unsigned char camera_reg_resolution_1024x768_jpeg[][2] = {
    {0xFF, 0x01}, {0x11, 0x01}, {0x12, 0x00}, {0x17, 0x11}, {0x18, 0x75},
    {0x32, 0x36}, {0x19, 0x01}, {0x1a, 0x97}, {0x03, 0x0f}, {0x37, 0x40},
    {0x4f, 0xbb}, {0x50, 0x9c}, {0x5a, 0x57}, {0x6d, 0x80}, {0x3d, 0x34},
    {0x39, 0x02}, {0x35, 0x88}, {0x22, 0x0a}, {0x37, 0x40}, {0x34, 0xa0},
    {0x06, 0x02}, {0x0d, 0xb7}, {0x0e, 0x01}, {0xFF, 0x00}, {0xc0, 0xC8},
    {0xc1, 0x96}, {0x8c, 0x00}, {0x86, 0x3D}, {0x50, 0x00}, {0x51, 0x90},
    {0x52, 0x2C}, {0x53, 0x00}, {0x54, 0x00}, {0x55, 0x88}, {0x5a, 0x00},
    {0x5b, 0xC0}, {0x5c, 0x01}, {0xd3, 0x02}, {0xff, 0xff}};

static const unsigned char camera_reg_resolution_1280x960_jpeg[][2] = {
    {0xFF, 0x01}, {0x11, 0x01}, {0x12, 0x00}, {0x17, 0x11}, {0x18, 0x75},
    {0x32, 0x36}, {0x19, 0x01}, {0x1a, 0x97}, {0x03, 0x0f}, {0x37, 0x40},
    {0x4f, 0xbb}, {0x50, 0x9c}, {0x5a, 0x57}, {0x6d, 0x80}, {0x3d, 0x34},
    {0x39, 0x02}, {0x35, 0x88}, {0x22, 0x0a}, {0x37, 0x40}, {0x34, 0xa0},
    {0x06, 0x02}, {0x0d, 0xb7}, {0x0e, 0x01}, {0xFF, 0x00}, {0xe0, 0x04},
    {0xc0, 0xc8}, {0xc1, 0x96}, {0x86, 0x3d}, {0x50, 0x00}, {0x51, 0x90},
    {0x52, 0x2c}, {0x53, 0x00}, {0x54, 0x00}, {0x55, 0x88}, {0x57, 0x00},
    {0x5a, 0x40}, {0x5b, 0xf0}, {0x5c, 0x01}, {0xd3, 0x02}, {0xe0, 0x00},
    {0xff, 0xff}};

static const unsigned char camera_reg_contrast_plus2[][2] = {
    {0xff, 0x00}, {0x7c, 0x00}, {0x7d, 0x04}, {0x7c, 0x07}, {0x7d, 0x20},
    {0x7d, 0x28}, {0x7d, 0x0c}, {0x7d, 0x06}, {0xff, 0xff}};

static const unsigned char camera_reg_contrast_plus1[][2] = {
    {0xff, 0x00}, {0x7c, 0x00}, {0x7d, 0x04}, {0x7c, 0x07}, {0x7d, 0x20},
    {0x7d, 0x24}, {0x7d, 0x16}, {0x7d, 0x06}, {0xff, 0xff}};

static const unsigned char camera_reg_contrast_zero[][2] = {
    {0xff, 0x00}, {0x7c, 0x00}, {0x7d, 0x04}, {0x7c, 0x07}, {0x7d, 0x20},
    {0x7d, 0x20}, {0x7d, 0x20}, {0x7d, 0x06}, {0xff, 0xff}};

static const unsigned char camera_reg_contrast_minus1[][2] = {
    {0xff, 0x00}, {0x7c, 0x00}, {0x7d, 0x04}, {0x7c, 0x07}, {0x7d, 0x20},
    {0x7d, 0x1c}, {0x7d, 0x2a}, {0x7d, 0x06}, {0xff, 0xff}};

static const unsigned char camera_reg_contrast_minus2[][2] = {
    {0xff, 0x00}, {0x7c, 0x00}, {0x7d, 0x04}, {0x7c, 0x07}, {0x7d, 0x20},
    {0x7d, 0x18}, {0x7d, 0x34}, {0x7d, 0x06}, {0xff, 0xff}};

static const unsigned char camera_reg_saturation_plus2[][2] = {
    {0xff, 0x00}, {0x7c, 0x00}, {0x7d, 0x02}, {0x7c, 0x03},
    {0x7d, 0x68}, {0x7d, 0x68}, {0xff, 0xff}};

static const unsigned char camera_reg_saturation_plus1[][2] = {
    {0xff, 0x00}, {0x7c, 0x00}, {0x7d, 0x02}, {0x7c, 0x03},
    {0x7d, 0x58}, {0x7d, 0x68}, {0xff, 0xff}};

static const unsigned char camera_reg_saturation_zero[][2] = {
    {0xff, 0x00}, {0x7c, 0x00}, {0x7d, 0x02}, {0x7c, 0x03},
    {0x7d, 0x48}, {0x7d, 0x48}, {0xff, 0xff}};

static const unsigned char camera_reg_saturation_minus1[][2] = {
    {0xff, 0x00}, {0x7c, 0x00}, {0x7d, 0x02}, {0x7c, 0x03},
    {0x7d, 0x38}, {0x7d, 0x38}, {0xff, 0xff}};

static const unsigned char camera_reg_saturation_minus2[][2] = {
    {0xff, 0x00}, {0x7c, 0x00}, {0x7d, 0x02}, {0x7c, 0x03},
    {0x7d, 0x28}, {0x7d, 0x28}, {0xff, 0xff}};

static const unsigned char camera_reg_brightness_plus2[][2] = {
    {0xff, 0x00}, {0x7c, 0x00}, {0x7d, 0x04}, {0x7c, 0x09},
    {0x7d, 0x40}, {0x7d, 0x00}, {0xff, 0xff}};

static const unsigned char camera_reg_brightness_plus1[][2] = {
    {0xff, 0x00}, {0x7c, 0x00}, {0x7d, 0x04}, {0x7c, 0x09},
    {0x7d, 0x30}, {0x7d, 0x00}, {0xff, 0xff}};

static const unsigned char camera_reg_brightness_zero[][2] = {
    {0xff, 0x00}, {0x7c, 0x00}, {0x7d, 0x04}, {0x7c, 0x09},
    {0x7d, 0x20}, {0x7d, 0x00}, {0xff, 0xff}};

static const unsigned char camera_reg_brightness_minus1[][2] = {
    {0xff, 0x00}, {0x7c, 0x00}, {0x7d, 0x04}, {0x7c, 0x09},
    {0x7d, 0x10}, {0x7d, 0x00}, {0xff, 0xff}};

static const unsigned char camera_reg_brightness_minus2[][2] = {
    {0xff, 0x00}, {0x7c, 0x00}, {0x7d, 0x04}, {0x7c, 0x09},
    {0x7d, 0x00}, {0x7d, 0x00}, {0xff, 0xff}};

static const unsigned char camera_reg_effect_normal[][2] = {
    {0xff, 0x00}, {0x7c, 0x00}, {0x7d, 0x00}, {0x7c, 0x05},
    {0x7d, 0x80}, {0x7d, 0x80}, {0xff, 0xff}};

static const unsigned char camera_reg_effect_antique[][2] = {
    {0xff, 0x00}, {0x7c, 0x00}, {0x7d, 0x18}, {0x7c, 0x05},
    {0x7d, 0x40}, {0x7d, 0xa6}, {0xff, 0xff}};

static const unsigned char camera_reg_effect_black_negative[][2] = {
    {0xff, 0x00}, {0x7c, 0x00}, {0x7d, 0x58}, {0x7c, 0x05},
    {0x7d, 0x80}, {0x7d, 0x80}, {0xff, 0xff}};

static const unsigned char camera_reg_effect_bluish[][2] = {
    {0xff, 0x00}, {0x7c, 0x00}, {0x7d, 0x18}, {0x7c, 0x05},
    {0x7d, 0xa0}, {0x7d, 0x40}, {0xff, 0xff}};

static const unsigned char camera_reg_effect_black_white[][2] = {
    {0xff, 0x00}, {0x7c, 0x00}, {0x7d, 0x18}, {0x7c, 0x05},
    {0x7d, 0x80}, {0x7d, 0x80}, {0xff, 0xff}};

static const unsigned char camera_reg_effect_negative[][2] = {
    {0xff, 0x00}, {0x7c, 0x00}, {0x7d, 0x40}, {0x7c, 0x05},
    {0x7d, 0x80}, {0x7d, 0x80}, {0xff, 0xff}};

static const unsigned char camera_reg_effect_greenish[][2] = {
    {0xff, 0x00}, {0x7c, 0x00}, {0x7d, 0x18}, {0x7c, 0x05},
    {0x7d, 0x40}, {0x7d, 0x40}, {0xff, 0xff}};

static const unsigned char camera_reg_effect_reddish[][2] = {
    {0xff, 0x00}, {0x7c, 0x00}, {0x7d, 0x18}, {0x7c, 0x05},
    {0x7d, 0x40}, {0x7d, 0xc0}, {0xff, 0xff}};

static const unsigned char camera_reg_white_balance_auto[][2] = {
    {0xff, 0x00}, {0xc7, 0x00}, {0xff, 0xff}};

static const unsigned char camera_reg_white_balance_sunny[][2] = {
    {0xff, 0x00}, {0xc7, 0x40}, {0xcc, 0x5e},
    {0xcd, 0x41}, {0xce, 0x54}, {0xff, 0xff}};

static const unsigned char camera_reg_white_balance_cloudy[][2] = {
    {0xff, 0x00}, {0xc7, 0x40}, {0xcc, 0x65},
    {0xcd, 0x41}, {0xce, 0x4f}, {0xff, 0xff}};

static const unsigned char camera_reg_white_balance_office[][2] = {
    {0xff, 0x00}, {0xc7, 0x40}, {0xcc, 0x52},
    {0xcd, 0x41}, {0xce, 0x66}, {0xff, 0xff}};

static const unsigned char camera_reg_white_balance_home[][2] = {
    {0xff, 0x00}, {0xc7, 0x40}, {0xcc, 0x42},
    {0xcd, 0x3f}, {0xce, 0x71}, {0xff, 0xff}};

/**
 * @brief Stops DCMI capture and waits briefly for peripheral stabilization.
 */
static void stop_capture(void)
{
    // Give DCMI a short settle period before next start/stop transition
    HAL_DCMI_Stop(camera_config.dcmi_handle);
    HAL_Delay(10);

#ifdef DEBUG
    serial_print(camera_config.uart, "DCMI has been stopped \r\n");
#endif
}

/**
 * @brief Writes one register/value pair through SCCB.
 *
 * @param reg_addr OV2640 register address.
 * @param data Register value.
 * @return 1 on success, otherwise 0.
 */
static short sccb_write_register(const uint8_t reg_addr, const uint8_t data)
{
    short operation_status = 0;
    uint8_t buffer[2] = {0};

    // SCCB write payload: first byte is register address, second is register value
    buffer[0] = reg_addr;
    buffer[1] = data;

    const HAL_StatusTypeDef hal_status = HAL_I2C_Master_Transmit(
        camera_config.i2c_handle, (uint16_t)0x60, buffer, 2, 100);

    if (hal_status == HAL_OK) {
        operation_status = 1;
    } else {
        operation_status = 0;
    }

#ifdef DEBUG
    serial_print(camera_config.uart,
                 "sccb_write_register reg=0x%x data=0x%x hal=%d\r\n", reg_addr,
                 data, hal_status);
#endif

    return operation_status;
}

/**
 * @brief Reads one OV2640 register over SCCB.
 *
 * @param reg_addr OV2640 register address.
 * @param out_value Output pointer receiving one byte from the register.
 * @return 1 on success, otherwise 0.
 */
static short sccb_read_register(uint8_t reg_addr, uint8_t *const out_value)
{
    short operation_status = 0;
    HAL_StatusTypeDef hal_status;

    // SCCB read is a write(register address) followed by a single-byte read
    hal_status = HAL_I2C_Master_Transmit(camera_config.i2c_handle,
                                         (uint16_t)0x60, &reg_addr, 1, 100);
    if (hal_status == HAL_OK) {
        hal_status = HAL_I2C_Master_Receive(camera_config.i2c_handle,
                                            (uint16_t)0x60, out_value, 1, 100);
        if (hal_status == HAL_OK) {
            operation_status = 1;
        } else {
            operation_status = 0;
        }
    } else {
        operation_status = 0;
    }
#ifdef DEBUG
    serial_print(camera_config.uart,
                 "sccb_read_register reg=0x%x hal=%d data=0x%x\r\n", reg_addr,
                 hal_status, *out_value);
#endif
    return operation_status;
}

/**
 * @brief Reinitializes the SCCB bus after communication failures.
 */
static void sccb_recover_bus(void)
{
#ifdef DEBUG
    serial_print(camera_config.uart, "SCCB: recovering I2C2 peripheral\r\n");
#endif
    HAL_I2C_DeInit(camera_config.i2c_handle);
    HAL_Delay(SCCB_RECOVERY_DELAY_MS);
    HAL_I2C_Init(camera_config.i2c_handle);
    HAL_Delay(SCCB_RECOVERY_DELAY_MS);
}

/**
 * @brief Writes one OV2640 register with bounded retries and bus recovery.
 *
 * @param reg_addr OV2640 register address.
 * @param data Register value to write.
 * @return 1 on success, 0 if all retries fail.
 */
static short sccb_write_register_retry(const uint8_t reg_addr,
                                       const uint8_t data)
{
    uint8_t attempt;

    for (attempt = 0U; attempt < SCCB_MAX_RETRIES; ++attempt) {
        if (sccb_write_register(reg_addr, data) == 1) {
            return 1;
        }

#ifdef DEBUG
        serial_print(camera_config.uart,
                     "SCCB_write retry reg=0x%x attempt=%u\r\n", reg_addr,
                     (unsigned int)(attempt + 1U));
#endif
        sccb_recover_bus();
    }

    return 0;
}

/**
 * @brief Reads one OV2640 register with bounded retries and bus recovery.
 *
 * @param reg_addr OV2640 register address.
 * @param out_value Output byte pointer for register value.
 * @return 1 on success, 0 if all retries fail.
 */
static short sccb_read_register_retry(const uint8_t reg_addr,
                                      uint8_t *const out_value)
{
    uint8_t attempt;

    for (attempt = 0U; attempt < SCCB_MAX_RETRIES; ++attempt) {
        if (sccb_read_register(reg_addr, out_value) == 1) {
            return 1;
        }

#ifdef DEBUG
        serial_print(camera_config.uart,
                     "SCCB_read retry reg=0x%x attempt=%u\r\n", reg_addr,
                     (unsigned int)(attempt + 1U));
#endif
        sccb_recover_bus();
    }

    return 0;
}

/**
 * @brief Indicates if a written register should be read back for verification.
 *
 * @param reg_addr OV2640 register address.
 * @return 1 when read-back verification is enabled, otherwise 0.
 */
static uint8_t sccb_should_verify_register(const uint8_t reg_addr)
{
    // Verify only control-critical registers to keep configuration time bounded
    return (uint8_t)((reg_addr == 0xffU) || (reg_addr == 0x12U) ||
                     (reg_addr == 0x15U) || (reg_addr == 0x11U) ||
                     (reg_addr == 0x24U) || (reg_addr == 0x25U) ||
                     (reg_addr == 0x26U) || (reg_addr == 0xd3U) ||
                     (reg_addr == 0xe0U));
}

/**
 * @brief Writes one register table to OV2640 over SCCB.
 *
 * The array must be terminated with `{0xff, 0xff}`.
 *
 * @param register_table Register/value pairs to be written sequentially.
 */
static void apply_register_table(const unsigned char register_table[][2])
{
    unsigned short table_index = 0;
    uint8_t reg_addr, data, data_read;
    short write_ok;
    short read_ok;
    while (1) {
        reg_addr = register_table[table_index][0];
        data = register_table[table_index][1];
        // Sentinel marks the end of this register table
        if (reg_addr == 0xff && data == 0xff) {
            break;
        }
        write_ok = sccb_write_register_retry(reg_addr, data);
#ifdef DEBUG
        serial_print(camera_config.uart,
                     "SCCB write: 0x%x=>0x%x status=%d\r\n", reg_addr, data,
                     (int)write_ok);
#endif
        HAL_Delay(SCCB_WRITE_SETTLE_DELAY_MS);

        // Read-back verification is intentionally selective for speed and bus stability
        if ((write_ok == 1) && (sccb_should_verify_register(reg_addr) == 1U)) {
            read_ok = sccb_read_register_retry(reg_addr, &data_read);
            if ((read_ok == 1) && (data != data_read)) {
#ifdef DEBUG
                serial_print(camera_config.uart,
                             "SCCB verify mismatch: reg=0x%x expected=0x%x "
                             "got=0x%x\r\n",
                             reg_addr, data, data_read);
#endif
            } else if (read_ok != 1) {
#ifdef DEBUG
                serial_print(camera_config.uart,
                             "SCCB verify failed: reg=0x%x\r\n", reg_addr);
#endif
            }
        } else if (write_ok != 1) {
#ifdef DEBUG
            serial_print(
                camera_config.uart,
                "SCCB write failed after retries: reg=0x%x data=0x%x\r\n",
                reg_addr, data);
#endif
        }
        table_index++;
    }
}

/**
 * @brief Internal resolution switch used by camera_set_resolution().
 *
 * @param resolution_profile Internal resolution index in the range [0, 5].
 */
static void apply_resolution_profile(const short resolution_profile)
{
#ifdef DEBUG
    serial_print(camera_config.uart, "Starting resolution choice \r\n");
#endif
    // Apply common JPEG pipeline first, then patch registers for the target resolution
    apply_register_table(camera_reg_jpeg_init);
    apply_register_table(camera_reg_yuv422);
    apply_register_table(camera_reg_jpeg);
    HAL_Delay(10);
    sccb_write_register(0xff, 0x01);
    HAL_Delay(10);
    sccb_write_register(0x15, 0x00);

    switch (resolution_profile) {
        case 0:
            apply_register_table(camera_reg_resolution_160x120_jpeg);
            break;
        case 1:
            apply_register_table(camera_reg_resolution_320x240_jpeg);
            break;
        case 2:
            apply_register_table(camera_reg_resolution_640x480_jpeg);
            break;
        case 3:
            apply_register_table(camera_reg_resolution_800x600_jpeg);
            break;
        case 4:
            apply_register_table(camera_reg_resolution_1024x768_jpeg);
            break;
        case 5:
            apply_register_table(camera_reg_resolution_1280x960_jpeg);
            break;
        default:
            apply_register_table(camera_reg_resolution_160x120_jpeg);
            break;
    }

#ifdef DEBUG
    serial_print(camera_config.uart, "Finalize configuration \r\n");
#endif
}

/**
 * @brief Scans a frame buffer for JPEG SOI/EOI markers and returns image size.
 *
 * @param buffer Frame buffer bytes.
 * @param buffer_size Number of bytes to scan.
 * @return JPEG size in bytes (from SOI to EOI, inclusive), or 0 if not found.
 */
static uint16_t get_jpeg_size(const uint8_t *const buffer,
                              const uint16_t buffer_size)
{
    uint16_t index;
    uint8_t header_found = 0U;

    for (index = 0U; index < (uint16_t)(buffer_size - 1U); ++index) {
        if ((header_found == 0U) && (buffer[index] == 0xFFU) &&
            (buffer[index + 1U] == 0xD8U)) {
            header_found = 1U;
#ifdef DEBUG
            serial_print(camera_config.uart, "Found header of JPEG file\r\n");
#endif
        }

        if ((header_found == 1U) && (buffer[index] == 0xFFU) &&
            (buffer[index + 1U] == 0xD9U)) {
#ifdef DEBUG
            serial_print(camera_config.uart, "Found EOF of JPEG file\r\n");
#endif
            return (uint16_t)(index + 2U);
        }
    }

    return 0U;
}

void camera_init(const camera_config_t *const config)
{
    camera_config = *config;

#ifdef DEBUG
    serial_print(camera_config.uart, "camera_init: begin\r\n");
    serial_print(camera_config.uart, "camera_init: forcing PWDN low\r\n");
#endif

    // Drive PWDN low to enable normal operation
    HAL_GPIO_WritePin(camera_config.pwdn_gpio_port,
                      camera_config.pwdn_gpio_pin, GPIO_PIN_RESET);

#ifdef DEBUG
    serial_print(camera_config.uart, "camera_init: hardware reset low\r\n");
#endif

    // Force a clean hardware reboot
    HAL_GPIO_WritePin(camera_config.reset_gpio_port,
                      camera_config.reset_gpio_pin, GPIO_PIN_RESET);

    HAL_Delay(100);

#ifdef DEBUG
    serial_print(camera_config.uart, "camera_init: hardware reset high\r\n");
#endif

    // Release reset and wait for internal clocks/registers to stabilize
    HAL_GPIO_WritePin(camera_config.reset_gpio_port,
                      camera_config.reset_gpio_pin, GPIO_PIN_SET);
    HAL_Delay(100);

    // Software reset: reset all registers to default values
#ifdef DEBUG
    serial_print(camera_config.uart,
                 "camera_init: sending software reset\r\n");
#endif

    // Select register bank 1 before issuing software reset
#ifdef DEBUG
    const short bank_select_status = sccb_write_register(0xff, 0x01);
    const short reset_status = sccb_write_register(0x12, 0x80);
    serial_print(camera_config.uart,
                 "camera_init: SCCB write bank select -> %d\r\n",
                 bank_select_status);
    serial_print(camera_config.uart, "camera_init: SCCB write reset -> %d\r\n",
                 reset_status);
#else
    sccb_write_register(0xff, 0x01);
    sccb_write_register(0x12, 0x80);
#endif
    HAL_Delay(100);

#ifdef DEBUG
    uint8_t pid;
    uint8_t ver;

    sccb_read_register(0x0a, &pid);  // pid value is 0x26
    sccb_read_register(0x0b, &ver);  // ver value is 0x42
    serial_print(camera_config.uart, "PID: 0x%x, VER: 0x%x\n", pid, ver);

    serial_print(camera_config.uart,
                 "camera_init: stopping DCMI before capture\r\n");
#endif

    // Stop DCMI clear buffer
    stop_capture();
    camera_set_resolution(camera_config.image_resolution);

#ifdef DEBUG
    serial_print(camera_config.uart, "camera_init: done\r\n");
#endif
}

void camera_set_resolution(const camera_resolution_t resolution)
{
#ifdef DEBUG
    serial_print(camera_config.uart, "camera_set_resolution: %u\r\n",
                 (unsigned int)resolution);
#endif
    switch (resolution) {

        case CAMERA_RES_160X120:
            camera_config.image_resolution = CAMERA_RES_160X120;
            apply_resolution_profile(0);
            break;

        case CAMERA_RES_320X240:
            camera_config.image_resolution = CAMERA_RES_320X240;
            apply_resolution_profile(1);
            break;

        case CAMERA_RES_640X480:
            camera_config.image_resolution = CAMERA_RES_640X480;
            apply_resolution_profile(2);
            break;

        case CAMERA_RES_800X600:
            camera_config.image_resolution = CAMERA_RES_800X600;
            apply_resolution_profile(3);
            break;

        case CAMERA_RES_1024X768:
            camera_config.image_resolution = CAMERA_RES_1024X768;
            apply_resolution_profile(4);
            break;

        case CAMERA_RES_1280X960:
            camera_config.image_resolution = CAMERA_RES_1280X960;
            apply_resolution_profile(5);
            break;
        default:
            camera_config.image_resolution = CAMERA_RES_160X120;
            apply_resolution_profile(0);
#ifdef DEBUG
            serial_print(camera_config.uart,
                         "camera_set_resolution: invalid value, fallback to "
                         "160x120\r\n");
#endif
            break;
    }
}

void camera_set_special_effect(const camera_special_effect_t effect)
{
#ifdef DEBUG
    serial_print(camera_config.uart, "Special effect value:%d\r\n", effect);
#endif
    if (effect == CAMERA_SPECIAL_EFFECT_ANTIQUE) {
        apply_register_table(camera_reg_effect_antique);
    } else if (effect == CAMERA_SPECIAL_EFFECT_BLUISH) {
        apply_register_table(camera_reg_effect_bluish);
    } else if (effect == CAMERA_SPECIAL_EFFECT_GREENISH) {
        apply_register_table(camera_reg_effect_greenish);
    } else if (effect == CAMERA_SPECIAL_EFFECT_REDDISH) {
        apply_register_table(camera_reg_effect_reddish);
    } else if (effect == CAMERA_SPECIAL_EFFECT_BLACK_WHITE) {
        apply_register_table(camera_reg_effect_black_white);
    } else if (effect == CAMERA_SPECIAL_EFFECT_NEGATIVE) {
        apply_register_table(camera_reg_effect_negative);
    } else if (effect == CAMERA_SPECIAL_EFFECT_NEGATIVE_BLACK_WHITE) {
        apply_register_table(camera_reg_effect_black_negative);
    } else if (effect == CAMERA_SPECIAL_EFFECT_NORMAL) {
        apply_register_table(camera_reg_effect_normal);
    }
}

void camera_set_white_balance(
    const camera_white_balance_mode_t white_balance_mode)
{
#ifdef DEBUG
    serial_print(camera_config.uart, "White balance mode value:%d\r\n",
                 white_balance_mode);
#endif

    sccb_write_register(0xff, 0x00);
    HAL_Delay(1);

    if (white_balance_mode == WHITE_BALANCE_AUTO) {
        sccb_write_register(0xc7, 0x00);
    } else {
        sccb_write_register(0xc7, 0x10);
    }
}

void camera_set_brightness(const camera_brightness_t brightness_level)
{
#ifdef DEBUG
    serial_print(camera_config.uart, "Brightness value:%d\r\n",
                 brightness_level);
#endif

    if (brightness_level == CAMERA_BRIGHTNESS_0) {
        apply_register_table(camera_reg_brightness_zero);
    } else if (brightness_level == CAMERA_BRIGHTNESS_1) {
        apply_register_table(camera_reg_brightness_plus1);
    } else if (brightness_level == CAMERA_BRIGHTNESS_2) {
        apply_register_table(camera_reg_brightness_plus2);
    } else if (brightness_level == CAMERA_BRIGHTNESS_3) {
        apply_register_table(camera_reg_brightness_minus1);
    } else if (brightness_level == CAMERA_BRIGHTNESS_4) {
        apply_register_table(camera_reg_brightness_minus2);
    }
}

void camera_set_light_mode(const camera_light_mode_t white_balance_mode)
{
#ifdef DEBUG
    serial_print(camera_config.uart, "Light mode value:%d\r\n",
                 white_balance_mode);
#endif

    if (white_balance_mode == CAMERA_LIGHT_MODE_AUTO) {
        camera_set_white_balance(WHITE_BALANCE_AUTO);
    } else if (white_balance_mode == CAMERA_LIGHT_MODE_SUNNY) {
        apply_register_table(camera_reg_white_balance_sunny);
    } else if (white_balance_mode == CAMERA_LIGHT_MODE_CLOUDY) {
        apply_register_table(camera_reg_white_balance_cloudy);
    } else if (white_balance_mode == CAMERA_LIGHT_MODE_OFFICE) {
        apply_register_table(camera_reg_white_balance_office);
    } else if (white_balance_mode == CAMERA_LIGHT_MODE_HOME) {
        apply_register_table(camera_reg_white_balance_home);
    }
}

void camera_set_saturation(const camera_saturation_t saturation_level)
{
#ifdef DEBUG
    serial_print(camera_config.uart, "Saturation value:%d\r\n",
                 saturation_level);
#endif

    if (saturation_level == CAMERA_SATURATION_0) {
        apply_register_table(camera_reg_saturation_zero);
    } else if (saturation_level == CAMERA_SATURATION_1) {
        apply_register_table(camera_reg_saturation_plus1);
    } else if (saturation_level == CAMERA_SATURATION_2) {
        apply_register_table(camera_reg_saturation_plus2);
    } else if (saturation_level == CAMERA_SATURATION_3) {
        apply_register_table(camera_reg_saturation_minus1);
    } else if (saturation_level == CAMERA_SATURATION_4) {
        apply_register_table(camera_reg_saturation_minus2);
    }
}

void camera_set_contrast(const camera_contrast_t contrast_level)
{
#ifdef DEBUG
    serial_print(camera_config.uart, "Contrast value:%d\r\n", contrast_level);
#endif

    if (contrast_level == CAMERA_CONTRAST_0) {
        apply_register_table(camera_reg_contrast_zero);
    } else if (contrast_level == CAMERA_CONTRAST_1) {
        apply_register_table(camera_reg_contrast_plus1);
    } else if (contrast_level == CAMERA_CONTRAST_2) {
        apply_register_table(camera_reg_contrast_plus2);
    } else if (contrast_level == CAMERA_CONTRAST_3) {
        apply_register_table(camera_reg_contrast_minus1);
    } else if (contrast_level == CAMERA_CONTRAST_4) {
        apply_register_table(camera_reg_contrast_minus2);
    }
}

uint16_t camera_capture_frame(uint8_t *const frame_buffer,
                              const int transfer_length)
{
    const uint32_t frame_buffer_addr = (uint32_t)(uintptr_t)frame_buffer;

#ifdef DEBUG
    serial_print(camera_config.uart, "camera_capture_frame: start len=%d\r\n",
                 transfer_length);
#endif

    const HAL_StatusTypeDef status =
        HAL_DCMI_Start_DMA(camera_config.dcmi_handle, DCMI_MODE_SNAPSHOT,
                           frame_buffer_addr, transfer_length);

#ifdef DEBUG
    serial_print(camera_config.uart,
                 "camera_capture_frame: HAL_DCMI_Start_DMA=%d\r\n", status);
#endif
    if (status != HAL_OK) {
        return 0U;
    }

    // Fixed wait keeps behavior deterministic when frame-complete interrupt flow is not used
    HAL_Delay(2000);

    HAL_DCMI_Suspend(camera_config.dcmi_handle);
    HAL_DCMI_Stop(camera_config.dcmi_handle);

#ifdef DEBUG
    serial_print(camera_config.uart,
                 "camera_capture_frame: suspend/stop complete\r\n");
#endif

    return get_jpeg_size((const uint8_t *)frame_buffer,
                         (uint16_t)transfer_length);
}
