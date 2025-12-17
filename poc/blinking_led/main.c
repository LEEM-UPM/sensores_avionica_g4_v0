// ----- LIBRARIES -----
#include "main.h"
#include "gpio.h"

// ----- FUNCTION DECLARATION -----

  extern void SystemClock_Config(void);

// MAIN CODE
int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();

  while (1)
  {
  //Enciende el LED
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);    
    HAL_Delay(2000);
    
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
    
    HAL_Delay(2000);
  }
}