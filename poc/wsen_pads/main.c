/******************************************************************************
 * WSEN-PADS FIFO threshold interrupt + DMA - Proof of Concept
 * https://www.we-online.com/components/products/manual/Manual-um-wsen-pads-2511020213301%20(rev3.1).pdf
 *
 * Flujo:
 *  1. El sensor acumula muestras en su FIFO interno.
 *  2. Al superar el umbral, genera una interrupción en PA5 (PADS2_INT1).
 *  3. La ISR activa un flag; el main lanza una lectura DMA (no bloqueante).
 *  4. Al terminar el DMA, el callback decodifica e imprime las muestras.
 ******************************************************************************/

/* === Includes ============================================================ */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "gpio.h"
#include "i2c.h"
#include "usart.h"

#include "WSEN_PADS_2511020213301.h"
#include "platform.h"

/* === Configuración ======================================================= */
#define DEBUG_MODE true

/* Número de muestras acumuladas antes de generar la interrupción de umbral */
#define FIFO_THRESHOLD 25U

/* Cada muestra del sensor ocupa 5 bytes en bruto: 3 de presión + 2 de temperatura */
#define PADS_RAW_BYTES_PER_SAMPLE 5U

/* === Interfaz con el sensor ============================================== */
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

/* === Variables globales ================================================== */

/* Flag activado por la ISR de EXTI cuando el FIFO supera el umbral */
static volatile bool fifo_event = false;

/* Buffer DMA donde el periférico I2C escribe los bytes en bruto */
static uint8_t dma_rx_buf[PADS_FIFO_BUFFER_SIZE * PADS_RAW_BYTES_PER_SAMPLE];

/* Número de muestras pedidas en la transferencia DMA en curso */
static volatile uint8_t dma_samples_pending = 0;

/* Buffers de muestras ya decodificadas (presión en Pa, temperatura raw) */
static int32_t pressure_buffer[PADS_FIFO_BUFFER_SIZE];
static int16_t temperature_buffer[PADS_FIFO_BUFFER_SIZE];

/* Referencia de tiempo para los mensajes de debug */
static uint32_t start_time_ms = 0;

/* === Declaraciones previas =============================================== */
extern void SystemClock_Config(void);
static void mcu_init(void);
static bool pads_init(void);
static bool pads_start(uint8_t fifo_threshold);
static void pads_handle_fifo_event(void);
static void pads_decode_dma_buffer(uint8_t num_samples);
static void pads_print_data(uint8_t num_samples);

/* === Main ================================================================ */
int main(void) {
  mcu_init();
  pads_init();
  pads_start(FIFO_THRESHOLD);

  start_time_ms = HAL_GetTick();
  printf("WSEN-PADS listo. Esperando datos...\r\n");

  /* El main solo gestiona el flag; todo el trabajo pesado ocurre en callbacks */
  while (1) {
    if (fifo_event) {
      fifo_event = false;
      pads_handle_fifo_event();
    }
  }
}

/* Lanza la lectura DMA cuando el FIFO tiene datos */
static void pads_handle_fifo_event(void) {
  uint8_t fifo_level = 0;

  if (PADS_getFifoFillLevel(&pads, &fifo_level) != WE_SUCCESS) {
    return;
  }

  if (fifo_level == 0) {
    return;
  }

  /* Limitar al tamaño máximo del buffer para no desbordar */
  uint8_t samples_to_read =
      (fifo_level > PADS_FIFO_BUFFER_SIZE) ? PADS_FIFO_BUFFER_SIZE : fifo_level;

  if (fifo_level >= PADS_FIFO_BUFFER_SIZE) {
    printf("[WARN]: FIFO lleno, posible pérdida de datos.\r\n");
  }

  /* Iniciar transferencia DMA: no bloqueante, continúa en el callback */
  dma_samples_pending = samples_to_read;
  HAL_I2C_Mem_Read_DMA(&hi2c2,
                        (uint16_t)(PADS_ADDRESS_I2C_1 << 1),
                        PADS_FIFO_DATA_P_XL_REG,
                        I2C_MEMADD_SIZE_8BIT,
                        dma_rx_buf,
                        (uint16_t)(samples_to_read * PADS_RAW_BYTES_PER_SAMPLE));
}

/* Llamado automáticamente por HAL cuando el DMA termina de recibir */
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c) {
  if (hi2c->Instance != I2C2) {
    return;
  }

  uint8_t num_samples = dma_samples_pending;
  dma_samples_pending = 0;

  pads_decode_dma_buffer(num_samples);
  pads_print_data(num_samples);
}

/* Convierte los bytes en bruto del buffer DMA a presión (Pa) y temperatura */
static void pads_decode_dma_buffer(uint8_t num_samples) {
  for (uint8_t i = 0; i < num_samples; i++) {
    const uint8_t *b = &dma_rx_buf[i * PADS_RAW_BYTES_PER_SAMPLE];

    /* Presión: 24 bits con signo, primero el byte menos significativo */
    int32_t raw_p = (int32_t)((uint32_t)b[2] << 24 |
                               (uint32_t)b[1] << 16 |
                               (uint32_t)b[0] << 8);
    raw_p /= 256;                        /* extensión de signo a 32 bits */
    pressure_buffer[i] = (raw_p * 100) / 4096; /* conversión a Pa */

    /* Temperatura: 16 bits con signo, primero el byte menos significativo */
    temperature_buffer[i] = (int16_t)((uint16_t)b[4] << 8 | b[3]);
  }
}

/* Imprime todas las muestras decodificadas por UART */
static void pads_print_data(uint8_t num_samples) {
  uint32_t elapsed = HAL_GetTick() - start_time_ms;

  printf("--- %lu ms | %u muestras ---\r\n", elapsed, num_samples);

  for (uint8_t i = 0; i < num_samples; i++) {
    printf("  [%u] Presion: %ld Pa | Temp raw: %d\r\n",
           i, pressure_buffer[i], temperature_buffer[i]);
  }
}

static bool pads_init(void) {
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
