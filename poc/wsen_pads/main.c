// Example demonstrating usage of FIFO buffer in continuous mode (threshold interrupt)
#include <stdbool.h>

#include "i2c.h"
#include "gpio.h"

#include "WSEN_PADS_2511020213301.h"
#include "platform.h"

WE_sensorInterface_t pads = {
    .sensorType = WE_PADS,
    .interfaceType = WE_i2c,
    .options = {
        .i2c = {
            .address = PADS_ADDRESS_I2C_1, 
            .burstMode = 1, 
            .slaveTransmitterMode = 0, 
            .useRegAddrMsbForMultiBytesRead = 0, 
            .reserved = 0
        },
        .readTimeout = 1000,
        .writeTimeout = 1000
    },
    .handle = &hi2c2
};

static volatile bool interrupt_triggered = false;

static bool mcu_init(void);
static bool pads_init(void);
static bool pads_start(int fifo_threshold);

extern void SystemClock_Config(void);

int main()
{
    mcu_init();
    pads_init();

    // FIFO THRESHOLD 
    const int fifo_threshold = 25;
    // FIFO GROUP SIZE (number of measurements of each group of the buffer)
    const int buffer_group_size = 5;
    // FIFO TOTAL GRUPS (number of groups on the buffer)
    const int buffer_total_groups = fifo_threshold/buffer_group_size;

    pads_start(fifo_threshold);
    int32_t pressure_buffer[PADS_FIFO_BUFFER_SIZE] = {0};
    PADS_state_t PADSstate;
    uint8_t fifoLevel;
    PADS_state_t fifoOvr;

    while (1)
    {
        PADS_getPressureOverrunStatus(&pads, &PADSstate);
        PADS_getFifoFillLevel(&pads, &fifoLevel);
        PADS_getFifoOverrunState(&pads,&fifoOvr);

        if (fifoLevel <= fifo_threshold)
        {
            if (interrupt_triggered == PADS_enable)
            {
                interrupt_triggered = false;

                /* Get pressure measurements in one go. */
                /* Note: Samples must be read faster than the ODR. */
                
                PADS_getFifoPressure_int(&pads, fifo_threshold, pressure_buffer);

                /* Compute average of captured pressurevalues for each group of the fifo */
                uint32_t pressure_avg[5] = {0};
                for (uint8_t j = 0; j < buffer_total_groups; j++)
                {
                    uint32_t sum = 0;
                
                    for (uint8_t i = 0; i < buffer_group_size; i++)
                    {
                        sum += pressure_buffer[j*5 + i];
                    }
                
                    pressure_avg[j] = sum / buffer_group_size;
                }
            }
        }
        else
        {
            PADS_setFifoMode(&pads, PADS_bypassMode);
            PADS_setFifoMode(&pads, PADS_continuousMode);
        } 
    }
}

static bool pads_init(void)
{
    /* Wait for boot */
    HAL_Delay(50);
    while (WE_isSensorInterfaceReady(&pads) != WE_SUCCESS)
    {
    }

    HAL_Delay(5);

    /* First communication test */
    uint8_t device_id_value = 0;
    if (PADS_getDeviceID(&pads, &device_id_value) != WE_SUCCESS)
    {
        return false;
    }
    else
    {
        if (device_id_value != PADS_DEVICE_ID_VALUE) /* who am i ? - i am WSEN-PADS! */
        {
            return false;
        }
    }

    /* Perform soft reset of the sensor */
    PADS_softReset(&pads, PADS_enable);
    PADS_state_t sw_reset;
    do
    {
        PADS_getSoftResetState(&pads, &sw_reset);
    } while (sw_reset);

    /* Enable low-noise configuration */
    PADS_setPowerMode(&pads, PADS_lowNoise);

    /* Automatic increment register address */
    PADS_enableAutoIncrement(&pads, PADS_enable);

    /* Enable additional low pass filter */
    PADS_enableLowPassFilter(&pads, PADS_enable);

    /* Set filter bandwidth of ODR/20 */
    PADS_setLowPassFilterConfig(&pads, PADS_lpFilterBW2);

    /* Enable block data update */
    PADS_enableBlockDataUpdate(&pads, PADS_enable);

    /* Interrupts are active high */
    PADS_setInterruptActiveLevel(&pads, PADS_activeHigh);

    /* Interrupts are push-pull */
    PADS_setInterruptPinType(&pads, PADS_pushPull);

    return true;
}

static bool pads_start(int fifo_threshold)
{
    /* Enable FIFO continuous mode */
    PADS_setFifoMode(&pads, PADS_continuousMode);

    /* Set FIFO fill threshold */
    PADS_setFifoThreshold(&pads, fifo_threshold);

    /* Interrupt for FIFO buffer fill threshold reached on INT1 */
    PADS_enableFifoThresholdInterrupt(&pads, PADS_enable);

    /* Activate FIFO threshold interrupt in event control register */
    PADS_setInterruptEventControl(&pads, PADS_dataReady);

    /* Enable continuous operation with an update rate of 200 Hz */
    PADS_setOutputDataRate(&pads, PADS_outputDataRate100Hz);

    return true;
}

static bool mcu_init(void)
{
  HAL_Init();

  SystemClock_Config();

  MX_GPIO_Init();
  MX_I2C2_Init();

  return true;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == PADS2_INT1_Pin)
  {
    interrupt_triggered = true;
  }
}