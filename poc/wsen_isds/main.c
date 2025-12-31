// ----- INCLUDED LIBRARIES -----
#include "i2c.h"
#include <usart.h>
#include <gpio.h>
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>

  // LIBRARIES USED BY WSEN_ISDS
  #include "WeSensorsSDK.h"
  #include "WSEN_ISDS_2536030320001.h"
  #include "platform.h"
  #include "main.h"
 

// ----- PREPROCESSED -----

#define WSEN_IDS_NUM_MEA_FIFO 20

/* FIFO fill threshold - an event is generated when the FIFO buffer is
 * filled up to this amount (6 values per sample, 3 accelerations and 3
 * angular rates). */
#define FIFO_THRESH (WSEN_IDS_NUM_MEA_FIFO * 6)


// ----- VARIABLE DECLARATION -----

  uint16_t fillLevel;
  extern UART_HandleTypeDef huart2;

  // WSEN_ISDS sensor interface configuration
    static WE_sensorInterface_t isds = {
      .sensorType = WE_ISDS, 
      .interfaceType = WE_i2c,
      .options = {
          .i2c = {.address = ISDS_ADDRESS_I2C_1, 
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

  // Is set to true when an interrupt has been triggered 
    static bool interrupt0Triggered = false;
    static bool interrupt1Triggered = false;

  // Accelerometer/Gryoscope measurements  
    typedef struct 
    {
      int32_t xRate;
      int32_t yRate;
      int32_t zRate;
      int32_t xAcc;
      int32_t yAcc; 
      int32_t zAcc; 
    
    } ISDS_fifoMeasurement;

    ISDS_fifoMeasurement isds_buffer[WSEN_IDS_NUM_MEA_FIFO];

    uint16_t fifoDataRaw[FIFO_THRESH];

    uint32_t InitTime;
    uint32_t CurrentTime;

// ----- FUNCTION DECLARATION -----

bool mcu_init();
extern void SystemClock_Config(void);

bool sensors_init();
static bool isds_init(void);
bool isds_interrupt_handler();
bool isds_obtain_data();
bool isds_print_data();

// ----- MAIN CODE -----
int main(void) 
{
  mcu_init();
  sensors_init();
  InitTime = HAL_GetTick();

  while (1)
  {
    if (!isds_interrupt_handler())
    {
      // TODO: Warning message
      continue;
    }

  }
}

// ----- FUNCTION DEFINITIONS -----

bool mcu_init()
{
  if ( HAL_Init() != HAL_OK) {
    return false;
  }

  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  
  // Delay 10 seconds until I2C1_SCL jumper is disconected
  HAL_GPIO_WritePin(TEST_LED_GPIO_Port, TEST_LED_Pin, GPIO_PIN_SET);
  HAL_Delay(10000);
  HAL_GPIO_WritePin(TEST_LED_GPIO_Port, TEST_LED_Pin, GPIO_PIN_RESET);
  MX_I2C1_Init();

  return true;
}

bool sensors_init()
{
  // Initialization IMU WSEN_ISDS
  if ( !isds_init() )
  {
    return false;
  }

  return true;
}

/**
 * @brief Initializes the WSEN_ISDS sensor.
 */
static bool isds_init(void)
{
  // WAIT TILL ISDS SENSOR INTERFACE IS READY
    HAL_Delay(50);
    while (WE_SUCCESS != WE_isSensorInterfaceReady(&isds))
    {
      // TODO: IF TIME > 10 SEC, error_msg + return false
    }
    // debugPrintln("**** WE_isSensorInterfaceReady(): OK ****");

    HAL_Delay(5);

  // FIRST COMMUNICATION TEST

    uint8_t deviceIdValue = 0;
    if (WE_SUCCESS == ISDS_getDeviceID(&isds, &deviceIdValue))
    {
        if (deviceIdValue == ISDS_DEVICE_ID_VALUE) /* who am i ? - i am WSEN-ISDS! */
        {
            // TODO: PrintFunction --->  debugPrintln("**** ISDS_DEVICE_ID_VALUE: OK ****");
        }
        else
        {
            // TODO: PrintFunction --->   debugPrintln("**** ISDS_DEVICE_ID_VALUE: NOT OK ****");
            return false;
        }
    }
    else
    {
        // TODO: PrintFunction --->   debugPrintln("**** ISDS_getDeviceID(): NOT OK ****");
        return false;
    }

  // SOFT RESET OF THE SENSOR

    ISDS_softReset(&isds, ISDS_enable);
    ISDS_state_t swReset;
    do
    {
        ISDS_getSoftResetState(&isds, &swReset);
    } while (swReset);
    // TODO: PrintFunction ---> debugPrintln("**** ISDS reset complete ****");


  // PERFORM REBOOT (retrieve trimming parameters from nonvolatile memory)
    ISDS_reboot(&isds, ISDS_enable);
    HAL_Delay(15);
    // TODO: PrintFunction ---> debugPrintln("**** ISDS reboot complete ****");

  // ENABLE BLOCK DATA UPDATE (Cuando está habilitado, los registros de salida no se actualizan hasta que se hayan leído por completo (LSB + MSB))
    ISDS_enableBlockDataUpdate(&isds, ISDS_enable);

  // ENABLE ADDRESS AUTO INCREMENT
    ISDS_enableAutoIncrement(&isds, ISDS_enable);

  // SET ACCELEROMETER AND GYROSCOPE SAMPLING RATE TO 208 HZ ---> Frecuencia de muestreo del acelerometro y giroscopio.
    ISDS_setAccOutputDataRate(&isds, ISDS_accOdr208Hz);
    ISDS_setGyroOutputDataRate(&isds, ISDS_gyroOdr208Hz);

    // SET FIFO TO OPERATE AT 208 HZ (SAME AS ACCELEROMETER AND GYROSCOPE) ---> Frecuencia a la que los datos se cargan en la fifo.
    ISDS_setFifoOutputDataRate(&isds, ISDS_fifoOdr208Hz);

  // CONFIGURE FULL SCALE: ±16 G FOR ACCELEROMETER AND ±2000 DPS FOR GYROSCOPE
    ISDS_setAccFullScale(&isds, ISDS_accFullScaleSixteenG);
    ISDS_setGyroFullScale(&isds, ISDS_gyroFullScale2000dps);

  // SET FIFO FILL THRESHOLD ---> Establece el threshold al cual saltará la interrupcion
    ISDS_setFifoThreshold(&isds, FIFO_THRESH);

  // CONFIGURE INTERRUPTS AS ACTIVE HIGH ---> Active High: La línea de interrupción (INT) estará en nivel alto (con voltaje) cuando ocurra una interrupción.
    ISDS_setInterruptActiveLevel(&isds, ISDS_activeHigh);

  // CONFIGURE INTERRUPTS AS PUSH-PULL
    /* Push-Pull ----> El pin puede conducir corriente tanto hacia alto (Vcc) como hacia bajo (GND).
    // Open-Drain ---> El pin solo puede ir a bajo (GND) cuando se activa la interrupción. 
                       Cuando no está activo, el pin queda "flotando" (alta impedancia), como si estuviera desconectado
                       Se necesita una resistencia pull-up externa para que el pin vuelva a estado alto cuando no hay interrupción.
    */
    ISDS_setInterruptPinType(&isds, ISDS_pushPull);

  // DISABLE LATCHED MODE (INTERRUPTS RESET AUTOMATICALLY) 
    /*  Latched interrupt ---> Latched interrupt: El pin de interrupción (INT) se mantiene activado hasta que el microcontrolador la "borra" explícitamente.
        Not Latched       ---> El pin INT solo se activa brevemente cuando ocurre el evento, y se desactiva automáticamente después, sin intervención del microcontrolador.
    */
    ISDS_enableLatchedInterrupt(&isds, ISDS_disable);

  // ENABLE INTERRUPTS (2º argument enabless/disabless interrupts ISDS_enable/ISDS_disable) ---> Capacidad del sensor para generar interrupciones.
    ISDS_enableInterrupts(&isds, ISDS_enable);

    /* Note that, depending on the configuration of the filtering chain of the
   * accelerometer and the gyroscope, it might be necessary to discard the
   * first samples after changing ODRs or when changing the power mode.
   * Consult the user manual for details on the number of samples to be discarded. */

    /* Interrupt when FIFO buffer is filled up to threshold on INT0 */
    ISDS_enableFifoThresholdINT0(&isds, ISDS_enable);

    /* Interrupts for FIFO buffer full and overrun events on INT1 */
    ISDS_enableFifoOverrunINT1(&isds, ISDS_enable);
    ISDS_enableFifoFullINT1(&isds, ISDS_enable);

    /* No decimation of accelerations and angular rates
   * (i.e. use the FIFO sample rate set in init function) */
    ISDS_setFifoAccDecimation(&isds, ISDS_fifoDecimationDisabled);
    ISDS_setFifoGyroDecimation(&isds, ISDS_fifoDecimationDisabled);

    /* Enable continuous mode */
    ISDS_setFifoMode(&isds, ISDS_continuousMode);

    return true;
}


/**
 * @brief isds_interrupt_handler function: Samples are collected continuously.
 * When the FIFO buffer is filled up to FIFO_THRESH, the collected data is read
 * thus making space for new samples.
 */

bool isds_interrupt_handler()
{
  if (interrupt0Triggered == true)
  {
      // FIFO buffer is filled up to threshold 
      interrupt0Triggered = false;

      if (isds_obtain_data())
      {
        isds_print_data();
      }
      else
      {
        printf("[Error]: Failed obtaining IMU WSEN-ISDS fifo data");
      }
  }

  if (interrupt1Triggered == true)
  {
      // ¡¡¡FIFO BUFFER FULL OR OVERRUN!!! - this shouldn't happen, if samples are read fast enough 

      interrupt1Triggered = false;
      ISDS_fifoStatus2_t status;
      uint16_t fillLevel, fifoPattern;
      ISDS_getFifoStatus(&isds, &status, &fillLevel, &fifoPattern);

      if (status.fifoOverrunState)
      {
        printf("[Error]: WSEN ISDS FIFO buffer overrun");
      }
      else if (status.fifoFullSmartState)
      {
          printf("[Error]: FIFO buffer is full" );
      }

      /* Restart data collection by first setting bypass mode and then re-enabling FIFO mode */
      ISDS_setFifoMode(&isds, ISDS_bypassMode);
      ISDS_setFifoMode(&isds, ISDS_continuousMode);
  }

  return true;
}

bool isds_obtain_data()
{
      // Get FIFO status
      ISDS_fifoStatus2_t status;
      uint16_t fillLevel, fifoPattern;
      ISDS_getFifoStatus(&isds, &status, &fillLevel, &fifoPattern);

      /* Note that we are always reading a multiple of the number of variables being
      * captured and the collection rate is identical for all data sets -
      * therefore fifoPattern should always be zero and the order of
      * the retrieved values should always be xRate,yRate,zRate,xAcc,yAcc,zAcc,...
      * Depending on the timing and the way the FIFO is read, fifoPattern might
      * be non-zero - in that case, we must use fifoPattern to determine the type
      * of samples read from the FIFO. In this example, we skip the first samples
      * if necessary, so that the first used sample is always xRate. */

      uint16_t patternOffset = (fifoPattern == 0) ? 0 : (6 - fifoPattern);

      /* Retrieve FIFO_THRESH samples from sensor */
      CurrentTime = HAL_GetTick() - InitTime; 
      if (WE_SUCCESS == ISDS_getFifoData(&isds, FIFO_THRESH, fifoDataRaw))
      {
        uint16_t numSamples = (FIFO_THRESH - patternOffset) / 6;

        for (uint16_t i = 0; i < numSamples; i++)
        {
            uint16_t offset = i * 6 + patternOffset;

            isds_buffer[i].xRate  = ISDS_convertAngularRateFs2000dps_int(*(int16_t*)&fifoDataRaw[offset]);
            isds_buffer[i].yRate  = ISDS_convertAngularRateFs2000dps_int(*(int16_t*)&fifoDataRaw[offset + 1]);   
            isds_buffer[i].zRate  = ISDS_convertAngularRateFs2000dps_int(*(int16_t*)&fifoDataRaw[offset + 2]);   
            isds_buffer[i].xAcc   = ISDS_convertAccelerationFs16g_int(*(int16_t*)&fifoDataRaw[offset + 3]);
            isds_buffer[i].yAcc   = ISDS_convertAccelerationFs16g_int(*(int16_t*)&fifoDataRaw[offset + 4]);
            isds_buffer[i].zAcc   = ISDS_convertAccelerationFs16g_int(*(int16_t*)&fifoDataRaw[offset + 5]);
        }

        return true;
      }
      else
      {
          return false;
      }
}

// Function used by printf
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

bool isds_print_data()
{
  printf("%8" PRId32 " %8" PRId32 " %8" PRId32 " %8" PRId32 " %8" PRId32 " %8" PRId32 " %8" PRId32 "\r\n",
        isds_buffer[0].xRate,
        isds_buffer[0].yRate,
        isds_buffer[0].zRate,
        isds_buffer[0].xAcc,
        isds_buffer[0].yAcc,
        isds_buffer[0].zAcc,
        CurrentTime);

  return true;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  /* Interrupt source depends on example mode. */

  if (GPIO_Pin == IMU1_INT0_Pin)
  {
    /* Trigger event handling in main function. */
    interrupt0Triggered = true;
  }
  else if (GPIO_Pin == IMU1_INT1_Pin)
  {
    /* Trigger event handling in main function. */
    interrupt1Triggered = true;
  }
}