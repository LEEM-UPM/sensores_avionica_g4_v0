#include "pads.h"

#include "i2c.h"
#include "gpio.h"

#include "platform.h"

WE_sensorInterface_t pads1 = {
    .sensorType = WE_PADS,
    .interfaceType = WE_i2c,
    .options = {
        .i2c = {
            .address = PADS_ADDRESS_I2C_0,
            .burstMode = 1,
            .slaveTransmitterMode = 0,
            .useRegAddrMsbForMultiBytesRead = 0,
            .reserved = 0
        },
        .readTimeout = 1000,
        .writeTimeout = 1000
    },
    .handle = &hi2c1
};

static volatile bool pads1_int_triggered = false;

bool pads_init(WE_sensorInterface_t *pads)
{
    /* Wait for boot */
    HAL_Delay(50);
    while (WE_isSensorInterfaceReady(pads) != WE_SUCCESS)
    {
    }

    HAL_Delay(5);

    /* First communication test */
    uint8_t device_id_value = 0;
    if (PADS_getDeviceID(pads, &device_id_value) != WE_SUCCESS)
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
    PADS_softReset(pads, PADS_enable);
    PADS_state_t sw_reset;
    do
    {
        PADS_getSoftResetState(pads, &sw_reset);
    } while (sw_reset);

    /* Enable low-noise configuration */
    PADS_setPowerMode(pads, PADS_lowNoise);

    /* Automatic increment register address */
    PADS_enableAutoIncrement(pads, PADS_enable);

    /* Enable additional low pass filter */
    PADS_enableLowPassFilter(pads, PADS_enable);

    /* Set filter bandwidth of ODR/20 */
    PADS_setLowPassFilterConfig(pads, PADS_lpFilterBW2);

    /* Enable block data update */
    PADS_enableBlockDataUpdate(pads, PADS_enable);

    /* Interrupts are active high */
    PADS_setInterruptActiveLevel(pads, PADS_activeHigh);

    /* Interrupts are push-pull */
    PADS_setInterruptPinType(pads, PADS_pushPull);

    return true;
}

bool pads_start(WE_sensorInterface_t *pads, int fifo_threshold)
{
    /* Enable FIFO continuous mode */
    PADS_setFifoMode(pads, PADS_continuousMode);

    /* Set FIFO fill threshold */
    PADS_setFifoThreshold(pads, fifo_threshold);

    /* Interrupt for FIFO buffer fill threshold reached on INT1 */
    PADS_enableFifoThresholdInterrupt(pads, PADS_enable);

    /* Activate FIFO threshold interrupt in event control register */
    PADS_setInterruptEventControl(pads, PADS_dataReady);

    /* Enable continuous operation with an update rate of 200 Hz */
    PADS_setOutputDataRate(pads, PADS_outputDataRate200Hz);

    return true;
}

bool pads_read_fifo(WE_sensorInterface_t *pads, int32_t *buffer, size_t len)
{
    return PADS_getFifoPressure_int(pads, len, buffer) == WE_SUCCESS;
}

void pads_int_handler(WE_sensorInterface_t *pads)
{
    if (pads == &pads1)
    {
        pads1_int_triggered = true;
    }
}

bool pads_event_pending(WE_sensorInterface_t *pads)
{
    if (pads == &pads1)
    {
        return pads1_int_triggered;
    }

    return false;
}

void pads_clear_event(WE_sensorInterface_t *pads)
{
    if (pads == &pads1)
    {
        pads1_int_triggered = false;
    }
}