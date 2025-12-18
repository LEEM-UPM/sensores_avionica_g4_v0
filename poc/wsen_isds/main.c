// ----- INCLUDED LIBRARIES -----
#include "i2c.h"
// #include "usart.h"
#include "gpio.h"
#include <stdbool.h>

  // LIBRARIES USED BY WSEN_ISDS
  #include "WeSensorsSDK.h"
  #include "WSEN_ISDS_2536030320001.h"
  #include "platform.h"
  #include "stm32g4xx_hal.h"
  #include <stdio.h>

// ----- PREPROCESSED -----

#define WSEN_IDS_NUM_MEA_FIFO 20

/* FIFO fill threshold - an event is generated when the FIFO buffer is
 * filled up to this amount (6 values per sample, 3 accelerations and 3
 * angular rates). */
#define FIFO_THRESH (WSEN_IDS_NUM_MEA_FIFO * 6)


// ----- VARIABLE DECLARATION -----

  // WSEN_ISDS sensor acceleration buffer (obtained from FIFO data)
    double acc_buffer[WSEN_IDS_NUM_MEA_FIFO];

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
    int32_t xRateAvg = 0;
    int32_t yRateAvg = 0;
    int32_t zRateAvg = 0;
    int32_t xAccAvg = 0;
    int32_t yAccAvg = 0;
    int32_t zAccAvg = 0;
    uint32_t nextPrintTime = 0;

// ----- FUNCTION DECLARATION -----

  extern void SystemClock_Config(void);
  bool obtain_accelerations();
  bool print_accelerations();
    
  bool hw_init();
  static bool isds_init_leem(void);
    
  // void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin);
  void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

// ----- MAIN CODE -----
int main(void) 
{

  hw_init();

  while (1)
  {
    if (!obtain_accelerations())
    {
      // TODO: Warning message
      continue;
    }

    // if (!print_time())
    // {
    //   continue;
    // }

    // if (!print_accelerations())
    // {
    //   continue;
    // }

    // if (!print_time())
    // {
    //   continue;
    // }
  }
}

// ----- FUNCTION DEFINITIONS -----

// bool print_time()
// {
//   // TODO: Imprimir por terminal el tiempo de cada bucle.

//   return true;
// }

bool hw_init()
{
  if ( HAL_Init() != HAL_OK) {
    return false;
  }

  SystemClock_Config();
  MX_GPIO_Init();

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
  HAL_Delay(10000);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);   
  

  // Initialize I2C_1
     MX_I2C1_Init(); 
  // Initialization WSEN_ISDS
  if ( !isds_init_leem() )
  {
    return false;
  }

  return true;
}

/**
 * @brief Initializes the WSEN_ISDS sensor.
 */
static bool isds_init_leem(void)
{
  // INITIALIZE SENSOR INTERFACE (I2C WITH ISDS ADDRESS, BURST MODE ACTIVATED)

    // ISDS_getDefaultInterface(&isds);
    // isds.interfaceType = WE_i2c;
    // isds.options.i2c.burstMode = 1;
    // isds.options.i2c.address = ISDS_ADDRESS_I2C_1;
    // #warning "Please use correct i2c address here"

    // isds.handle = &hi2c1;

    /* Wait for boot */
    // HAL_Delay(50);

  // WAIT TILL ISDS SENSOR INTERFACE IS READY
    while (WE_SUCCESS != WE_isSensorInterfaceReady(&isds))
    {
      // TODO: IF TIME > 10 SEC, error_msg + return false
    }
    // debugPrintln("**** WE_isSensorInterfaceReady(): OK ****");

    // HAL_Delay(5);

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
    // HAL_Delay(15);
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

    return true;
}

/**
 * @brief obtain_accelerations function: Samples are collected continuously.
 * When the FIFO buffer is filled up to FIFO_THRESH, the collected data is read
 * thus making space for new samples.
 */

bool obtain_accelerations()
{
  if (interrupt0Triggered == true)
  {
      // FIFO buffer is filled up to threshold 
      interrupt0Triggered = false;

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
        // debugPrint(".");

      }
      else
      {
          // debugPrintln("**** ISDS_getFifoData() failed");
      }
      // debugPrint(".");
  }

  if (interrupt1Triggered == true)
  {
      // ¡¡¡FIFO BUFFER FULL OR OVERRUN!!! - this shouldn't happen, if samples are read fast enough 

      interrupt1Triggered = false;
      ISDS_fifoStatus2_t status;
      uint16_t fillLevel, fifoPattern;
      ISDS_getFifoStatus(&isds, &status, &fillLevel, &fifoPattern);
      // char buffer[5];
      // sprintf(buffer, "%d", fillLevel);
      if (status.fifoOverrunState)
      {
          // debugPrintln("FIFO buffer overrun");
      }
      else if (status.fifoFullSmartState)
      {
          // debugPrint("FIFO buffer is full (contains ");
          // debugPrint(buffer);
          // debugPrintln(" samples)");
      }

      /* Restart data collection by first setting bypass mode and then re-enabling FIFO mode */
      ISDS_setFifoMode(&isds, ISDS_bypassMode);
      ISDS_setFifoMode(&isds, ISDS_continuousMode);
  }

  // /* Print last accelerations and angular rates (averaged) every second. */
  // uint32_t currentTime = HAL_GetTick();
  // if (currentTime >= nextPrintTime)
  // {
  //     nextPrintTime = currentTime + 1000;
  //     debugPrintln("");
  //     debugPrintAcceleration_int("X", xAccAvg);
  //     debugPrintAcceleration_int("Y", yAccAvg);
  //     debugPrintAcceleration_int("Z", zAccAvg);
  //     debugPrintAngularRate_int("X", xRateAvg);
  //     debugPrintAngularRate_int("Y", yRateAvg);
  //     debugPrintAngularRate_int("Z", zRateAvg);
  // }

  return true;
}

// bool print_accelerations()
// {
//   // TODO: Bucle por todo el acc_buffer que imprima cada una de las aceleraciones.

//   return true;
// }


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  /* Interrupt source depends on example mode. */

  if (GPIO_Pin == GPIO_PIN_14)
  {
    /* Trigger event handling in main function. */
    interrupt0Triggered = true;
  }
  else if (GPIO_Pin == GPIO_PIN_15)
  {
    /* Trigger event handling in main function. */
    interrupt1Triggered = true;
  }
}


// void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
// {
//   /* Interrupt source depends on example mode. */

//   if (GPIO_Pin == GPIO_PIN_14)
//   {
//     /* Trigger event handling in main function. */
//     interrupt0Triggered = true;
//   }
//   else if (GPIO_Pin == GPIO_PIN_15)
//   {
//     /* Trigger event handling in main function. */
//     interrupt1Triggered = true;
//   }
// }