// ----- LIBRARIES -----
#include "main.h"
#include "gpio.h"
// #include "stm32g4xx_hal_gpio.h"

// ----- FUNCTION DECLARATION -----

  extern void SystemClock_Config(void);

// ----- PREPROCESSED -----


// MAIN CODE
int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  HAL_Delay(10000);

  while (1)
  {
    HAL_GPIO_TogglePin(TEST_LED_GPIO_Port, TEST_LED_Pin);
    HAL_Delay(1000);
  }
}