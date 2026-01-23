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
#define MAX_GROUPS (PADS_FIFO_BUFFER_SIZE / BUFFER_GROUP_SIZE)

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

/* FIFO raw buffers */
static int32_t pressure_buffer[PADS_FIFO_BUFFER_SIZE];
static int16_t temperature_buffer[PADS_FIFO_BUFFER_SIZE];

/* Processed data */
static uint32_t pressure_avg[MAX_GROUPS];
static uint16_t temperature_avg[MAX_GROUPS];

/* Time reference */
static uint32_t start_time_ms = 0;

/* === Forward declarations ============================================== */
extern void SystemClock_Config(void);
static void mcu_init(void);
static bool pads_init(void);
static bool pads_start(uint8_t fifo_threshold);
static void pads_handle_fifo_event(void);
static void pads_compute_averages(uint8_t num_samples);
static void pads_print_data(uint8_t num_groups, uint8_t raw_level);

/* === Main ================================================================
 */
int main(void) {
  mcu_init();
  pads_init();
  pads_start(FIFO_THRESHOLD);

  start_time_ms = HAL_GetTick();
  printf("WSEN-PADS Ready. Waiting for data...\r\n");

  while (1) {
    if (fifo_event) {
      fifo_event = false;
      pads_handle_fifo_event();
    }
  }
}

static void pads_handle_fifo_event(void) {
  uint8_t fifo_level = 0;

  if (PADS_getFifoFillLevel(&pads, &fifo_level) != WE_SUCCESS) {
    return;
  }

  if (fifo_level == 0) {
    return;
  }

  /* 2. Protección de desbordamiento de buffer local */
  uint8_t samples_to_read =
      (fifo_level > PADS_FIFO_BUFFER_SIZE) ? PADS_FIFO_BUFFER_SIZE : fifo_level;

  if (PADS_getFifoValues_int(&pads, samples_to_read, pressure_buffer,
                             temperature_buffer) != WE_SUCCESS) {
    return;
  }

  if (fifo_level >= PADS_FIFO_BUFFER_SIZE) {
    printf("[WARN]: FIFO Overrun detected! Data loss occurred.\r\n");
  }

  if (samples_to_read >= FIFO_THRESHOLD) {
    uint8_t groups_processed = samples_to_read / BUFFER_GROUP_SIZE;
    pads_compute_averages(samples_to_read);
    pads_print_data(groups_processed, fifo_level);
  }
}

static void pads_compute_averages(uint8_t num_samples) {
  uint8_t total_groups = num_samples / BUFFER_GROUP_SIZE;
  if (total_groups > MAX_GROUPS)
    total_groups = MAX_GROUPS;

  for (uint8_t group = 0; group < total_groups; group++) {
    int64_t p_sum = 0;
    int32_t t_sum = 0;

    for (uint8_t i = 0; i < BUFFER_GROUP_SIZE; i++) {
      uint8_t idx = (group * BUFFER_GROUP_SIZE) + i;
      p_sum += pressure_buffer[idx];
      t_sum += temperature_buffer[idx];
    }

    pressure_avg[group] = (uint32_t)(p_sum / BUFFER_GROUP_SIZE);
    temperature_avg[group] = (uint16_t)(t_sum / BUFFER_GROUP_SIZE);
  }
}

static void pads_print_data(uint8_t num_groups, uint8_t raw_level) {
  uint32_t elapsed = HAL_GetTick() - start_time_ms;

  // Imprimimos una cabecera para saber cuántos datos han llegado en este lote
  printf("--- Batch at %lu ms | FIFO Level: %u | Groups: %u ---\r\n", elapsed,
         raw_level, num_groups);

  // Bucle para recorrer y mostrar cada una de las medidas promediadas
  for (uint8_t i = 0; i < num_groups; i++) {
    printf("  Group[%u] -> Pres: %lu Pa | Temp: %u\r\n", i, pressure_avg[i],
           temperature_avg[i]);
  }
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

static bool pads_start(uint8_t fifo_threshold) {
  PADS_setFifoMode(&pads, PADS_continuousMode);
  PADS_setFifoThreshold(&pads, fifo_threshold);
  PADS_enableFifoThresholdInterrupt(&pads, PADS_enable);
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
