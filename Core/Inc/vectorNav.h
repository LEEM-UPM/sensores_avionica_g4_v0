/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    vectorNav.h
  * @brief   This file contains all the function prototypes for
  *          the vectorNav.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __VECTORNAV_H__
#define __VECTORNAV_H__

#ifdef __cplusplus
extern "C" {
#endif
/* User defines --------------------------------------------------------------*/
#define VN_messageSize 72
#define VN_buffer[VN_messageSize];
#define VN_TERMINATOR 0xFA

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"
#include <stdbool.h>
typedef union v4{
  float f;
  uint8_t b[4];
}v8;

typedef union v4{
  double d;
  uint8_t b[8];
}v4;

typedef struct VN100_Data_t{
  double time;

  float yaw;
  float pitch; 
  float roll;

  float angRateX;
  float angRateY;
  float angRateZ;

  float accelX;
  float accelY;
  float accelZ;

  float magX;
  float magY;
  float magZ;

  float temperature;
  float pressure;
  float altitude;
}VN100_Data_t;

void VN_config(UART_HandleTypeDef *huart);
_Bool VN_read(UART_HandleTypeDef* huart, VN100_Data_t* vnData);
#ifdef __cplusplus
}
#endif

#endif /* __VECTORNAV_H__ */

