#include "fdcan.h"
#include "gpio.h"
#include "stm32g4xx_hal_def.h"
#include "usart.h"
#include <stdbool.h>
#include <string.h>

extern void SystemClock_Config(void);

/* ============================================================
 * EXAMPLE CONFIGURATION
 * ============================================================ */
#define TEST_CAN_ID 0x123
#define TEST_PAYLOAD 0xA5
#define TX_PERIOD_MS 200

/* ============================================================
 * VARIABLES SHARED BETWEEN IRQ AND MAIN CONTEXT
 * ============================================================ */

/* Simple flag indicating that a new CAN frame has been received */
static volatile bool fdcan_rx_pending = false;

/* Buffer where the IRQ copies the last received CAN frame */
static FDCAN_RxHeaderTypeDef rx_header;
static uint8_t rx_data[8];

/* ============================================================
 * INTERNAL VARIABLES
 * ============================================================ */
static uint32_t last_tx_tick = 0;

/* ============================================================
 * PROTOTYPES
 * ============================================================ */
static void fdcan_core_init(void);
static void fdcan_send_test_frame(void);
static void fdcan_process_rx(void);


/* ============================================================
 * MAIN
 * ============================================================ */
int main(void) {
  HAL_Init();
  SystemClock_Config();
  

  MX_GPIO_Init();
  MX_FDCAN1_Init();
  MX_FDCAN2_Init();

  /* ------------------------------------------------------------
   * TCAN1044:
   * STB = LOW -> Normal mode (CAN enabled)
   * ------------------------------------------------------------ */
  HAL_GPIO_WritePin(CAN1_STB_GPIO_Port, CAN1_STB_Pin | CAN2_STB_Pin, GPIO_PIN_RESET);
  
  fdcan_core_init();

  while (1) {
    uint32_t now = HAL_GetTick();
    
    if(fdcan_rx_pending){
      fdcan_process_rx();
    }
  }
}

static void fdcan_core_init(void) {
  /* Accept any standard ID and route it to RX FIFO0 */
  FDCAN_FilterTypeDef filter = {.IdType = FDCAN_STANDARD_ID,
                                .FilterIndex = 0,
                                .FilterType = FDCAN_FILTER_MASK,
                                .FilterConfig = FDCAN_FILTER_TO_RXFIFO0,
                                .FilterID1 = 0x000,
                                .FilterID2 = 0x000};

  HAL_FDCAN_ConfigFilter(&hfdcan2, &filter);

  /* Enable interrupt on new message in RX FIFO0 */
  HAL_FDCAN_ActivateNotification(&hfdcan2, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);

  HAL_FDCAN_Start(&hfdcan1);
  HAL_FDCAN_Start(&hfdcan2);
}

static void fdcan_send_test_frame(void) {
  FDCAN_TxHeaderTypeDef hdr = {.Identifier = TEST_CAN_ID,
                               .IdType = FDCAN_STANDARD_ID,
                               .TxFrameType = FDCAN_DATA_FRAME,
                               .DataLength = FDCAN_DLC_BYTES_1,
                               .ErrorStateIndicator = FDCAN_ESI_ACTIVE,
                               .BitRateSwitch = FDCAN_BRS_OFF,
                               .FDFormat = FDCAN_CLASSIC_CAN,
                               .TxEventFifoControl = FDCAN_NO_TX_EVENTS,
                               .MessageMarker = 0};

  uint8_t data = TEST_PAYLOAD;

  HAL_StatusTypeDef status =
      HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &hdr, &data);

  if (status != HAL_OK) {
    Error_Handler();
  }
}

/* ============================================================
 * FDCAN RX INTERRUPT CALLBACK
 *
 * IMPORTANT:
 * - Keep this function as short as possible
 * - Do NOT add application logic here
 * - Do NOT use blocking calls (e.g. UART)
 * ============================================================ */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan,
                               uint32_t RxFifo0ITs) {
  if ((hfdcan->Instance == FDCAN2) &&
      (RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE)) {
    HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rx_header, rx_data);

    /* Notify main loop that a new frame is available */
    fdcan_rx_pending = true;
  }
}

static void fdcan_process_rx(void) {
  fdcan_rx_pending = false;

  if ((rx_header.Identifier == TEST_CAN_ID) && (rx_data[0] == TEST_PAYLOAD)) {
    //testing purposes
    HAL_GPIO_TogglePin(TEST_LED_GPIO_Port, TEST_LED_Pin);
    
  } 
}

