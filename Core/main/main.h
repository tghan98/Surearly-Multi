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
/* v1.1 board: LED anode is hard-wired to VCC via R13(510R); cathode = nLEDx on
   MCU. Drive Push-Pull, Low = ON (sink), High = OFF. No LED_PWM pin (PB2 is now
   LCD_COM0). */
//GPIO
#define LED1_GPIO_PORT          GPIOB   /* nLED1 cathode (Push-Pull) */
#define LED2_GPIO_PORT          GPIOB   /* nLED2 cathode (Push-Pull) */

//Pin
#define LED1_GPIO_PIN           GPIO_Pin_4
#define LED2_GPIO_PIN           GPIO_Pin_3

/* Custom LCD */
//GPIO
#define LCD_COM0_GPIO_PORT      GPIOB   /* v1.1 board: LCD COM on PB2 (Push-Pull AC drive) */

#define LCD_SEG0_GPIO_PORT	GPIOC
#define LCD_SEG1_GPIO_PORT	GPIOC
#define LCD_SEG2_GPIO_PORT	GPIOD
/* 물방울(DROP) = PC4.
     · v1.1 회로도(r1574)는 PB0으로 그려져 있으나, 실물은 PC4다.
     · 근거 1: 구형 펌웨어 BSP/BSP_HW_Config.h → LCDSEG3 = GPIOC / Pin_4
     · 근거 2: TM 테스트 패드(= MCU 20번 핀 = PC4) 실측 1.25V 토글 (2026-10-13)
     · 근거 3: 회수 기기 13대 2사이클 측정 — 화면이 뜨는 기기는 예외 없이
               PC4에서 물방울이 점등했다. PB0 보드는 한 대도 없었다. (2026-10-14)
     회로도와 코드가 어긋나는 상태이므로, 회로도 정정 여부는 회로 담당과 확인할 것. */
#define LCD_SEG3_GPIO_PORT	GPIOC

//Pin
#define LCD_COM0_GPIO_PIN       GPIO_Pin_2

#define LCD_SEG0_GPIO_PIN	GPIO_Pin_5
#define LCD_SEG1_GPIO_PIN	GPIO_Pin_6
#define LCD_SEG2_GPIO_PIN       GPIO_Pin_0
#define LCD_SEG3_GPIO_PIN	GPIO_Pin_4	/* PC4 — 위 LCD_SEG3_GPIO_PORT 주석 참조 */

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

/* PTR Power: removed on v1.1 board (PTR collector wired directly to VCC, no gating). */

/* EEPROM */
#define EEPROM_START_ADDR       0x001000

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
