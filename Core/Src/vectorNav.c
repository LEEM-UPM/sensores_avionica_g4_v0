#include "vectorNav.h"

void VN_Config(UART_HandleTypeDef* huart){
    char config_cmd = "$VNWRG,75,1,4,1,0529*XX\r\n";

    HAL_UART_Transmit(&huart, (uint8_t*)config_cmd, sizeof(config_cmd) - 1, HAL_MAX_DELAY);
}

_Bool VN_read(UART_HandleTypeDef* huart, VN100_Data_t* vnData){
    uint8_t rx_byte = 0; 
    uint8_t vn_size = 0;
    uint8_t buffer[VN_messageSize];
    v8 v8 = {
        0.0f,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    };
    v4 v4 = {
        0.0f,
        0,
        0,
        0,
        0,
    };

    while(rx_byte != VN_TERMINATOR){
        if(HAL_UART_Receive(huart, &rx_byte, 1, 10) != HAL_OK){
            return false;
        }
    }

    while(vn_size < VN_messageSize){
        if(HAL_UART_Receive(&huart, &rx_byte, 1, 10) != HAL_OK){
            vn_size++;
        } else {
            return false;
        }
    }

    if(vn_size != VN_messageSize){
        return false;
    }

    for (int i = 0; i < 8; ++i) {
        v8.b[i] = buffer[i + 4];
    }
    vnData->time = v8.f; // Update structure field directly [cite: 6]

    // 4. Reconstruct 4-byte float chunks into structural elements
    for (int pos = 12; pos < vn_size; pos += 4)
    {
        for (int j = 0; j < 4; ++j) {
            v4.b[j] = buffer[pos + j]; // [cite: 7]
        }

        switch (pos)
        {
            case 12:  vnData->yaw         = v4.d; break; // [cite: 8]
            case 16:  vnData->pitch       = v4.d; break; // [cite: 9]
            case 20:  vnData->roll        = v4.d; break; // [cite: 10]
            case 24:  vnData->angRateX    = v4.d; break; // [cite: 11]
            case 28:  vnData->angRateY    = v4.d; break; // [cite: 12]
            case 32:  vnData->angRateZ    = v4.d; break; // [cite: 13]
            case 36:  vnData->accelX      = v4.d; break; // [cite: 14]
            case 40:  vnData->accelY      = v4.d; break; // [cite: 15]
            case 44:  vnData->accelZ      = v4.d; break; // [cite: 16]
            case 48:  vnData->magX        = v4.d; break; // [cite: 17]
            case 52:  vnData->magY        = v4.d; break; // [cite: 18]
            case 56:  vnData->magZ        = v4.d; break; // [cite: 19]
            case 60:  vnData->temperature = v4.d; break; // [cite: 20]
            case 64:  
                vnData->pressure   = v4.d; 
                // If you have a custom get_altitude function, calculate it here:
                // vnData->altitude = get_altitude(vnData->pressure); 
                break; // [cite: 21]
            default:  
                break;
        }
    }

    return true; // Fresh data safely stored in your target handler struct

}