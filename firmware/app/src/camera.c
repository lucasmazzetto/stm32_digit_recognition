#include "camera.h"

#define OV2640_REG_PID 0x0AU  // Product ID MSB register (bank 1)
#define OV2640_REG_VER 0x0BU  // Product ID LSB / version register (bank 1)
#define OV2640_PID_VALUE 0x26U

static camera_config_t camera_config;

static const unsigned char camera_reg_init[][2] = {
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
    {0xFF, 0x00}, {0x05, 0x00}, {0xDA, 0x00}, {0xD7, 0x03}, {0x33, 0xA0},
    {0xE5, 0x1F}, {0xE1, 0x67}, {0x00, 0x00}, {0xff, 0xff},
};

static const unsigned char camera_reg_resolution_160x120[][2] = {
    {0xFF, 0x01}, {0x12, 0x40}, {0x17, 0x11}, {0x18, 0x43}, {0x19, 0x00},
    {0x1a, 0x4b}, {0x32, 0x09}, {0x4f, 0xca}, {0x50, 0xa8}, {0x5a, 0x23},
    {0x6d, 0x00}, {0x39, 0x12}, {0x35, 0xda}, {0x22, 0x1a}, {0x37, 0xc3},
    {0x23, 0x00}, {0x34, 0xc0}, {0x36, 0x1a}, {0x06, 0x88}, {0x07, 0xc0},
    {0x0d, 0x87}, {0x0e, 0x41}, {0x4c, 0x00}, {0xFF, 0x00}, {0xe0, 0x04},
    {0xc0, 0x64}, {0xc1, 0x4b}, {0x86, 0x35}, {0x50, 0x92}, {0x51, 0xc8},
    {0x52, 0x96}, {0x53, 0x00}, {0x54, 0x00}, {0x55, 0x00}, {0x57, 0x00},
    {0x5a, 0x28}, {0x5b, 0x1e}, {0x5c, 0x00}, {0xe0, 0x00}, {0xff, 0xff}};

/**
 * @brief Stops DCMI capture and waits briefly for peripheral stabilization.
 */
static void stop_capture(void)
{
    // Stop any ongoing capture before reconfiguring or starting a new snapshot
    HAL_DCMI_Stop(camera_config.dcmi_handle);
    // Short settle delay avoids immediate start/stop race on the peripheral
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

    // SCCB write payload packs register address and value in one transfer
    buffer[0] = reg_addr;
    buffer[1] = data;

    // OV2640 SCCB write address is 0x60 in 8-bit HAL address format
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

    // SCCB read flow: write target register first, then issue a 1-byte read
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
    // Reinitialize I2C peripheral to recover from stuck bus or NACK storms
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

    // Retry writes to tolerate transient SCCB/I2C timing issues
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

    // Retry reads with bus recovery when read transaction fails
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
 * @brief Verifies the sensor identity before configuration.
 *
 * Selects the sensor register bank and reads PID/VER. Only PID is enforced so
 * die revisions and clones with a different VER still pass.
 *
 * @return HAL_OK when a sensor with the expected PID answered, otherwise
 *         HAL_ERROR.
 */
static HAL_StatusTypeDef camera_probe(void)
{
    uint8_t pid = 0U;
    uint8_t ver = 0U;

    // Software reset restores register defaults, so select bank 1 explicitly
    if (sccb_write_register_retry(0xff, 0x01) != 1) {
        return HAL_ERROR;
    }

    if (sccb_read_register_retry(OV2640_REG_PID, &pid) != 1) {
        return HAL_ERROR;
    }

    if (sccb_read_register_retry(OV2640_REG_VER, &ver) != 1) {
        return HAL_ERROR;
    }

#ifdef DEBUG
    serial_print(camera_config.uart, "PID: 0x%x, VER: 0x%x\r\n", pid, ver);
#endif

    if (pid != OV2640_PID_VALUE) {
#ifdef DEBUG
        serial_print(camera_config.uart,
                     "camera_probe: PID mismatch expected=0x%x got=0x%x\r\n",
                     OV2640_PID_VALUE, pid);
#endif
        return HAL_ERROR;
    }

    return HAL_OK;
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
 * @return HAL_OK when every register write succeeded, HAL_ERROR on the first
 *         write that fails after retries.
 */
static HAL_StatusTypeDef apply_register_table(
    const unsigned char register_table[][2])
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
        // Fail fast: retries and bus recovery already happened inside
        // sccb_write_register_retry, so this register is unrecoverable
        if (write_ok != 1) {
#ifdef DEBUG
            serial_print(
                camera_config.uart,
                "SCCB write failed after retries: reg=0x%x data=0x%x\r\n",
                reg_addr, data);
#endif
            return HAL_ERROR;
        }
        HAL_Delay(SCCB_WRITE_SETTLE_DELAY_MS);

        // Read-back verification is intentionally selective for speed and bus stability
        if (sccb_should_verify_register(reg_addr) == 1U) {
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
        }
        table_index++;
    }

    return HAL_OK;
}

HAL_StatusTypeDef camera_init(const camera_config_t *const config)
{
    // Store handles and GPIO mapping for runtime camera operations
    camera_config = *config;

#ifdef DEBUG
    serial_print(camera_config.uart, "camera_init: begin\r\n");
    serial_print(camera_config.uart, "camera_init: forcing PWDN low\r\n");
#endif

    // Drive PWDN low so camera leaves power-down mode
    HAL_GPIO_WritePin(camera_config.pwdn_gpio_port,
                      camera_config.pwdn_gpio_pin, GPIO_PIN_RESET);

#ifdef DEBUG
    serial_print(camera_config.uart, "camera_init: hardware reset low\r\n");
#endif

    // Assert RESET low to start from a known camera state
    HAL_GPIO_WritePin(camera_config.reset_gpio_port,
                      camera_config.reset_gpio_pin, GPIO_PIN_RESET);

    HAL_Delay(100);

#ifdef DEBUG
    serial_print(camera_config.uart, "camera_init: hardware reset high\r\n");
#endif

    // Release RESET and wait for internal clocks/registers to stabilize
    HAL_GPIO_WritePin(camera_config.reset_gpio_port,
                      camera_config.reset_gpio_pin, GPIO_PIN_SET);
    HAL_Delay(100);

    // Perform software reset to restore OV2640 internal defaults
#ifdef DEBUG
    serial_print(camera_config.uart,
                 "camera_init: sending software reset\r\n");
#endif

    // Select bank 1 before writing COM7 reset bit
    if (sccb_write_register_retry(0xff, 0x01) != 1) {
        return HAL_ERROR;
    }

    if (sccb_write_register_retry(0x12, 0x80) != 1) {
        return HAL_ERROR;
    }

    HAL_Delay(100);

    // Abort before writing the register tables when no expected sensor answers
    if (camera_probe() != HAL_OK) {
        return HAL_ERROR;
    }

#ifdef DEBUG
    serial_print(camera_config.uart,
                 "camera_init: stopping DCMI before capture\r\n");
#endif

    // Ensure DCMI is idle before applying camera register profile
    stop_capture();

#ifdef DEBUG
    serial_print(camera_config.uart, "Applying fixed profile 160x120 YUV422\r\n");
#endif
    // Apply vendor initialization table before output format setup
    if (apply_register_table(camera_reg_init) != HAL_OK) {
        return HAL_ERROR;
    }

    // Switch DSP output path to YUV422
    if (apply_register_table(camera_reg_yuv422) != HAL_OK) {
        return HAL_ERROR;
    }

    // Ensure register bank and output control are in expected state
    HAL_Delay(10);
    if (sccb_write_register_retry(0xff, 0x01) != 1) {
        return HAL_ERROR;
    }

    HAL_Delay(10);
    if (sccb_write_register_retry(0x15, 0x00) != 1) {
        return HAL_ERROR;
    }

    // Apply fixed 160x120 windowing/scaling registers
    if (apply_register_table(camera_reg_resolution_160x120) != HAL_OK) {
        return HAL_ERROR;
    }

#ifdef DEBUG
    serial_print(camera_config.uart, "Finalize configuration\r\n");
    serial_print(camera_config.uart, "camera_init: done\r\n");
#endif

    return HAL_OK;
}

uint32_t camera_capture_frame(uint8_t *const frame_buffer,
                              uint32_t transfer_length)
{
    const uint32_t frame_buffer_addr = (uint32_t)(uintptr_t)frame_buffer;
    const uint32_t transfer_words = transfer_length / 4U;
    const uint32_t transfer_remainder = transfer_length % 4U;
    DMA_HandleTypeDef *dma_handle;
    uint8_t dma_started = 0U;
    uint32_t start_tick;

#ifdef DEBUG
    serial_print(camera_config.uart,
                 "camera_capture_frame: start len=%lu bytes (%lu words)\r\n",
                 (unsigned long)transfer_length,
                 (unsigned long)transfer_words);
#endif

    // HAL DCMI DMA length unit is 32-bit words from DCMI DR
    if ((transfer_length == 0U) || (transfer_words == 0U) ||
        (transfer_remainder != 0U)) {
#ifdef DEBUG
        serial_print(camera_config.uart,
                     "camera_capture_frame: invalid transfer length=%lu "
                     "(must be non-zero and multiple of 4)\r\n",
                     (unsigned long)transfer_length);
#endif
        return 0U;
    }

    // Clear stale DCMI flags so wait loop observes the new frame only
    __HAL_DCMI_CLEAR_FLAG(camera_config.dcmi_handle,
                          DCMI_FLAG_FRAMERI | DCMI_FLAG_OVRRI |
                              DCMI_FLAG_ERRRI | DCMI_FLAG_VSYNCRI |
                              DCMI_FLAG_LINERI);

    // Start one snapshot transfer into the provided frame buffer
    const HAL_StatusTypeDef status =
        HAL_DCMI_Start_DMA(camera_config.dcmi_handle, DCMI_MODE_SNAPSHOT,
                           frame_buffer_addr, transfer_words);

#ifdef DEBUG
    serial_print(camera_config.uart,
                 "camera_capture_frame: HAL_DCMI_Start_DMA=%d\r\n", status);
#endif

    if (status != HAL_OK) {
        return 0U;
    }

    dma_handle = camera_config.dcmi_handle->DMA_Handle;
    if (dma_handle == NULL) {
        dma_started = 1U;
    }

    // Wait for frame-complete instead of fixed delay to avoid partial frames
    start_tick = HAL_GetTick();

    while ((__HAL_DCMI_GET_FLAG(camera_config.dcmi_handle, DCMI_FLAG_FRAMERI) ==
            RESET) ||
           (dma_started == 0U)) {

        // Require at least one DMA beat to avoid accepting stale FRAMERI
        if ((dma_handle != NULL) &&
            (__HAL_DMA_GET_COUNTER(dma_handle) < transfer_words)) {
            dma_started = 1U;
        }

        // Abort early on hardware overrun or DCMI capture errors
        if ((__HAL_DCMI_GET_FLAG(camera_config.dcmi_handle, DCMI_FLAG_OVRRI) !=
             RESET) ||
            (__HAL_DCMI_GET_FLAG(camera_config.dcmi_handle, DCMI_FLAG_ERRRI) !=
             RESET)) {

#ifdef DEBUG
            serial_print(camera_config.uart,
                         "camera_capture_frame: DCMI error/overrun\r\n");
#endif

            HAL_DCMI_Stop(camera_config.dcmi_handle);
            return 0U;
        }

        // Timeout protects main loop from hanging if frame complete never arrives
        if ((HAL_GetTick() - start_tick) > CAMERA_CAPTURE_TIMEOUT_MS) {
#ifdef DEBUG
            serial_print(camera_config.uart,
                         "camera_capture_frame: frame timeout\r\n");
#endif
            HAL_DCMI_Stop(camera_config.dcmi_handle);
            return 0U;
        }
    }

    // FRAMERI can assert before DMA fully drains FIFO into memory
    if (dma_handle != NULL) {
        uint8_t dma_drain_timed_out = 0U;
        start_tick = HAL_GetTick();

        // Wait until DMA engine is ready and no bytes remain pending
        while ((HAL_DMA_GetState(dma_handle) != HAL_DMA_STATE_READY) ||
               (__HAL_DMA_GET_COUNTER(dma_handle) != 0U)) {

            // If drain wait times out, continue with stop and report in DEBUG
            if ((HAL_GetTick() - start_tick) > CAMERA_DMA_DRAIN_TIMEOUT_MS) {
                dma_drain_timed_out = 1U;
#ifdef DEBUG
                serial_print(camera_config.uart,
                             "camera_capture_frame: DMA drain timeout, "
                             "state=%d ndtr=%lu\r\n",
                             (int)HAL_DMA_GetState(dma_handle),
                             (unsigned long)__HAL_DMA_GET_COUNTER(dma_handle));
#endif
                break;
            }
        }

        // Minimal safety fix: reject frame when DMA drain timeout occurs
        if (dma_drain_timed_out == 1U) {
            HAL_DCMI_Stop(camera_config.dcmi_handle);
            return 0U;
        }
    }

    HAL_DCMI_Suspend(camera_config.dcmi_handle);

    // Stop snapshot so next capture always starts from a clean state
    HAL_DCMI_Stop(camera_config.dcmi_handle);

#ifdef DEBUG
    serial_print(camera_config.uart,
                 "camera_capture_frame: suspend/stop complete\r\n");
#endif

    return transfer_words * 4U;
}
