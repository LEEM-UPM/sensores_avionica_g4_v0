/******************************************************************************
 * WSEN-HIDS - Proof of Concept (Modo Polling / One-Shot)
 *
 * Concepto:
 * - El sensor HIDS no tiene interrupciones de Data Ready.
 * - El microcontrolador pide una medida, espera y lee el resultado.
 ******************************************************************************/

/* === Includes ============================================================ */
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "gpio.h"
#include "i2c.h"
#include "usart.h"

#include "WSEN_HIDS_2525020210002.h"
#include "platform.h"

/* === Configuration ======================================================= */
#define DEBUG_MODE true

/* === Sensor interface ==================================================== */
WE_sensorInterface_t hids;

/* === Globals ============================================================= */
static uint32_t start_time_ms = 0;

/* === Forward declarations ============================================== */
extern void SystemClock_Config(void);
static void mcu_init(void);
static bool hids_init(void);

/* === Main ================================================================ */
int main(void) {
  mcu_init();
  
  printf("\r\n--- Inicializando Prueba HIDS (Modo Polling) ---\r\n");
  
  if (!hids_init()) {
      printf("[ERROR] Fallo al inicializar HIDS. Comprueba los cables.\r\n");
      while(1) {
          HAL_Delay(100); // Bloquear si falla
      }
  }

  start_time_ms = HAL_GetTick();
  printf("WSEN-HIDS Listo. Empezando a leer datos...\r\n\r\n");

  int32_t temperatureRaw = 0;
  int32_t humidityRaw = 0;

  /* Bucle principal: Preguntar, leer, esperar y repetir */
  while (1) {
      
      // 1. Pedir al sensor que mida en modo de Alta Precisión (HPM)
      if (WE_SUCCESS == HIDS_Sensor_Measure_Raw(&hids, HIDS_MEASURE_HPM, &temperatureRaw, &humidityRaw)) {
          
          uint32_t elapsed = HAL_GetTick() - start_time_ms;
          
          // 2. Imprimir los resultados
          printf("[%lu ms] Humedad(Raw): %li | Temperatura(Raw): %li \r\n", elapsed, humidityRaw, temperatureRaw);
          
      } else {
          printf("[WARN] Fallo al leer el sensor por I2C.\r\n");
      }

      // 3. Esperar 1 segundo antes de volver a molestar al sensor
      HAL_Delay(1000); 
  }
}

/* === Sensor Setup ======================================================== */
static bool hids_init(void) {
  /* 1. Configurar la interfaz física */
  HIDS_Get_Default_Interface(&hids);
  hids.interfaceType = WE_i2c;
  hids.handle = &hi2c2; // Mantengo tu I2C2

  HAL_Delay(50); // Tiempo para que el sensor reciba corriente

  /* 2. Inicializar el sensor de verdad */
  if (WE_SUCCESS != HIDS_Sensor_Init(&hids)) {
      return false;
  }

  return true;
}

/* === System Functions ==================================================== */
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