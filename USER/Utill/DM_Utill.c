/**
  ******************************************************************************
  * @file           : DM_Utill.c
  * @brief          : Utility Functions Implementation
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "DM_Utill.h"

/* Functions -----------------------------------------------------------------*/

/**
  * @brief  Calculates intensity with 4-digit fixed-point precision.
  * @param  Divedend: The dividend value.
  * @param  Divisor: The divisor value.
  * @retval uint16_t: The calculated quotient.
  */
uint16_t CalculateIntensity(uint16_t Divedend, uint16_t Divisor)
{
  uint16_t Quotient, tmp;
  static uint8_t nCnt;

  Quotient = 0;
  if(Divedend && Divisor){
    for(nCnt=0; nCnt<4; nCnt++){
      tmp = Divedend / Divisor;
      Quotient = Quotient * 10 + tmp;
      Divedend = Divedend - tmp * Divisor;

      Divedend = Divedend * 10;
    }
  }
  return Quotient;
}

/**
  * @brief  Calculates ratio: (Target * Constant) / 100.
  * @param  Ratio_Target: The target value.
  * @param  Ratio_Constant: The constant value (percentage).
  * @retval uint16_t: The calculated ratio.
  */
uint16_t Cal_Ratio(uint16_t Ratio_Target, uint16_t Ratio_Constant)
{
  uint32_t Temp;

  Temp = Ratio_Target;
  Temp  = (Temp * Ratio_Constant)/100;

  return (uint16_t)Temp;
}
