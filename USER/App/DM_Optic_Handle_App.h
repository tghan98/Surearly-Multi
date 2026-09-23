/**
  ******************************************************************************
  * @file           : DM_Optic_Handle_App.h
  * @brief          : Optical Sensor Handling Application Header
  * @author         : Gemini CLI
  * @date           : 2026-04-24
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 Surearly Multi.
  * All rights reserved.
  *
  ******************************************************************************
  */

#ifndef __DM_OPTIC_HANDLE_APP_H__
#define __DM_OPTIC_HANDLE_APP_H__

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported Types ------------------------------------------------------------*/

/**
  * @brief Optical Channel Enumeration
  */
typedef enum
{
    OPTIC_CH_0 = 0,             /**< LED2 + ADC Front */
    OPTIC_CH_1,                 /**< LED2 + ADC Middle */
    OPTIC_CH_2,                 /**< LED1 + ADC Middle */
    OPTIC_CH_3,                 /**< LED1 + ADC Rear */
    
    OPTIC_CH_MAX
} OPTIC_CH_t;

/* Exported Constants --------------------------------------------------------*/
#define OPTIC_SUCCESS               0   /**< Measurement completed successfully */
#define OPTIC_ERR_STICK_REMOVED      1   /**< Measurement failed due to stick removal */

#define FAIL_OPTIC_CH0              0x10 /**< Channel 0 tuning failed after max iterations */
#define FAIL_OPTIC_CH1              0x11 /**< Channel 1 tuning failed after max iterations */
#define FAIL_OPTIC_CH2              0x12 /**< Channel 2 tuning failed after max iterations */
#define FAIL_OPTIC_CH3              0x13 /**< Channel 3 tuning failed after max iterations */

extern volatile uint16_t g_awDiag_A_Single[OPTIC_CH_MAX];
extern volatile uint16_t g_awDiag_A_Sum14[OPTIC_CH_MAX];
extern volatile uint16_t g_awDiag_B_Sum14[OPTIC_CH_MAX];
extern volatile uint16_t g_awDiag_C_Sum14[OPTIC_CH_MAX][6];
extern volatile uint16_t g_awDiag_D_Sum14[OPTIC_CH_MAX][5];
extern volatile uint16_t g_awDiag_E_Sum14[OPTIC_CH_MAX][2];
extern volatile uint8_t g_bOpticDiag_Complete;

/* Exported Functions --------------------------------------------------------*/

/**
  * @brief  Measures ADC value for a specific optical channel set.
  * @note   LED on for wOnTime_us, then bursts 20 ADC samples, returns the sum of
  *         the middle 14 (after sort/trim).
  * @param  tCh: Optical channel selection (0 to MAX-1).
  * @param  wOnTime_us: LED on-time before the ADC burst (us).
  * @retval uint16_t: filtered ADC result.
  */
uint16_t DM_App_Optic_Measure(OPTIC_CH_t tCh, uint16_t wOnTime_us);

void DM_App_Optic_RunDiagnostic(void);

/**
  * @brief  Returns the current on-time (us) for a specific optical channel.
  * @param  tCh: Optical channel selection.
  * @retval uint16_t: Current on-time (us).
  */
uint16_t DM_App_Optic_GetOnTime(OPTIC_CH_t tCh);

/**
  * @brief  Resets all channels' on-time values to 0.
  * @param  None
  * @retval None
  */
void DM_App_Optic_ResetOnTime(void);

/**
  * @brief  Measures initial PTR values for all 4 optic channels at PWM 199 and 399.
  * @note   Checks stick insertion status during measurement. Total 8 measurements are
  *         stored. Each channel's readings are validated against
  *         OPTIC_INITIAL_MIN/MAX_50 and _100 (Parameter_define.h).
  * @param  pBuffer: Pointer to uint16_t array (size 8) to store measurements.
  * @retval uint8_t: OPTIC_SUCCESS, OPTIC_ERR_STICK_REMOVED, or FAIL_OPTIC_CH0..CH3
  *         if a channel's reading is out of the expected range.
  */
uint8_t DM_App_Optic_MeasureInitialStick(uint16_t* pBuffer);

/**
  * @brief  Tunes PWM values for all channels to reach a target ADC value within tolerance.
  * @note   Checks stick insertion status approximately every 100ms.
  * @param  pInitBuffer: Pointer to the 8 initial measurement values.
  * @param  wTargetADC: The target ADC value to reach (0-4095).
  * @retval uint8_t: OPTIC_SUCCESS or OPTIC_ERR_STICK_REMOVED.
  */
uint8_t DM_App_Optic_TuneTargetADC(uint16_t* pInitBuffer, uint16_t wTargetADC);

/**
  * @brief  Measures Empty PTR values for all 4 optic channels using the currently tuned PWM.
  * @note   Checks stick insertion status during measurement.
  * @param  pBuffer: Pointer to uint16_t array (size 4) to store measurements.
  * @retval uint8_t: OPTIC_SUCCESS or OPTIC_ERR_STICK_REMOVED.
  */
uint8_t DM_App_Optic_MeasureEmptyPTR(uint16_t* pBuffer);

/**
  * @brief  Immediately measures OPTIC_CH_3 (innermost sensor, LED1) at Duty 100%
  *         and judges whether the stick is physically present.
  * @note   Always a fresh measurement (no caching/throttling) - intended for
  *         one-off checks such as post-wake-up confirmation. For use inside
  *         frequent wait loops, use DM_App_Stick_Get_Status() instead, which
  *         wraps this with a re-measure throttle.
  * @param  None
  * @retval uint8_t: 1 if stick present, 0 if removed.
  */
uint8_t DM_App_Optic_Check_StickPresent(void);

void LED_ON_Test(void);


#endif /* __DM_OPTIC_HANDLE_APP_H__ */

/************************ (C) COPYRIGHT Surearly Multi *****END OF FILE****/
