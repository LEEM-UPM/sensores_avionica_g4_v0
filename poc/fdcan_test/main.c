#include "main.h"
#include <stdio.h> // Para printf si se usa

// Definimos nuestro propio handler para no depender del de Core, podríamos usar 'extern' pero para aislar creo uno local
FDCAN_HandleTypeDef hfdcan_poc; 

// Variables para el mensaje
FDCAN_TxHeaderTypeDef TxHeader;
uint8_t TxData[8] = {0xCA, 0xFE, 0xBA, 0xBE, 0, 0, 0, 0};

//funciones
void SystemClock_Config(void); // la traemos del Core (es compleja de replicar)
void My_FDCAN_Init(void);      // Nuestra versión corregida
void My_GPIO_Init(void);       // Para el LED
void Error_Handler(void);
int main(void)
{
  // 1. Inicialización del chip
  HAL_Init();
  SystemClock_Config();
  My_GPIO_Init();
  My_FDCAN_Init(); // 2. Inicialización del CAN con nuestra función
  // 3. Configurar Filtros 
  FDCAN_FilterTypeDef sFilterConfig;
  sFilterConfig.IdType = FDCAN_STANDARD_ID;
  sFilterConfig.FilterIndex = 0;
  sFilterConfig.FilterType = FDCAN_FILTER_MASK;
  sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
  sFilterConfig.FilterID1 = 0x000; 
  sFilterConfig.FilterID2 = 0x000; // Aceptar todo
  
if (HAL_FDCAN_ConfigFilter(&hfdcan_poc, &sFilterConfig) != HAL_OK) {
     Error_Handler();
}
  // 4. Arrancar
  if (HAL_FDCAN_Start(&hfdcan_poc) != HAL_OK) {
     while(1);
  }

  // 5. Configurar Cabecera de envío
  TxHeader.Identifier = 0x555; // ID diferente para notar el cambio
  TxHeader.IdType = FDCAN_STANDARD_ID;
  TxHeader.TxFrameType = FDCAN_DATA_FRAME;
  TxHeader.DataLength = FDCAN_DLC_BYTES_8;
  TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  TxHeader.BitRateSwitch = FDCAN_BRS_OFF;
  TxHeader.FDFormat = FDCAN_CLASSIC_CAN;
  TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
  TxHeader.MessageMarker = 0;

  while (1)
  {
      // Enviar
      if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan_poc, &TxHeader, TxData) == HAL_OK) {
          // Parpadeo lento = Todo OK
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_9);
          HAL_Delay(200);
          HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_9);
          HAL_Delay(800);
      } else {
          // Si falla (buffer lleno), espera un poco
          HAL_Delay(10);
      }
  }
}

// NUESTRA VERSIÓN DE LA INICIALIZACIÓN
void My_FDCAN_Init(void)
{
  hfdcan_poc.Instance = FDCAN1;
  hfdcan_poc.Init.ClockDivider = FDCAN_CLOCK_DIV1;
  hfdcan_poc.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
  hfdcan_poc.Init.Mode = FDCAN_MODE_NORMAL;
  hfdcan_poc.Init.AutoRetransmission = DISABLE;
  hfdcan_poc.Init.TransmitPause = DISABLE;
  hfdcan_poc.Init.ProtocolException = DISABLE;
  // Configuración de velocidad (500kHz aprox dependiendo del reloj)
  hfdcan_poc.Init.NominalPrescaler = 16; 
  hfdcan_poc.Init.NominalSyncJumpWidth = 1;
  hfdcan_poc.Init.NominalTimeSeg1 = 2; // Ajustar según tu reloj real si es necesario
  hfdcan_poc.Init.NominalTimeSeg2 = 2;
  hfdcan_poc.Init.DataPrescaler = 1;
  hfdcan_poc.Init.DataSyncJumpWidth = 1;
  hfdcan_poc.Init.DataTimeSeg1 = 1;
  hfdcan_poc.Init.DataTimeSeg2 = 1;
  hfdcan_poc.Init.StdFiltersNbr = 1; // <-
  hfdcan_poc.Init.ExtFiltersNbr = 0;
  hfdcan_poc.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
  
  if (HAL_FDCAN_Init(&hfdcan_poc) != HAL_OK)
  {
Error_Handler();
}
}

// Inicialización simple del LED 
void My_GPIO_Init(void)
{
  
  
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOB_CLK_ENABLE();
 
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}