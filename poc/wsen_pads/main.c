/******************************************************************************
 * WSEN-PADS FIFO threshold interrupt - Proof of Concept
 *
 * Concept:
 *  - Sensor accumulates samples in FIFO
 *  - FIFO threshold interrupt triggers data read
 *  - Firmware reacts to event (event-driven)
 ******************************************************************************/

/* === Includes ============================================================ */
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "gpio.h"
#include "i2c.h"
#include "usart.h"

#include "WSEN_PADS_2511020213301.h"
#include "platform.h"

/* === Configuration ======================================================= */
#define DEBUG_MODE true

#define FIFO_THRESHOLD 25U
#define BUFFER_GROUP_SIZE 5U
#define BUFFER_TOTAL_GROUPS (FIFO_THRESHOLD / BUFFER_GROUP_SIZE)

/* Ensure FIFO grouping is valid */
_Static_assert(FIFO_THRESHOLD % BUFFER_GROUP_SIZE == 0,
               "FIFO_THRESHOLD must be multiple of BUFFER_GROUP_SIZE");

/* === Sensor interface ==================================================== */
WE_sensorInterface_t pads = {
    .sensorType = WE_PADS,
    .interfaceType = WE_i2c,
    .options = {.i2c = {.address = PADS_ADDRESS_I2C_1,
                        .burstMode = 1,
                        .slaveTransmitterMode = 0,
                        .useRegAddrMsbForMultiBytesRead = 0,
                        .reserved = 0},
                .readTimeout = 1000,
                .writeTimeout = 1000},
    .handle = &hi2c2};

/* === Globals ============================================================= */
/* Flag set by ISR when FIFO threshold interrupt occurs */
static volatile bool fifo_event = false;

/* Fifo state variables */
static uint8_t fifoLevel;
static PADS_state_t fifoFull;

/* FIFO raw buffers */
static int32_t pressure_buffer[PADS_FIFO_BUFFER_SIZE];
static int16_t temperature_buffer[PADS_FIFO_BUFFER_SIZE];

/* Processed data */
static uint32_t pressure_avg[BUFFER_TOTAL_GROUPS];
static uint16_t temperature_avg[BUFFER_TOTAL_GROUPS];

/* Time reference */
static uint32_t start_time_ms = 0;

/* === Forward declarations ============================================== */
extern void SystemClock_Config(void);

static void mcu_init(void);
static bool pads_init(void);
static bool pads_start(void);

static void pads_handle_fifo_event(void);
static void pads_compute_averages(void);
static void pads_print_data(void);

/* === Main ================================================================ */
int main(void) {
  mcu_init();
  pads_init();
  pads_start();

  start_time_ms = HAL_GetTick();

  while (1) {
    if (fifo_event) {
      fifo_event = false;
      pads_handle_fifo_event();
    }
  }
}

static void pads_handle_fifo_event(void) {
  PADS_getFifoFillLevel(&pads, &fifoLevel);
  PADS_isFifoFull(&pads, &fifoFull);

  if (fifoFull)
  {
    PADS_getFifoValues_int(&pads, PADS_FIFO_BUFFER_SIZE, pressure_buffer,
                             temperature_buffer);

    printf("FIFO full \r\n");                     
  }
  else
  {
    if (fifoLevel ==  FIFO_THRESHOLD)
    {
      PADS_getFifoValues_int(&pads, FIFO_THRESHOLD, pressure_buffer,
                             temperature_buffer);
      
      pads_compute_averages();
      pads_print_data();
    }
    else if (fifoLevel > FIFO_THRESHOLD && fifoLevel < PADS_FIFO_BUFFER_SIZE)
    {
      PADS_getFifoValues_int(&pads, PADS_FIFO_BUFFER_SIZE, pressure_buffer,
                               temperature_buffer);
      printf("FIFO Threshold Overrun \r\n"); 
      
    }
  }
}

static void pads_compute_averages(void) {
  for (uint8_t group = 0; group < BUFFER_TOTAL_GROUPS; group++) {
    uint32_t p_sum = 0;
    uint32_t t_sum = 0;

    for (uint8_t i = 0; i < BUFFER_GROUP_SIZE; i++) {
      uint8_t idx = group * BUFFER_GROUP_SIZE + i;
      p_sum += pressure_buffer[idx];
      t_sum += temperature_buffer[idx];
    }

    pressure_avg[group] = p_sum / BUFFER_GROUP_SIZE;
    temperature_avg[group] = t_sum / BUFFER_GROUP_SIZE;
  }
}

static void pads_print_data(void) {
  uint32_t elapsed_ms = HAL_GetTick() - start_time_ms;

  printf("%8" PRIu32 " %8" PRIu16 " %8" PRIu32 " %8u\r\n",
         pressure_avg[0],temperature_avg[0],elapsed_ms,(unsigned int)fifoLevel);
}

static bool pads_init(void) {
  HAL_Delay(50);

  while (WE_isSensorInterfaceReady(&pads) != WE_SUCCESS) {
  }

  uint8_t device_id = 0;
  if (PADS_getDeviceID(&pads, &device_id) != WE_SUCCESS) {
    return false;
  }

  if (device_id != PADS_DEVICE_ID_VALUE) {
    return false;
  }

  /* Soft reset */
  PADS_softReset(&pads, PADS_enable);
  PADS_state_t reset_state;
  do {
    PADS_getSoftResetState(&pads, &reset_state);
  } while (reset_state);

  /* Sensor configuration */
  PADS_setPowerMode(&pads, PADS_lowNoise);
  PADS_enableAutoIncrement(&pads, PADS_enable);
  PADS_enableLowPassFilter(&pads, PADS_enable);
  PADS_setLowPassFilterConfig(&pads, PADS_lpFilterBW2);
  PADS_enableBlockDataUpdate(&pads, PADS_enable);

  /* Interrupt configuration */
  PADS_setInterruptActiveLevel(&pads, PADS_activeHigh);
  PADS_setInterruptPinType(&pads, PADS_pushPull);

  return true;
}

static bool pads_start(void) {
  PADS_setFifoMode(&pads, PADS_continuousMode);
  PADS_setFifoThreshold(&pads, FIFO_THRESHOLD);
  PADS_enableFifoThresholdInterrupt(&pads, PADS_enable);
  // PADS_enableFifoOverrunInterrupt(&pads, PADS_enable);     // Se ha sobrescrito 1 medida
  // PADS_enableFifoFullInterrupt(&pads, PADS_enable);        // Se ha llenado la fifo 
  PADS_setInterruptEventControl(&pads, PADS_dataReady);
  PADS_setOutputDataRate(&pads, PADS_outputDataRate100Hz);

  return true;
}

static void mcu_init(void) {
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_I2C2_Init();
  MX_USART2_UART_Init();
}

int __io_putchar(int ch) {
#if DEBUG_MODE
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
#endif
  return ch;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  if (GPIO_Pin == PADS2_INT1_Pin) {
    fifo_event = true;
  }
}
