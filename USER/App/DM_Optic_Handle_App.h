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

/* Exported Functions --------------------------------------------------------*/

/**
  * @brief  Measures one optical channel at 100% LED drive.
  * @note   Returns the raw (pre-gain) reading: LED on, 20ms settle, discard the
  *         first conversion, then 20 samples sorted and averaged over the
  *         middle 16. Apply Cal_Ratio(result, gain) where a tuned value is
  *         wanted; range and stick-presence checks use this raw value.
  * @param  tCh: Optical channel selection (0 to MAX-1).
  * @retval uint16_t: 0~4095 raw reading, or 0 if an ADC read timed out.
  */
uint16_t DM_App_Optic_Measure(OPTIC_CH_t tCh);

/**
  * @brief  Returns the tuned gain (integer percent) for a specific optical channel.
  * @param  tCh: Optical channel selection.
  * @retval uint16_t: Current gain, 0~100.
  */
uint16_t DM_App_Optic_GetGain(OPTIC_CH_t tCh);

/**
  * @brief  Resets all channels' gain values to 0.
  * @note   Called at the start of a new stick-insert cycle so a stale gain
  *         from the previous cycle isn't saved to ROM if this cycle fails
  *         before tuning runs.
  * @param  None
  * @retval None
  */
void DM_App_Optic_ResetGain(void);

/**
  * @brief  Measures initial PTR values for all 4 optic channels at 100% drive.
  * @note   Checks stick insertion status during measurement. Each channel's raw
  *         reading is validated against OPTIC_INITIAL_MIN/MAX_100.
  * @param  pBuffer: Pointer to uint16_t array (size 8); entries 0~3 receive the
  *         per-channel readings and 4~7 are zero-filled to keep the EEPROM
  *         layout unchanged.
  * @retval uint8_t: OPTIC_SUCCESS, OPTIC_ERR_STICK_REMOVED, or FAIL_OPTIC_CH0..CH3
  *         if a channel's reading is out of the expected range.
  */
uint8_t DM_App_Optic_MeasureInitialStick(uint16_t* pBuffer);

/**
  * @brief  Computes each channel's gain from the initial readings.
  * @note   Reuses pInitBuffer instead of re-measuring; the values were taken
  *         moments earlier under identical conditions.
  * @param  pInitBuffer: Pointer to the initial measurement buffer.
  * @param  wTargetADC: The target ADC value to reach (0-4095).
  * @retval uint8_t: OPTIC_SUCCESS or FAIL_OPTIC_CH0..CH3.
  */
uint8_t DM_App_Optic_TuneTargetADC(uint16_t* pInitBuffer, uint16_t wTargetADC);

/**
  * @brief  Derives the Empty PTR values from the initial readings and the tuned gain.
  * @note   No measurement here: gain mode keeps the LED at 100%, so the initial
  *         reading, taken under the same conditions, scaled by gain is the Empty
  *         value. Requires DM_App_Optic_TuneTargetADC() to have run first.
  * @param  pInitBuffer: Initial readings from DM_App_Optic_MeasureInitialStick().
  * @param  pBuffer: Pointer to uint16_t array (size 4) to store results.
  * @retval uint8_t: OPTIC_SUCCESS or FAIL_OPTIC_CH0..CH3 if a channel's gain is 0.
  */
uint8_t DM_App_Optic_CalcEmptyPTR(const uint16_t* pInitBuffer, uint16_t* pBuffer);

/**
  * @brief  Immediately measures OPTIC_CH_3 (innermost sensor, LED1) at 100%
  *         drive and judges whether the stick is physically present.
  * @note   Always a fresh measurement (no caching/throttling) - intended for
  *         one-off checks such as post-wake-up confirmation. For use inside
  *         frequent wait loops, use DM_App_Stick_Get_Status() instead, which
  *         wraps this with a re-measure throttle.
  * @param  None
  * @retval uint8_t: 1 if stick present, 0 if removed.
  */
uint8_t DM_App_Optic_Check_StickPresent(void);

/**
  * @brief  Same as DM_App_Optic_Check_StickPresent(), but also hands back the
  *         raw CH3 reading it judged on.
  * @note   DM_App_Optic_Check_StickPresent() delegates to this one, so the
  *         threshold comparison lives in exactly one place.
  *         Added for the post-wake-up gate, which records the reading of a
  *         failed gate to EEPROM for later diagnosis. Measuring again outside
  *         would cost one more LED burst on a coin cell.
  * @param  pADC: Receives the raw (pre-gain) CH3 reading. May be NULL.
  * @retval uint8_t: 1 if stick present, 0 if removed.
  */
uint8_t DM_App_Optic_Check_StickPresent_WithValue(uint16_t* pADC);

/**
  * @brief  DM_App_Optic_Check_StickPresent()의 배율 적용판.
  * @note   배율이 계산된 뒤에만 부를 것. 배율이 0이면 항상 "빠짐"으로 나온다.
  *         대기 루프처럼 시료가 들어온 뒤에도 판정해야 하는 구간에서 쓴다.
  * @param  None
  * @retval uint8_t: 1 if stick present, 0 if removed.
  */
uint8_t DM_App_Optic_Check_StickPresent_Scaled(void);

void LED_ON_Test(void);


#endif /* __DM_OPTIC_HANDLE_APP_H__ */

/************************ (C) COPYRIGHT Surearly Multi *****END OF FILE****/
