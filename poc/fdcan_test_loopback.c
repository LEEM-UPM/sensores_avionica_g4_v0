#include "main.h"
#include "fdcan.h"
#include "gpio.h"
#include "stm32g4xx_hal_def.h"
#include "usart.h"
#define TESTING_BLINKER 0

extern void SystemClock_Config(void);

#define FDCAN1_TEST_ID 0x123
#define FDCAN1_TEST_PAYLOAD 0xA5
#define TX_PEDIOD_MS 200

int main(void){
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_FDCAN1_Init();
    MX_FDCAN2_Init();

    // Configure Filter for loopback communication 
    FDCAN_FilterTypeDef filter;
    filter.IdType = FDCAN_STANDARD_ID;
    filter.FilterIndex = 0;
    filter.FilterType = FDCAN_FILTER_RANGE;
    filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    filter.FilterID1 = 0x123;
    filter.FilterID2 = 0x123;

    //RX & TX config data
    FDCAN_TxHeaderTypeDef TxHeader;
    FDCAN_RxHeaderTypeDef RxHeader;
    TxHeader.Identifier = 0x123;
    TxHeader.IdType = FDCAN_STANDARD_ID;
    TxHeader.TxFrameType = FDCAN_DATA_FRAME; 
    TxHeader.DataLength = FDCAN_DLC_BYTES_8;
    TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    TxHeader.BitRateSwitch = FDCAN_BRS_OFF;
    TxHeader.FDFormat = FDCAN_CLASSIC_CAN; 
    TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    TxHeader.MessageMarker = 0;

    uint8_t TxData[8] = {'h','e','l','l','o','\0'};
    uint8_t RxData[8];  
    

    // Filter config data
    HAL_FDCAN_ConfigFilter(&hfdcan1, &filter);
    HAL_FDCAN_ConfigFilter(&hfdcan2, &filter);

    // Interrupts for reception FIFO (flag) 
    HAL_FDCAN_ActivateNotification(&hfdcan2, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
    HAL_FDCAN_Start(&hfdcan1);
    HAL_FDCAN_Start(&hfdcan2);


    // for debug purposes 
    // por alguna razon solo funciona cuando lo 
    // flasheas con el cable de 5v soltado y luego lo conectas todo despues a la vez
    // suerte para quien le toque descubrir porque

    #if TESTING_BLINKER == 1
        while(1){
            HAL_GPIO_TogglePin(TEST_LED_GPIO_Port, TEST_LED_Pin);
            HAL_Delay(100);
        }
    #endif

    // LOOP
    while(1){
        //send
        HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader, TxData);
        //receive
        // 7. Check FDCAN2 Receive FIFO 0 and Read the data
        if (HAL_FDCAN_GetRxFifoFillLevel(&hfdcan2, FDCAN_RX_FIFO0) > 0){
            if (HAL_FDCAN_GetRxMessage(&hfdcan2, FDCAN_RX_FIFO0, &RxHeader, RxData) == HAL_OK){
                // Success! RxData now contains "Hello"
                // RxHeader.Identifier will automatically equal 0x123
                HAL_GPIO_TogglePin(TEST_LED_GPIO_Port, TEST_LED_Pin); // Turn on Nucleo LED
            }
        }
        
        HAL_Delay(100);
    }
}
