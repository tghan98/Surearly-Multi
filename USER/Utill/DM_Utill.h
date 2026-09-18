/**
  ******************************************************************************
  * @file           : DM_Utill.h
  * @brief          : Utility Functions Header
  * @author         : Gemini CLI
  * @date           : 2026-04-29
  ******************************************************************************
  */

#ifndef __DM_UTILL_H__
#define __DM_UTILL_H__

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported Functions --------------------------------------------------------*/

/**
  * @brief  Calculates intensity with 4-digit fixed-point precision (multiplied by 1000).
  * @param  Divedend: The dividend value.
  * @param  Divisor: The divisor value.
  * @retval uint16_t: The calculated quotient.
  */
uint16_t CalculateIntensity(uint16_t Divedend, uint16_t Divisor);

/**
  * @brief  Calculates ratio: (Target * Constant) / 100.
  * @param  Ratio_Target: The target value.
  * @param  Ratio_Constant: The constant value (percentage).
  * @retval uint16_t: The calculated ratio.
  */
uint16_t Cal_Ratio(uint16_t Ratio_Target, uint16_t Ratio_Constant);

#endif /* __DM_UTILL_H__ */
