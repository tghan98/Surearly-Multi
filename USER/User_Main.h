/**
  ******************************************************************************
  * @file           : User_Main.h
  * @brief          : Header for User_Main.c
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USER_MAIN_H__
#define __USER_MAIN_H__

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported Functions --------------------------------------------------------*/

/**
  * @brief  Returns the current system tick count (10ms unit).
  * @param  None
  * @retval Current system tick value.
  */
uint32_t GetSystemTick(void);

/**
  * @brief  Accumulates a known, deterministic block of real elapsed time (e.g.
  *         ADC conversion time, which never goes through
  *         DM_HW_Drv_SystemSleep_10ms() and so is otherwise invisible to
  *         GetSystemTick()), so elapsed-time checks based on GetSystemTick()
  *         stay accurate without needing any hardware timer to run
  *         continuously (TIM4 keeps its existing enable-only-while-sleeping
  *         behavior).
  * @note   Foreground-only: only ever called from normal (non-ISR) code, so it
  *         never writes the ISR-owned system tick counter directly - no
  *         critical section needed, no race with the TIM4 ISR. Folds whole
  *         10ms units in immediately via a cheap subtract loop (not division -
  *         STM8 has no hardware support for 32-bit division, and this used to
  *         run inside GetSystemTick() on every call, costing ~33-35s of extra
  *         real time over a 5-minute wait; moved here since this runs far less
  *         often, once per DM_HW_Drv_ADC_Read() call).
  * @param  wMicroseconds: Known elapsed time to add, in microseconds.
  * @retval None
  */
void SystemTick_AddUntracked_us(uint16_t wMicroseconds);

/**
  * @brief  User-level initialization. Called once from main.c.
  * @retval 0: Success, Others: Error code
  */
int32_t User_Main_Init(void);

/**
  * @brief  User-level main loop. Called repeatedly from main.c.
  * @retval 0: Success, Others: Error code
  */
int32_t User_Main_Run(void);

#endif /* __USER_MAIN_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
