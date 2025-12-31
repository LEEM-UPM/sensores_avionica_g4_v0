// Example demonstrating usage of FIFO buffer in continuous mode (threshold interrupt)

// ----- LIBRARIES -----

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "i2c.h"
#include "gpio.h"
#include "usart.h"

#include "WSEN_PADS_2511020213301.h"
#include "platform.h"

// ----- CONFIGURATION MACROS -----

#define DEBUG_MODE true

#define FIFO_THRESHOLD_VALUE 25
#define BUFFER_GROUP_SIZE 5
#define BUFFER_TOTAL_GROUPS (FIFO_THRESHOLD_VALUE/BUFFER_GROUP_SIZE)


// ----- TYPES DEFINITION -----

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

// ----- VARIABLE DECLARATION -----ç

static volatile bool interrupt_triggered = false;

int32_t pressure_buffer[PADS_FIFO_BUFFER_SIZE] = {0};
int16_t temperature_buffer[PADS_FIFO_BUFFER_SIZE] = {0};
uint32_t pressure_avg[BUFFER_GROUP_SIZE] = {0};
uint16_t temperature_avg[BUFFER_GROUP_SIZE] = {0};

char msg[150];
uint32_t init_time;
uint32_t current_time;
int fifo_threshold = FIFO_THRESHOLD_VALUE;
uint8_t fifoLevel;

// ----- FUNCTION DECLARATION -----

extern void SystemClock_Config(void);

static bool mcu_init(void);
static bool pads_init(void);
static bool pads_start(int fifo_threshold);
void pads_interrupt_handler();
bool pads_compute_average_fifo_data();
void pads_print_data();


// ----- MAIN CODE ----- 
int main()
{
    mcu_init();
    pads_init();
    pads_start(fifo_threshold);

    init_time = HAL_GetTick();
    current_time = HAL_GetTick() - init_time;

    while (1)
    {
        pads_interrupt_handler();
    }
}

void pads_interrupt_handler()
{
    PADS_getFifoFillLevel(&pads, &fifoLevel);
    if (fifoLevel <= fifo_threshold)
    {
        if (interrupt_triggered == true)
        {
            interrupt_triggered = false;

            /* Get pressure measurements in one go. */
            /* Note: Samples must be read faster than the ODR. */

            if (!pads_compute_average_fifo_data())
            {
                printf("[Error]: Failed obtaining pads fifo values \r\n");
            }
            else
            {
                pads_print_data();
            }
        }
    }
    else
    {
        // Fifo threshold has been overrun (not nominal operation) 
        // Principal cause: The fifo is not being read at time because other interrupts
        // with highter priority are blocking it.
        printf("[Error]: Fifo threshold has been overrun \r\n");
        printf("[Info]: Cleaning all pads fifo values \r\n");
        PADS_setFifoMode(&pads, PADS_bypassMode);
        PADS_setFifoMode(&pads, PADS_continuousMode);
    }
}

bool pads_compute_average_fifo_data()
{
    current_time = HAL_GetTick() - init_time;            

    if (PADS_getFifoValues_int(&pads, fifo_threshold, pressure_buffer, temperature_buffer) == WE_FAIL)
    {
        return false;
    }

    /* Compute average of captured pressurevalues for each group of the fifo */


    for (uint8_t j = 0; j < BUFFER_TOTAL_GROUPS; j++)
    {
        uint32_t sum = 0;
        uint32_t sum2 = 0;
    
        for (uint8_t i = 0; i < BUFFER_GROUP_SIZE; i++)
        {
            sum += pressure_buffer[j*BUFFER_GROUP_SIZE + i];
            sum2 += temperature_buffer[j*BUFFER_GROUP_SIZE + i];
        }
    
        pressure_avg[j] = sum / BUFFER_GROUP_SIZE;
        temperature_avg[j] = sum2 / BUFFER_GROUP_SIZE;
    }

    return true;
}


void pads_print_data()
{
    printf("%8" PRIu32 " %8" PRIu16 " %8" PRIu32 "\r\n",
           pressure_avg[0],
           temperature_avg[0],
           current_time);
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

    /* Enable continuous operation with an update rate of 100 Hz */
    PADS_setOutputDataRate(&pads, PADS_outputDataRate100Hz);

    return true;
}

static bool mcu_init(void)
{
  HAL_Init();

  SystemClock_Config();

  MX_GPIO_Init();
  MX_I2C2_Init();
  MX_USART2_UART_Init();

  return true;
}

// Function used by printf
int __io_putchar(int ch)
{
    #if DEBUG_MODE
        HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
        return ch;
    #endif

    return ch;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == PADS2_INT1_Pin)
  {
    interrupt_triggered = true;
  }
}