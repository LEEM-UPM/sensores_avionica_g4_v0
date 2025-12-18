// ----- LIBRARIES -----
#include "main.h"
#include "gpio.h"
#include "stm32g4xx_hal_gpio.h"

// ----- FUNCTION DECLARATION -----

  extern void SystemClock_Config(void);

// ----- PREPROCESSED -----

#define  TEST_LED_PERIPHERIAL GPIOB
#define  TEST_LED_PIN_NUMBER GPIO_PIN_9

// MAIN CODE
int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();

  while (1)
  {
    HAL_GPIO_TogglePin(TEST_LED_PERIPHERIAL, TEST_LED_PIN_NUMBER);
    HAL_Delay(1000);
  }
}