/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "stm8l15x.h"
#include "stm8l15x_it.h"

  
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

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN Private defines */
//HW IO Define 

/* LED  */
//GPIO
#define LED_PWM_GPIO_PORT       GPIOB
#define LED1_GPIO_PORT          GPIOC   /* nLED1: True Open-Drain pin (sink only) */
#define LED2_GPIO_PORT          GPIOC   /* nLED2: True Open-Drain pin (sink only) */

//Pin
#define LED_PWM_GPIO_PIN        GPIO_Pin_2
#define LED1_GPIO_PIN           GPIO_Pin_0
#define LED2_GPIO_PIN           GPIO_Pin_1

/* Custom LCD */
//GPIO
#define LCD_COM0_GPIO_PORT      GPIOB   /* Moved from PC0 (True OD) to PB4 (Push-Pull) for AC drive */

#define LCD_SEG0_GPIO_PORT	GPIOC
#define LCD_SEG1_GPIO_PORT	GPIOC
#define LCD_SEG2_GPIO_PORT	GPIOD
#define LCD_SEG3_GPIO_PORT	GPIOB

//Pin
#define LCD_COM0_GPIO_PIN       GPIO_Pin_4

#define LCD_SEG0_GPIO_PIN	GPIO_Pin_5
#define LCD_SEG1_GPIO_PIN	GPIO_Pin_6
#define LCD_SEG2_GPIO_PIN       GPIO_Pin_0
#define LCD_SEG3_GPIO_PIN	GPIO_Pin_0

// LCD Matching --> This Define Use LCD Control
#define LCD_BOOK_PORT           LCD_SEG0_GPIO_PORT
#define LCD_DROP_PORT           LCD_SEG3_GPIO_PORT
#define LCD_YES_PORT            LCD_SEG2_GPIO_PORT
#define LCD_NO_PORT             LCD_SEG1_GPIO_PORT

#define LCD_BOOK_PIN            LCD_SEG0_GPIO_PIN
#define LCD_DROP_PIN            LCD_SEG3_GPIO_PIN
#define LCD_YES_PIN             LCD_SEG2_GPIO_PIN
#define LCD_NO_PIN              LCD_SEG1_GPIO_PIN

//#define LCD_COM_PORT            LCD_COM0_GPIO_PORT
//#define LCD_COM_PIN             LCD_COM0_GPIO_PIN

/* COM */
//USART
#define USART_PORT		GPIOA
#define USART_TX_PIN		GPIO_Pin_2
#define USART_RX_PIN		GPIO_Pin_3

//ADC
#define ADC_FRNT_PORT           GPIOB
#define ADC_MIDL_PORT           GPIOB
#define ADC_REAR_PORT           GPIOB

#define ADC_FRNT_PIN            GPIO_Pin_5
#define ADC_MIDL_PIN            GPIO_Pin_6
#define ADC_REAR_PIN            GPIO_Pin_7

#define ADC_IDD_FRNT_CHANNEL    ADC_Channel_13	// Q1,F
#define ADC_IDD_MIDL_CHANNEL    ADC_Channel_12	// Q2,M
#define ADC_IDD_REAR_CHANNEL	ADC_Channel_11	// Q3,R

//Strip Check
#define STP_CK_PORT        GPIOB
#define STP_CK_PIN         GPIO_Pin_1

/* PTR Power */
#define PTR_PW_PORT        GPIOB   /* Moved from PC1 (True OD) to PB3 (Push-Pull) */
#define PTR_PW_GPIO        GPIO_Pin_3

/* EEPROM */
#define EEPROM_START_ADDR       0x001000

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
