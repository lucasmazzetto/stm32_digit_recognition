/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "convnet.h"
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
static const int input_sample[] = {
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -22359,  29555,  16191,  12079, -34696, -47032,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536,  48574,  65022,
     65022,  65022,  65022,  58340,  36238,  36238,  36238,  36238,
     36238,  36238,  36238,  36238,  21845, -38808, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -31097,  -6939, -28527,  -6939,  18247,  51144,
     65022,  50116,  65022,  65022,  65022,  62966,  52172,  65022,
     65022,   6425, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -56798, -31611, -58340, -31097, -31097,
    -31097, -35210, -54742,  55770,  65022, -11051, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -22873,  64508,
     41892, -56284, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -54228,  54228,  65536, -22873, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536,    771,  65022,  56798,
    -42920, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -35210,  62452,  65022, -33668, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536,   2827,  65022,  30583, -62966,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -60910,
     39836,  61938, -35724, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536,   -771,  65022,  28013, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -26985,  63480,
     57826, -36238, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -55770,  48060,  65022,  19789, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -63994,  38808,  65022,  47032,
    -47546, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -46004,  65022,  65022, -25957, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -49602,  49602,  65022,  -6425, -65022,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536,   2827,
     65022,  65022, -38808, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -34182,  58854,  65022,  65022, -38808, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536,  -3341,  65022,
     65022,  47032, -44976, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536,  -3341,  65022,  40864, -56284, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536,
    -65536, -65536, -65536, -65536, -65536, -65536, -65536, -65536
};

static int conv1_out[BATCH_SIZE * CONV1_OUT_CHANNELS * CONV1_OUT_HEIGHT * CONV1_OUT_WIDTH];
static int pool1_out[BATCH_SIZE * CONV1_OUT_CHANNELS * POOL1_OUT_HEIGHT * POOL1_OUT_WIDTH];
static int conv2_out[BATCH_SIZE * CONV2_OUT_CHANNELS * CONV2_OUT_HEIGHT * CONV2_OUT_WIDTH];
static int pool2_out[BATCH_SIZE * CONV2_OUT_CHANNELS * POOL2_OUT_HEIGHT * POOL2_OUT_WIDTH];
static int linear1_out[BATCH_SIZE * LINEAR1_OUT_FEATURES];
static int linear2_out[BATCH_SIZE * LINEAR2_OUT_FEATURES];
static int output[BATCH_SIZE * OUTPUT_DIM];
static unsigned int class_indices[BATCH_SIZE];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void report_prediction(unsigned int prediction)
{
  char msg[64];
  int len = snprintf(msg, sizeof(msg), "Predicted class: %lu\r\n", (unsigned long)prediction);

  if (len > 0) {
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, (uint16_t)len, HAL_MAX_DELAY);
  }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  if ((sizeof(input_sample) / sizeof(input_sample[0])) != INPUT_FLAT_SIZE) {
    Error_Handler();
  }

  convnet_forward(input_sample, conv1_out, pool1_out,
                  conv2_out, pool2_out, linear1_out,
                  linear2_out, output, class_indices);



  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    report_prediction(class_indices[0]);
    HAL_Delay(500);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
