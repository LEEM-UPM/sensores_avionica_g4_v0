/******************************************************************************
 * WSEN-ISDS FIFO threshold interrupt - Proof of Concept
 *
 * Concept:
 *  - Sensor accumulates samples in FIFO
 *  - FIFO threshold interrupt triggers data read
 *  - Firmware reacts to event (event-driven)
 ******************************************************************************/

/* === Includes ============================================================ */
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>

#include <gpio.h>
#include "i2c.h"
#include <usart.h>

#include "WeSensorsSDK.h"
#include "WSEN_ISDS_2536030320001.h"
#include "platform.h"
#include "main.h"
 
/* === Configuration ======================================================= */
#define DEBUG_MODE true

#define WSEN_IDS_NUM_MEA_FIFO 20
#define FIFO_THRESH (WSEN_IDS_NUM_MEA_FIFO * 6)

/* === Sensor interface ==================================================== */
WE_sensorInterface_t isds = {
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

/* === Globals ============================================================= */

/* ISDS fifo acceleration type */
typedef struct 
{
  int32_t xAcc;
  int32_t yAcc; 
  int32_t zAcc; 
} ISDS_fifoAcceleration;

/* ISDS fifo angular rate type */
typedef struct 
{
  int32_t xRate;
  int32_t yRate;
  int32_t zRate;
} ISDS_fifoAngularRate;

/* Flag set by ISR when FIFO threshold interrupt occurs */
static volatile bool fifo_event_0 = false;
/* Flag set by ISR when FIFO buffer is full or overrun */
static volatile bool fifo_event_1 = false;

/* ISDS fifo raw buffer */
static uint16_t fifoDataRaw[FIFO_THRESH];

/* ISDS fifo measurements */
static ISDS_fifoAcceleration isds_accelerations[WSEN_IDS_NUM_MEA_FIFO];
static ISDS_fifoAngularRate  isds_angular_rate[WSEN_IDS_NUM_MEA_FIFO];

/* ISDS fifo state variables */
static uint16_t fillLevel;

/* UART handle variables */
extern UART_HandleTypeDef huart2;

/* Time reference */
static uint32_t start_time_ms;

/* === Forward declarations ============================================== */
extern void SystemClock_Config(void);

static bool mcu_init(void);
static bool isds_init(void);
static bool isds_start(void);

static bool isds_handle_fifo0_event(void);
static bool isds_handle_fifo1_event(void);
static bool isds_obtain_data(void);
static bool isds_print_data(void);

/* === Main ================================================================ */
int main(void) 
{
  mcu_init();
  isds_init();
  isds_start();

  start_time_ms = HAL_GetTick();

  while (1)
  {
    if (fifo_event_0) {
      fifo_event_0 = false;
      isds_handle_fifo0_event();
    }

    if (fifo_event_1) {
      fifo_event_1 = false;
      isds_handle_fifo1_event();
    }

  }
}

bool mcu_init() {
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_USART2_UART_Init();
  
  /* Delay 10 seconds until I2C1_SCL jumper is disconected manually (to activate the I2C1_SCL pull up) */
  HAL_GPIO_WritePin(TEST_LED_GPIO_Port, TEST_LED_Pin, GPIO_PIN_SET);
  HAL_Delay(10000);
  HAL_GPIO_WritePin(TEST_LED_GPIO_Port, TEST_LED_Pin, GPIO_PIN_RESET);
  MX_I2C1_Init();

  return true;
}

static bool isds_init(void) {
  HAL_Delay(50);
  
  while (WE_SUCCESS != WE_isSensorInterfaceReady(&isds)){
  }

  HAL_Delay(5);

  uint8_t device_id = 0;
  if (WE_SUCCESS != ISDS_getDeviceID(&isds, &device_id)) {
    return false;
  }

  if (device_id != ISDS_DEVICE_ID_VALUE) {
      return false;
  }

  /* Soft reset */
  ISDS_softReset(&isds, ISDS_enable);
  ISDS_state_t reset_state;
  do
  {
      ISDS_getSoftResetState(&isds, &reset_state);
  } while (reset_state);

  /* Sensor reboot */
    ISDS_reboot(&isds, ISDS_enable);
    HAL_Delay(15);

  /* Sensor configuration */
    ISDS_enableBlockDataUpdate(&isds, ISDS_enable);
    ISDS_enableAutoIncrement(&isds, ISDS_enable);
    ISDS_setAccOutputDataRate(&isds, ISDS_accOdr208Hz);
    ISDS_setGyroOutputDataRate(&isds, ISDS_gyroOdr208Hz);
    ISDS_setFifoOutputDataRate(&isds, ISDS_fifoOdr208Hz);
    ISDS_setAccFullScale(&isds, ISDS_accFullScaleSixteenG);
    ISDS_setGyroFullScale(&isds, ISDS_gyroFullScale2000dps);
    ISDS_setFifoThreshold(&isds, FIFO_THRESH);

  /* Interrupt configuration */    
    ISDS_setInterruptActiveLevel(&isds, ISDS_activeHigh);
    ISDS_setInterruptPinType(&isds, ISDS_pushPull);

    return true;
}

static bool isds_start(void) {

  /* Disable latched interrupt (Non-Latched: interrupt is asserted briefly and clears automatically) */
  ISDS_enableLatchedInterrupt(&isds, ISDS_disable);

  /* Inerrupt Configuration */
  ISDS_enableInterrupts(&isds, ISDS_enable);
  ISDS_enableFifoThresholdINT0(&isds, ISDS_enable); 
  ISDS_enableFifoOverrunINT1(&isds, ISDS_enable);
  ISDS_enableFifoFullINT1(&isds, ISDS_enable);

  /* Disable FIFO decimation.
     Decimation reduces the effective output rate by storing only 1 sample
     every N sensor samples (output frequency = ODR / decimation factor). */
  ISDS_setFifoAccDecimation(&isds, ISDS_fifoDecimationDisabled);
  ISDS_setFifoGyroDecimation(&isds, ISDS_fifoDecimationDisabled);

  /* Enable coninious mode */
  ISDS_setFifoMode(&isds, ISDS_continuousMode);

  return true;
}

static bool isds_handle_fifo0_event() {

  isds_obtain_data();
  isds_print_data();

  return true;
}

static bool isds_handle_fifo1_event() {
  ISDS_fifoStatus2_t status;
  uint16_t fifoPattern;
  ISDS_getFifoStatus(&isds, &status, &fillLevel, &fifoPattern);

  if (status.fifoOverrunState)
  {
    printf("[Error]: WSEN ISDS FIFO overrun");
  }
  else if (status.fifoFullSmartState)
  {
    printf("[Error]: FIFO buffer will be fool at the next measurement" );
  }

  /* Restart data collection by first setting bypass mode and then re-enabling FIFO mode */
  ISDS_setFifoMode(&isds, ISDS_bypassMode);
  ISDS_setFifoMode(&isds, ISDS_continuousMode);

  return true;
}

bool isds_obtain_data() {
  // Get FIFO status
  ISDS_fifoStatus2_t status;
  uint16_t fifoPattern;
  ISDS_getFifoStatus(&isds, &status, &fillLevel, &fifoPattern);

  uint16_t patternOffset = (fifoPattern == 0) ? 0 : (6 - fifoPattern);

  /* Retrieve FIFO_THRESH samples from sensor */
  if (WE_SUCCESS == ISDS_getFifoData(&isds, FIFO_THRESH, fifoDataRaw)) {

    uint16_t numSamples = (FIFO_THRESH - patternOffset) / 6;
    for (uint16_t i = 0; i < numSamples; i++) {
      uint16_t offset = i * 6 + patternOffset;

      isds_angular_rate[i].xRate  = ISDS_convertAngularRateFs2000dps_int(*(int16_t*)&fifoDataRaw[offset]);
      isds_angular_rate[i].yRate  = ISDS_convertAngularRateFs2000dps_int(*(int16_t*)&fifoDataRaw[offset + 1]);   
      isds_angular_rate[i].zRate  = ISDS_convertAngularRateFs2000dps_int(*(int16_t*)&fifoDataRaw[offset + 2]);   
      isds_accelerations[i].xAcc   = ISDS_convertAccelerationFs16g_int(*(int16_t*)&fifoDataRaw[offset + 3]);
      isds_accelerations[i].yAcc   = ISDS_convertAccelerationFs16g_int(*(int16_t*)&fifoDataRaw[offset + 4]);
      isds_accelerations[i].zAcc   = ISDS_convertAccelerationFs16g_int(*(int16_t*)&fifoDataRaw[offset + 5]);
    }
    return true;

  }
  return false;
}

// Function used by printf
int __io_putchar(int ch) {
#if DEBUG_MODE
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);  
#endif
  return ch;
}

bool isds_print_data() {
  uint32_t elapsed_ms = HAL_GetTick() - start_time_ms;

  /* Header */
  printf("%-14s %-14s %-14s %-14s %-14s %-14s %-14s\r\n",
         "Rate_x (dps)","Rate_y (dps)","Rate_z (dps)",
         "Acc_x (mg)","Acc_y (mg)","Acc_z (mg)","time (ms)");

  /* Measured data */
  printf("%-14" PRId32 " %-14" PRId32 " %-14" PRId32" %-14" PRId32 " %-14" PRId32 " %-14" PRId32" %-14" PRId32 "\r\n",
         isds_angular_rate[0].xRate,isds_angular_rate[0].yRate,isds_angular_rate[0].zRate,
         isds_accelerations[0].xAcc,isds_accelerations[0].yAcc,isds_accelerations[0].zAcc,elapsed_ms);

  return true;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  if (GPIO_Pin == IMU1_INT0_Pin) {
    fifo_event_0 = true;
  }
  else if (GPIO_Pin == IMU1_INT1_Pin) {
    fifo_event_1 = true;
  }
}