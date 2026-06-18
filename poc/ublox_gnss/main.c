/******************************************************************************
 * WSEN-PADS FIFO threshold interrupt - Proof of Concept
 *
 * Concept:
 *  - Real time data gathering and storing insid
 *  - FIFO threshold interrupt triggers data read
 *  - Firmware reacts to event (event-driven)
 ******************************************************************************/



 /* === Includes ============================================================ */
#include "main.h"
#include "usart.h"
#include "gpio.h"
#include "minmea.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>


/* === Configuration ======================================================= */
#define DEBUG_MODE true
#define UART_RX_BUFFER_SIZE 128 

const uint8_t UBX_CFG_RATE_5HZ[] = {
    0xB5, 0x62, 0x06, 0x8A, 0x0A, 0x00, 
    0x01, 0x01, 0x00, 0x00, 0x01, 0x00, 0x21, 0x30, 0xC8, 0x00, 
    0xF3, 0xAC 
};

/* === Globals ============================================================= */
/* Flag set by ISR when FIFO treshhold overflows */
static volatile bool gnss_event = false;

/* memory buffers */
static uint8_t gps_dma_buffer[UART_RX_BUFFER_SIZE];
static char process_buffer[UART_RX_BUFFER_SIZE];

/* state variables */
float g_latitud = 0.0f;
float g_longuitud = 0.0f;
int g_satelites_activos = 0;

/* Time reference */
static uint32_t start_time_ms = 0;

/* === Forward declarations ============================================== */
extern void SystemClock_Config();
static void mcu_init(void);
static void M10S_init(UART_HandleTypeDef *huart);
static void gnss_process_block(void);
static void gnss_dma_startup(void);

/* === Main ============================================================== */
int main(){
    
    mcu_init();
    M10S_init(&huart2);
    gnss_dma_startup();

    start_time_ms = HAL_GetTick();

    
    while(1){

        if(gnss_event){
            gnss_event = false;
            gnss_process_block();
        }
    }
}


static void gnss_process_block(void){
    char* line = strtok(process_buffer, "\r\n");

    while(line != NULL){
        if(minmea_sentence_id(line, false) == MINMEA_SENTENCE_GGA){
            struct minmea_sentence_gga frameGGA;

            if(minmea_parse_gga(&frameGGA, line)){
                g_latitud = minmea_tocoord(&frameGGA.latitude);
                g_longuitud = minmea_tocoord(&frameGGA.longitude);
                g_satelites_activos = frameGGA.satellites_tracked;

                #if DEBUG_MODE
                printf("[GNSS SYNC] Lat: %f | Lon: %f | satellites: %d\r\n", g_latitud, g_longuitud, g_satelites_activos);
                #endif
            }
        }

        line = strtok(NULL, "\r\n");
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size){
    if(huart->Instance == USART2){
        memcpy(process_buffer, (char*)gps_dma_buffer, Size);
        process_buffer[Size] = '\0';

        gnss_event = true;

        // reload DMA since flag fired
        gnss_dma_startup();
    }
}

static void gnss_dma_startup(void){
    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, gps_dma_buffer, UART_RX_BUFFER_SIZE);
    __HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);
}

static void mcu_init(void){
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();
}


static void M10S_init(UART_HandleTypeDef *huart){
    HAL_Delay(500);
    HAL_UART_Transmit(huart, (uint8_t*)UBX_CFG_RATE_5HZ, sizeof(UBX_CFG_RATE_5HZ), 100);
    HAL_Delay(100);
}

int __io_putchar(int ch){
#if DEBUG_MODE
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
#endif
    return ch;
}



