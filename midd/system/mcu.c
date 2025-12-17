#include "mcu.h"

#include "gpio.h"
#include "i2c.h"

extern void SystemClock_Config(void);

bool mcu_init(void)
{
    HAL_Init();

    SystemClock_Config();

    MX_GPIO_Init();
    MX_I2C1_Init();

    return true;
}