/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
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
#include "delay_micros.h"
#include "stdio.h"
#include "stdbool.h"

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
 UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
GPIO_InitTypeDef GPIO_Mode; // this variable represents gpio on start, when initting gpio, copy init struct to this variable
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void OWGPIOInit(bool output)
{
//#warning make sure there is no interference with other pins
  /*Configure GPIO pin : OneWire_BB_Pin */
	GPIO_Mode.Pin = OneWire_BB_Pin;
	if (output) GPIO_Mode.Mode = GPIO_MODE_OUTPUT_OD;
	else GPIO_Mode.Mode = GPIO_MODE_INPUT;
	GPIO_Mode.Pull = GPIO_NOPULL;
	GPIO_Mode.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(OneWire_BB_GPIO_Port, &GPIO_Mode);

}

void OWReset(){
	if (GPIO_Mode.Mode == GPIO_MODE_INPUT) {OWGPIOInit(true);}
	delay_us(100); // a small amount of time to init
	HAL_GPIO_WritePin(OneWire_BB_GPIO_Port, OneWire_BB_Pin, GPIO_PIN_RESET);
	delay_us(480); // 480 us is OW reset command
	HAL_GPIO_WritePin(OneWire_BB_GPIO_Port, OneWire_BB_Pin, GPIO_PIN_SET);
	delay_us(480); // device responds, i don't care
	HAL_Delay(1); // debug
}

void OWZeroWrite(){
	if (GPIO_Mode.Mode == GPIO_MODE_INPUT) {OWGPIOInit(true);}
	delay_us(1); // recovery time
	HAL_GPIO_WritePin(OneWire_BB_GPIO_Port, OneWire_BB_Pin, GPIO_PIN_RESET);
	delay_us(120); // 60-120 us is OW 0 command
	HAL_GPIO_WritePin(OneWire_BB_GPIO_Port, OneWire_BB_Pin, GPIO_PIN_SET);
	delay_us(0);
}

void OWOneWrite(){
	if (GPIO_Mode.Mode == GPIO_MODE_INPUT) {OWGPIOInit(true);}
	delay_us(1); // recovery time
	HAL_GPIO_WritePin(OneWire_BB_GPIO_Port, OneWire_BB_Pin, GPIO_PIN_RESET);
	delay_us(1); // 1-15 us is OW 1 command
	HAL_GPIO_WritePin(OneWire_BB_GPIO_Port, OneWire_BB_Pin, GPIO_PIN_SET);
	delay_us(119); // up for some time until the next command
}

void OWByteWrite(char input[8]){
	if (GPIO_Mode.Mode == GPIO_MODE_INPUT) {OWGPIOInit(true);}
	for (int i = 0; i < 8; i++){
		if (input[i] == '1'){
			OWOneWrite();
		}
		else{
			OWZeroWrite();
		}
	}
}

bool OWReadBit(){
	if (GPIO_Mode.Mode == GPIO_MODE_INPUT) {OWGPIOInit(true);}
	bool record = HAL_GPIO_ReadPin(OneWire_BB_GPIO_Port, OneWire_BB_Pin);
	if (record) delay_us(1);
	delay_us(30);
	record = HAL_GPIO_ReadPin(OneWire_BB_GPIO_Port, OneWire_BB_Pin);
	delay_us(90);
	return record;
}

bool OW0ScratchpadMem[8] = {0}; // t lsb
bool OW1ScratchpadMem[8] = {0}; // t msb
bool OW2ScratchpadMem[8] = {0}; // t max
bool OW3ScratchpadMem[8] = {0}; // t min
bool OW4ScratchpadMem[8] = {0}; // config
bool OW8ScratchpadMem[8] = {0}; // CRC

void OWReadScratchpad(){
	if (GPIO_Mode.Mode == GPIO_MODE_INPUT) {OWGPIOInit(true);}
	OWReset();
	OWByteWrite("11001100"); // skip rom - we've got only one ds18b20
	OWByteWrite("10111110"); // read scratch
	HAL_GPIO_WritePin(OneWire_BB_GPIO_Port, OneWire_BB_Pin, GPIO_PIN_RESET);
	delay_us(2);
	HAL_GPIO_WritePin(OneWire_BB_GPIO_Port, OneWire_BB_Pin, GPIO_PIN_SET);
	OWGPIOInit(false);

	for (int i = 0; i < 8; i++){
			OW0ScratchpadMem[i] = OWReadBit();
		}
	for (int i = 0; i < 8; i++){
			OW1ScratchpadMem[i] = OWReadBit();
		}
	for (int i = 0; i < 8; i++){
			OW2ScratchpadMem[i] = OWReadBit();
		}
	for (int i = 0; i < 8; i++){
			OW3ScratchpadMem[i] = OWReadBit();
		}
	for (int i = 0; i < 8; i++){
			OW4ScratchpadMem[i] = OWReadBit();
		}
	for (int i = 0; i < 8; i++){
			OWReadBit();
		}
	for (int i = 0; i < 8; i++){
			OWReadBit();
		}
	for (int i = 0; i < 8; i++){
			OWReadBit();
	}
	for (int i = 0; i < 8; i++){
			OW8ScratchpadMem[i] = OWReadBit();
		}
}

float OWReadTemp(){
	OWGPIOInit(true);
	OWReset();
	OWByteWrite("11001100"); // skip rom - we've got only one ds18b20
	OWByteWrite("01000100"); // convert T
	//OWGPIOInit(false); // why do we do that?
	HAL_Delay(1000);
	OWReadScratchpad();
	int sign = OW0ScratchpadMem[0];
	float temp = OW1ScratchpadMem[7]*0.0625 + OW1ScratchpadMem[6]*0.125 + OW1ScratchpadMem[5]*0.5 + OW1ScratchpadMem[4]*1 + OW1ScratchpadMem[3]*2 + OW1ScratchpadMem[2]*4 + OW1ScratchpadMem[1]*8 + OW1ScratchpadMem[0]*16 + OW0ScratchpadMem[7]*32 + OW0ScratchpadMem[6]*64;
	if (sign) temp*=-1;
	char x[16];
	for (int i = 0; i < 16; i++){
		if (i < 8){
			x[i] = OW0ScratchpadMem[i] + '0';
		}
		else{
			x[i] = OW1ScratchpadMem[i] + '0';
		}
	}
	printf("scratch %s scratch", x);
	return temp;
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
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  DWT_Init();
  delay_us(1);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	float temp = OWReadTemp();
	printf("hello %f\r\n", temp);
	HAL_Delay(1000);
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(OneWire_BB_GPIO_Port, OneWire_BB_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : OneWire_BB_Pin */
  GPIO_InitStruct.Pin = OneWire_BB_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(OneWire_BB_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
int __io_putchar(int ch){
	  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 1000);
	  return ch;
}
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

#ifdef  USE_FULL_ASSERT
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
