/**
  ******************************************************************************
  * @file           : DM_Rom_Handl_App.h
  * @brief          : EEPROM Data Handling Application Header
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

#ifndef __DM_ROM_HAND_APP_H__
#define __DM_ROM_HAND_APP_H__

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported Functions --------------------------------------------------------*/

/**
  * @brief  Reads the stick insert count from EEPROM.
  * @note   If the value is 0xFF (initial state), it initializes it to 0.
  * @param  None
  * @retval uint8_t: 1-byte insert count.
  */
uint8_t DM_App_Rom_Get_InsertCount(void);

/**
  * @brief  Writes the stick insert count to EEPROM.
  * @param  bCount: 1-byte value to write.
  * @retval None
  */
void DM_App_Rom_Set_InsertCount(uint8_t bCount);

/**
  * @brief  Reads the test complete count from EEPROM.
  * @note   If the value is 0xFF (initial state), it initializes it to 0.
  * @param  None
  * @retval uint8_t: 1-byte complete count.
  */
uint8_t DM_App_Rom_Get_TestCompleteCount(void);

/**
  * @brief  Writes the test complete count to EEPROM.
  * @param  bCount: 1-byte value to write.
  * @retval None
  */
void DM_App_Rom_Set_TestCompleteCount(uint8_t bCount);

/**
  * @brief  Reads the error step from EEPROM.
  * @note   If the value is 0xFF (initial state), it initializes it to 0.
  * @param  None
  * @retval uint8_t: 1-byte error step.
  */
uint8_t DM_App_Rom_Get_ErrorStep(void);

/**
  * @brief  Writes the error step to EEPROM.
  * @param  bStep: 1-byte step value to write.
  * @retval None
  */
void DM_App_Rom_Set_ErrorStep(uint8_t bStep);

/**
  * @brief  Reads the error code from EEPROM.
  * @note   If the value is 0xFF (initial state), it initializes it to 0.
  * @param  None
  * @retval uint8_t: 1-byte error code.
  */
uint8_t DM_App_Rom_Get_ErrorCode(void);

/**
  * @brief  Writes the error code to EEPROM.
  * @param  bCode: 1-byte code value to write.
  * @retval None
  */
void DM_App_Rom_Set_ErrorCode(uint8_t bCode);

/**
  * @brief  Reads 8 stick PTR data (short) from EEPROM.
  * @note   If uninitialized (0xFFFF), sets all to 0.
  * @param  pBuffer: Pointer to uint16_t array (size 8) to store data.
  * @retval None
  */
void DM_App_Rom_Get_StickPtrData(uint16_t* pBuffer);

/**
  * @brief  Writes 8 stick PTR data (short) to EEPROM.
  * @param  pBuffer: Pointer to uint16_t array (size 8) containing data to write.
  * @retval None
  */
void DM_App_Rom_Set_StickPtrData(const uint16_t* pBuffer);

/**
  * @brief  Reads 16 result data (short) from a specific channel.
  * @param  bChannel: Channel index (0 ~ 6).
  * @param  pBuffer: Pointer to uint16_t array (size 16) to store data.
  * @retval None
  */
void DM_App_Rom_Get_ResultData(uint8_t bChannel, uint16_t* pBuffer);

/**
  * @brief  Saves 16 result data (short) to the current channel.
  * @note   Channel index is wrap-around using (bIndex % 7).
  * @param  bIndex: Target channel index or current count.
  * @param  pBuffer: Pointer to uint16_t array (size 16) containing result to write.
  * @retval None
  */
void DM_App_Rom_Save_CurrentResult(uint8_t bIndex, const uint16_t* pBuffer);

#endif /* __DM_ROM_HAND_APP_H__ */
