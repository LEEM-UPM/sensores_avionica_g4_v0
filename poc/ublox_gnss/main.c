/******************************************************************************
 * WSEN-PADS FIFO threshold interrupt - Proof of Concept
 *
 * Concept:
 *  - Real time data gathering and storing insid
 *  - FIFO threshold interrupt triggers data read
 *  - Firmware reacts to event (event-driven)
 ******************************************************************************/



#include "main.h"
#include "usart.h"
#include "gpio.h"
#include "minmea.h"


// State variables 
#define UART_RX_BUFFER_SIZE 128 // circular

// Global GPS data variables
float g_latitud = 0.0f;
float g_longuitud = 0.0f;
int g_satelites_activos = 0;

// exterm function declarations
extern void SystemClock_Config();
// function declarations
void M10S_Init_Configuration(UART_HandleTypeDef *huart);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);

// llaves de configuracion a 5HZ
const uint8_t UBX_CFG_RATE_5HZ[] = {
    0xB5, 0x62, 0x06, 0x8A, 0x0A, 0x00, 
    0x01, 0x01, 0x00, 0x00, 0x01, 0x00, 0x21, 0x30, 0xC8, 0x00, 
    0xF3, 0xAC // Checksum calculado (alg de fletcher)
};

extern UART_HandleTypeDef huart2;

// MAIN CODE
int main(){
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();

    M10S_Init_Configuration(&huart2);

    while(1){
        // GPS data is updated automatically in HAL_UART_RxCpltCallback
        // Access g_latitud, g_longuitud, g_satelites_activos as needed
        HAL_Delay(1000);
    }
    

    
}

void M10S_Init_Configuration(UART_HandleTypeDef *huart){
    HAL_Delay(500);
    HAL_UART_Transmit(huart, (uint8_t*)UBX_CFG_RATE_5HZ, sizeof(UBX_CFG_RATE_5HZ), 100);
    HAL_Delay(100);
}

// callback the interrucion al recibir un byte
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
    static int line_idx = 0;
    static char line_buffer[UART_RX_BUFFER_SIZE];
    uint8_t rx_byte;

    HAL_UART_Receive_IT(huart, &rx_byte, 1);

    if(huart->Instance == USART2){
        // add the line if place available on circular buffer
        if(line_idx < sizeof(line_buffer) - 1){
            line_buffer[line_idx++] = (char)rx_byte;
        }

        // detectar fin de linea 
        if(rx_byte == '\n'){
            line_buffer[line_idx] = 0;

            // Filtrar solo las lineas que me interesan
            if(minmea_sentence_id(line_buffer,false) == MINMEA_SENTENCE_RMC){
                struct minmea_sentence_rmc frame;

                // en principio datos de altitud son del barometro (gps se capa)
                if(minmea_parse_rmc(&frame,line_buffer)){
                    g_latitud = minmea_tocoord(&frame.latitude);
                    g_longuitud = minmea_tocoord(&frame.longitude);
                    g_satelites_activos = frame.satellites_tracked;
                }
            }

            //limpiar el indice para escribir otra linea desde 0
            line_idx = 0;


            HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
        }
        
    }
}


