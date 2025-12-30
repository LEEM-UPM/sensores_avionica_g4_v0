// ----- LIBRARIES -----
#include "main.h"
#include "gpio.h"
#include <string.h>
#include "usart.h"


// ----- FUNCTION DECLARATION -----

extern void SystemClock_Config(void);

// ----- PREPROCESSED -----
char msg[] = "Hola desde el STM32\r\n";

// -----  MAIN CODE ----- 
int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART2_UART_Init();

  while (1)
  {
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
    HAL_GPIO_TogglePin(TEST_LED_GPIO_Port, TEST_LED_Pin);
    HAL_Delay(1000);
  }
}