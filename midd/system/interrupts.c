#include "pads.h"
#include "gpio.h"

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0)
    {
        pads_int_handler(&pads1);
    }
}