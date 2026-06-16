#include "main.h"
#include "gpio.h"
#include "fdcan.h"
#include "stm32g4xx_hal_def.h"

#define FDCAN_AVIONICS_ID 0x10A
#define FDCAN_CORE_ID 0x20A

extern void SystemClock_Config(void);

FDCAN_TxHeaderTypeDef TxHeader;
FDCAN_FilterTypeDef RecvFilter;
uint8_t TxData[8] = {'h','e','l','l','o','\n'};

void main(){
    // transmit config
    TxHeader.Identifier = FDCAN_AVIONICS_ID;
    TxHeader.IdType = FDCAN_STANDARD_ID;
    TxHeader.TxFrameType = FDCAN_DATA_FRAME;
    TxHeader.DataLength = FDCAN_DLC_BYTES_0;
    TxHeader.ErrorStateIndicator = FDCAN_DLC_BYTES_0;
    TxHeader.BitRateSwitch = FDCAN_BRS_OFF;
    TxHeader.FDFormat = FDCAN_CLASSIC_CAN;
    TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    TxHeader.MessageMarker = 0;

    // acknowledge messages filter
    RecvFilter.IdType = FDCAN_STANDARD_ID;
    RecvFilter.FilterIndex = 0;
    RecvFilter.FilterType = FDCAN_FILTER_MASK;
    RecvFilter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    RecvFilter.FilterID1 = FDCAN_CORE_ID;
    RecvFilter.FilterID2 = 0x7FF; // forces to match 11 bytes

    if(HAL_FDCAN_ConfigFilter(&hfdcan1, &RecvFilter) != HAL_OK){
        Error_Handler();
    }

    // Reject Noise to the bus unless neccesary
    HAL_FDCAN_ConfigGlobalFilter(&hfdcan1, FDCAN_REJECT, FDCAN_REJECT, FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);

    // start and enable rx acknowledge interrupt
    HAL_FDCAN_Start(&hfdcan1);
    HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);

    // send hello message
    if(HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader, TxData) != HAL_OK ){
        Error_Handler();
    }
}