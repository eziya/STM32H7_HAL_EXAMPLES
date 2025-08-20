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
#include "stm32h7xx_hal.h"

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
#define LED1_Pin GPIO_PIN_12
#define LED1_GPIO_Port GPIOI
#define LED2_Pin GPIO_PIN_13
#define LED2_GPIO_Port GPIOI
#define LED3_Pin GPIO_PIN_14
#define LED3_GPIO_Port GPIOI
#define LED4_Pin GPIO_PIN_15
#define LED4_GPIO_Port GPIOI

/* USER CODE BEGIN Private defines */
#define QSPI_DUAL_MODE

#ifdef QSPI_DUAL_MODE
#define QSPI_FLASH_SIZE         (0x8000000) /* 128 MB (2 * 512Mbit) */
#define QSPI_SECTOR_SIZE        (0x20000)   /* 128 KB (2 * 64KB) */
#define QSPI_SUBSECTOR_SIZE     (0x2000)    /* 8 KB (2 * 4KB) */
#define QSPI_PAGE_SIZE          (512)       /* 512 Bytes (2 * 256B) */
#define QSPI_DUAL_FLASH_MODE    MT25QL512ABB_DUALFLASH_ENABLE
#else
#define QSPI_FLASH_SIZE         (0x4000000) /* 64 MB (1 * 512Mbit) */
#define QSPI_SECTOR_SIZE        (0x10000)   /* 64 KB */
#define QSPI_SUBSECTOR_SIZE     (0x1000)    /* 4 KB */
#define QSPI_PAGE_SIZE          (256)       /* 256 Bytes */
#define QSPI_DUAL_FLASH_MODE    MT25QL512ABB_DUALFLASH_DISABLE
#endif

#define QSPI_NUM_SECTORS        (QSPI_FLASH_SIZE / QSPI_SECTOR_SIZE)
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
