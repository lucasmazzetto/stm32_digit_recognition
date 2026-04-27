# STM32 Digit Recognition

This project captures images from an **OV2640** camera module and runs a [quantized neural network](https://github.com/lucasmazzetto/quantized_digit_recognition) embedded on the **STM32 NUCLEO F446RE** to predict handwritten digits using the microcontroller, with Integer-only operations on the MCU (no floating-point).


<img width="480" height="311" alt="ezgif-4314612878122757" src="https://github.com/user-attachments/assets/c8e7abfe-939d-44d8-b542-44eb0bd97c8b" />


The firmware now performs live camera bring-up and runs a continuous capture, preprocess, and inference loop. It also applies an input gate (known/unknown filter): after inference, it compares the current input image and the model logits against reference centers using cosine distance, and only accepts the digit when both distances are below their thresholds. After prediction and filtering, the result is shown on the 7-segment display.

At a hardware level, the **OV2640** sends pixel data through the DCMI interface while SCCB/I2C is used for camera control, and DMA transfers DCMI data into a RAM frame buffer with low CPU overhead. From this buffer, the firmware runs grayscale/crop/resize preprocessing, executes integer-only inference, applies the filter, and updates the display with the final result.

This is the current firmware execution sequence implemented in this project:

1. Boot and initialize `USART2`, `DCMI`, and `I2C2`.
2. Apply the OV2640 reset and power-up sequence.
3. Probe the camera over SCCB and print `PID` / `VER` in `DEBUG`.
4. Configure the sensor for `160x120` YUV422 capture.
5. Run a continuous loop: capture YUV422, convert to grayscale, center-crop to `120x120`, resize to `28x28`.
6. Convert `28x28` pixels to fixed-point format input using LUT and run quantized inference.
7. In `DEBUG`, print `NN_PRED <digit>` when input passes filtering, or `NN_PRED unknown` when it fails known/unknown filtering.
8. In `DEBUG`, pressing `B1` sends the latest `28x28` cached frame over UART using `FRAME_BEGIN`/`FRAME_END`.

Some useful scripts are also available to generate parameters and debug the application, such as:

- `capture.py` is used with firmware built in `DEBUG` to capture debug information and camera frames.
- `generate_distance.py` creates `distance.c` and `distance.h` from a collected dataset to calibrate cosine-distance filter.
- `generate_lut.py` creates a lookup table for input normalization and fixed-point conversion without float operations on hardware.
- `inference.py` provides integer-only host inference to verify neural-network performance with captured images.

In the next sections, you’ll find detailed instructions covering used materials, installation, hardware configuration, and usage.

## 🛠️ Materials

The following materials are used to assemble and run this project:

- MCU Nucleo board
- Camera module
- 7-segment display (common cathode)
- 330 Ω resistors (for display wiring)

## 💻 Installation

To clone this repository with all required dependencies, use:

```bash
git clone --recurse-submodules git@github.com:lucasmazzetto/stm32_digit_recognition.git
```

This firmware is based on STM32 HAL, so the safest workflow is to use the vendor toolchain, [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html), to compile and deploy the project. In **STM32CubeIDE**, import `firmware/NUCLEO_F446RE/`, and the project should be ready to build and flash to the MCU. Make sure the target is **STM32 NUCLEO F446RE**, because other MCU variants may require pinout and configuration changes.

To use the scripts provided for debugging or code generation, go to the project directory and install the dependencies listed in `requirements.txt`:

```bash
python3 -m venv .venv

source .venv/bin/activate

python -m pip install --upgrade pip

pip install -r requirements.txt
```

## ⚙️ Hardware Configuration

This hardware configuration, generated with **STM32CubeMX**, documents the complete peripheral mapping used by this project on the **STM32 NUCLEO-F446RE** to run the **OV2640** capture and embedded inference pipeline, including SCCB/I2C camera control, DCMI + DMA frame acquisition, MCO-based camera clock generation, and UART/DMA debug and frame-transfer support.

In the next sections, the pin mapping and the specific configuration of each peripheral used in this project are presented in detail.

### Pin Configuration

PERIPHERAL | MODE | FUNCTION | STM32 PIN | NUCLEO CONNECTOR | NUCLEO PIN
--- | --- | --- | --- | --- | ---
DCMI | Slave 8 bits External Synchro | DCMI_D0 | PC6 | CN10 | 4
DCMI | Slave 8 bits External Synchro | DCMI_D1 | PC7 | CN10 | 19
DCMI | Slave 8 bits External Synchro | DCMI_D2 | PC8 | CN10 | 2
DCMI | Slave 8 bits External Synchro | DCMI_D3 | PC9 | CN10 | 1
DCMI | Slave 8 bits External Synchro | DCMI_D4 | PC11 | CN7 | 2
DCMI | Slave 8 bits External Synchro | DCMI_D5 | PB6 | CN10 | 17
DCMI | Slave 8 bits External Synchro | DCMI_D6 | PB8 | CN10 | 3
DCMI | Slave 8 bits External Synchro | DCMI_D7 | PB9 | CN10 | 5
DCMI | Slave 8 bits External Synchro | DCMI_HSYNC | PA4 | CN7 | 32
DCMI | Slave 8 bits External Synchro | DCMI_PIXCLK | PA6 | CN10 | 13
DCMI | Slave 8 bits External Synchro | DCMI_VSYNC | PB7 | CN7 | 21
I2C2 | I2C | I2C2_SCL | PB10 | CN10 | 25
I2C2 | I2C | I2C2_SDA | PC12 | CN7 | 3
RCC | Clock-out-1 | RCC_MCO_1 (`XCLK`) | PA8 | CN10 | 23
USART2 | Asynchronous | USART2_TX | PA2 | ST-LINK VCP / CN10 | 35
USART2 | Asynchronous | USART2_RX | PA3 | ST-LINK VCP / CN10 | 37
GPIO | Output | CAMERA_PWDN | PB1 | CN10 | 24
GPIO | Output | CAMERA_RESET | PB12 | CN10 | 16
GPIO | GPIO_MODE_IT_FALLING (EXTI13) | USER_BTN | PC13 | On-board button / CN7 | 23
GPIO | Output | 7 Segments A | PA1 | CN7 | 30
GPIO | Output | 7 Segments B | PB0 | CN7 | 34
GPIO | Output | 7 Segments C | PC10 | CN7 | 1
GPIO | Output | 7 Segments D | PC0 | CN7 | 38
GPIO | Output | 7 Segments E | PC1 | CN7 | 36
GPIO | Output | 7 Segments F | PA0 | CN7 | 28
GPIO | Output | 7 Segments G | PD2 | CN7 | 4

### Camera Control GPIO Default Levels

This section defines the default startup states of the camera control GPIOs used during bring-up. These levels ensure the OV2640 starts in an active and controllable state before the firmware applies sensor configuration.

GPIO | Purpose | Startup level
--- | --- | ---
PB1 (`PWDN`) | Camera power-down control | `LOW`
PB12 (`RESET`) | Camera reset control | `HIGH`


### DCMI Configuration

The Digital Camera Interface (DCMI) is the peripheral used to receive parallel pixel data from the **OV2640** module directly in hardware. In this project, it captures camera frames that are then transferred by DMA to memory for preprocessing and inference.

OPTION | VALUE
--- | ---
Synchronization mode | Hardware
Extended data mode | 8-bit
Pixel clock polarity | Active on Rising edge
Vertical synchronization polarity | Active Low
Horizontal synchronization polarity | Active Low
Frequency of frame capture | All frames are captured
JPEG mode | Disabled
Byte select mode | All bytes
Byte select start | Odd
Line select mode | All lines
Line select start | Odd

### DMA Configuration

DMA is used in this project to transfer data between peripherals and memory with minimal CPU overhead. Here, one stream handles camera frame capture from DCMI to RAM, and another stream handles UART frame payload delivery to the host. Runtime debug text logs are still sent with regular UART transmit calls.

DMA request | Stream | Direction | Priority
--- | --- | --- | ---
DCMI | DMA2_Stream1 | Peripheral To Memory | Very High
USART2_TX | DMA1_Stream6 | Memory To Peripheral | Very High

#### DMA2_Stream1 Request Settings

`DMA2_Stream1` is dedicated to DCMI capture, moving incoming camera data into the frame buffer. Its configuration prioritizes snapshot transfer during continuous acquisition and preprocessing.

OPTION | VALUE
--- | ---
Mode | Normal
Use FIFO | Enable
FIFO Threshold | Full
Peripheral Increment | Disable
Memory Increment | Enable
Peripheral Data Width | Word
Memory Data Width | Byte
Peripheral Burst Size | Single
Memory Burst Size | Single

#### DMA1_Stream6 Request Settings

`DMA1_Stream6` is used by `USART2_TX` to send frame payload data (for cached image transfer) to the host. Regular debug log messages use standard UART transmit.

OPTION | VALUE
--- | ---
Mode | Normal
Use FIFO | Enable
FIFO Threshold | Full
Peripheral Increment | Disable
Memory Increment | Enable
Peripheral Data Width | Byte
Memory Data Width | Byte
Peripheral Burst Size | Single
Memory Burst Size | Single

### I2C2 Configuration

`I2C2` is used as the SCCB/I2C control channel for the **OV2640**, handling sensor register reads/writes during camera initialization and configuration.

> [!NOTE]
> `I2C2` is currently configured at `1 kHz` for debug stability during **OV2640** register programming. This is intentionally and should be treated as a bring-up setting.

OPTION | VALUE
--- | ---
I2C speed mode | Standard Mode
I2C speed frequency (Hz) | 1000
Duty cycle | 2
Primary address length selection | 7-bit
Primary slave address | 0
Dual address mode | Disabled
General call | Disabled
Clock no stretch mode | Disabled
GPIO pull-up on `PB10` / `PC12` | Enabled in CubeMX for debug bring-up

### RCC / Clock Configuration

This section summarizes the clock tree used by the firmware. These settings define system and bus frequencies, as well as the `MCO1` clock output used to generate the camera `XCLK` signal.

OPTION | VALUE
--- | ---
Oscillator source used by firmware | HSI
PLL source | HSI
PLLM | 16
PLLN | 336
PLLP | DIV4
SYSCLK | 84 MHz
AHB clock | 84 MHz
APB1 clock | 42 MHz
APB2 clock | 84 MHz
Flash latency | 2
MCO1 source | PLLCLK
MCO1 divider | DIV3
`PA8` camera `XCLK` output | 28 MHz

### USART2 Configuration

`USART2` provides the serial interface to the host for runtime logs and debug frame transfer.

OPTION | VALUE
--- | ---
Baud rate | 115200
Word length | 8 bits
Parity | None
Stop bits | 1
Data direction | Receive and Transmit
Hardware flow control | None
Oversampling | 16

### NVIC Configuration

This section lists the interrupt priorities enabled for the project, including DCMI, DMA, USART2, and EXTI, which coordinate capture, transfer, and debug-trigger events.

Interrupt table | Enable | Preemption Priority | SubPriority
--- | --- | --- | ---
Non maskable interrupt | true | 0 | 0
Hard fault interrupt | true | 0 | 0
Memory management fault | true | 0 | 0
Pre-fetch fault, memory access fault | true | 0 | 0
Undefined instruction or illegal state | true | 0 | 0
System service call via SWI instruction | true | 0 | 0
Debug monitor | true | 0 | 0
Pendable request for system service | true | 0 | 0
System tick timer | true | 0 | 0
DMA1 stream6 global interrupt | true | 0 | 0
DMA2 stream1 global interrupt | true | 0 | 0
DCMI global interrupt | true | 0 | 0
USART2 global interrupt | true | 0 | 0
EXTI line[15:10] interrupts | true | 5 | 0

### 7-Segment Display Mapping

Display type is `common-cathode`, so segment ON means GPIO `HIGH` (`3.3V`). Each segment must be connected to the MCU through a current-limiting resistor (for example, `330 Ω`). Corresponding STM32 pins:

```text
  --PA1--
 |       |
PA0     PB0
 |       |
  --PD2--
 |       |
PC1     PC10
 |       |
  --PC0--
```

## 🚀 Usage

After software installation and hardware configuration are complete, the firmware is ready to be built and deployed directly to the MCU.

However, if you want to create your own model from scratch, you must retrain the neural network by following the [quantized neural network](https://github.com/lucasmazzetto/quantized_digit_recognition) documentation to generate `params.h` and `params.c`, and then run `scripts/generate_lut.py` and `scripts/generate_distance.py` to complete the firmware configuration.

### Capture

A host utility is provided at `scripts/capture.py` to receive camera captures from the board and save them as grayscale `.pgm` images. This flow requires firmware built with `DEBUG`, and the script also prints general debug information emitted by the running firmware in the serial output.

Run the capture tool:

```bash
python3 scripts/capture.py --port /dev/ttyACM0 --baud 115200 --output capture.pgm
```

Available parameters:

- `--port` (default: `/dev/ttyACM0`): serial device path.
- `--baud` (default: `115200`): serial baud rate.
- `--output` (default: `capture.pgm`): output file path for the captured image; if no extension is provided, `.pgm` is added automatically.
- `--frame-timeout` / `--frame_timeout` (default: infinite wait): maximum seconds to wait for the next frame and payload (`0` also means infinite wait).
- `--frame-end-timeout` / `--frame_end_timeout` (default: `2.0`): seconds to wait for trailing serial logs after each capture.

The script runs in continuous mode and overwrites the same output file on each captured frame by default. When the script is listening, press the blue user button on the Nucleo board to request frame transfer.

### Inference

`scripts/inference.py` runs one-image host inference using the same quantized C implementation used by the embedded project. It reads a grayscale `.pgm` image, converts pixels to fixed-point input using the LUT logic, executes `convnet_forward`, and prints the predicted digit.

This is useful to validate captured frames quickly on the host before reflashing firmware, so you can isolate model/quantization behavior from MCU integration issues.

Run inference:

```bash
python3 scripts/inference.py --image capture.pgm
```

Available parameters:

- `--lib-path` / `--lib_path` (default: `external/quantized_digit_recognition/lib/convnet.so`): path to the compiled quantized shared library used for inference.
- `--image` (default: `capture.pgm`): input grayscale PGM image (`P5`) to classify.
- `--input-frac-bits` / `--input_frac_bits` (default: `16`): fractional bits used for fixed-point input conversion.

If the shared library is not available yet, build it first inside `external/quantized_digit_recognition` following the documentation of [quantized neural network](https://github.com/lucasmazzetto/quantized_digit_recognition) so the script can load `convnet.so`.

### Generate LUT

`scripts/generate_lut.py` generates the lookup table used to convert `uint8` camera pixels to model input fixed-point values directly on the MCU. The script writes `lut.h` and `lut.c`, which are consumed by the firmware preprocessing pipeline.

This LUT avoids floating-point normalization in embedded runtime, reducing CPU cost and ensuring host and firmware conversion stay aligned.

Generate LUT files:

```bash
python3 scripts/generate_lut.py
```

Available parameters:

- `--output-dir` / `--output_dir` (default: `firmware/app`): base directory where `include/lut.h` and `src/lut.c` are generated.
- `--frac-bits` / `--frac_bits` (default: `16`): fractional bits used to build the fixed-point LUT values.

### Generate Distance

`scripts/generate_distance.py` builds the known/unknown filter parameters used by firmware. It loads a labeled dataset, runs quantized inference for each sample, keeps correctly classified samples per class, computes class center vectors (image and logits), and derives class thresholds from cosine-distance percentiles.

The dataset must be organized with one folder per class label (0 to 9 by default), and each class folder must contain grayscale `.pgm` images. For the default setup, images should be 28x28 so they match the firmware input vector size (784).

The current distance parameters in this project were generated from a dataset of real images captured by the OV2640 camera and then lightly augmented to add variation, helping keep the known/unknown filter more robust in practical use.

The output is written to `distance.h` and `distance.c`, which are used at runtime to accept or reject predictions using integer-only cosine-distance checks.

Generate distance files:

```bash
python3 scripts/generate_distance.py --dataset-dir /path/to/dataset
```

Available parameters:

- `--dataset-dir` (default: `/mnt/Data/Datasets/mnist/augmented`): root folder with class subfolders (`0` to `9`) containing `.pgm` images.
- `--output-dir` (default: `firmware/app`): base directory where `include/distance.h` and `src/distance.c` are generated.
- `--lib-path` / `--lib_path` (default: `external/quantized_digit_recognition/lib/convnet.so`): quantized shared library used to run host inference during calibration.
- `--percentile` (default: `99.0`): percentile used to select class-wise cosine-distance thresholds.
- `--frac-bits` (default: `16`): fixed-point fractional bits used in distance computation.
- `--class-count` (default: `10`): number of classes expected in the dataset.
- `--image-vector-size` (default: `784`): flattened image vector size (for `28x28` images).
- `--logits-vector-size` (default: `10`): logits vector size produced by the model.
