/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define MAG_INT_Pin GPIO_PIN_13
#define MAG_INT_GPIO_Port GPIOC
#define IMU1_INT0_Pin GPIO_PIN_14
#define IMU1_INT0_GPIO_Port GPIOC
#define IMU1_INT1_Pin GPIO_PIN_15
#define IMU1_INT1_GPIO_Port GPIOC
#define GPS_INT_Pin GPIO_PIN_0
#define GPS_INT_GPIO_Port GPIOA
#define S_SENSING_3V_Pin GPIO_PIN_1
#define S_SENSING_3V_GPIO_Port GPIOA
#define GPS_TX_Pin GPIO_PIN_2
#define GPS_TX_GPIO_Port GPIOA
#define GPS_RX_Pin GPIO_PIN_3
#define GPS_RX_GPIO_Port GPIOA
#define ACT_ANTENNA_Pin GPIO_PIN_4
#define ACT_ANTENNA_GPIO_Port GPIOA
#define PADS2_INT1_Pin GPIO_PIN_5
#define PADS2_INT1_GPIO_Port GPIOA
#define GPS_RST_Pin GPIO_PIN_6
#define GPS_RST_GPIO_Port GPIOA
#define V_SENSING_3V_Pin GPIO_PIN_7
#define V_SENSING_3V_GPIO_Port GPIOA
#define TRANSDUCER_Pin GPIO_PIN_0
#define TRANSDUCER_GPIO_Port GPIOB
#define CAN1_STB_Pin GPIO_PIN_1
#define CAN1_STB_GPIO_Port GPIOB
#define CAN2_STB_Pin GPIO_PIN_2
#define CAN2_STB_GPIO_Port GPIOB
#define AIRBRAKE_Pin GPIO_PIN_10
#define AIRBRAKE_GPIO_Port GPIOB
#define V_SENSING_5V_Pin GPIO_PIN_11
#define V_SENSING_5V_GPIO_Port GPIOB
#define A_SENSING_5V_Pin GPIO_PIN_14
#define A_SENSING_5V_GPIO_Port GPIOB
#define VNAV_TARE_Pin GPIO_PIN_15
#define VNAV_TARE_GPIO_Port GPIOB
#define IMU2_INT0_Pin GPIO_PIN_10
#define IMU2_INT0_GPIO_Port GPIOA
#define VNAV_TX_Pin GPIO_PIN_3
#define VNAV_TX_GPIO_Port GPIOB
#define VNAV_RX_Pin GPIO_PIN_4
#define VNAV_RX_GPIO_Port GPIOB
#define IMU2_INT1_Pin GPIO_PIN_6
#define IMU2_INT1_GPIO_Port GPIOB
#define TEST_LED_Pin GPIO_PIN_9
#define TEST_LED_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
