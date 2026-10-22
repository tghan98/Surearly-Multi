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

#define ROM_RESULT_SLOT_COUNT       6
#define ROM_QC_REFERENCE_WORDS      4

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

/** Reads the four-channel QC reference without normalising erased 0xFFFF. */
void DM_App_Rom_Get_QcReference(uint16_t* pBuffer);

/** Stores one complete four-channel QC reference set. */
void DM_App_Rom_Set_QcReference(const uint16_t* pBuffer);

/**
  * @brief  Reads the wake-gate fail reading range (min, max) from EEPROM.
  * @note   Normalises the factory-fresh state (both 0xFFFF) on read: max is set
  *         to 0 and written back, otherwise it could never update.
  * @param  pMin: Receives the lowest reading seen. 0xFFFF means none recorded.
  * @param  pMax: Receives the highest reading seen. 0 means none recorded.
  * @retval None
  */
void DM_App_Rom_Get_WakeFail(uint16_t* pMin, uint16_t* pMax);

/**
  * @brief  Widens the stored wake-gate fail range with one new reading.
  * @note   Writes only when the range actually widens.
  * @param  wADC: Raw CH3 reading taken when the post-wake-up gate failed.
  * @retval None
  */
void DM_App_Rom_Update_WakeFail(uint16_t wADC);

/**
  * @brief  Reads 16 result data (short) from a specific channel.
  * @param  bChannel: Channel index (0 ~ 5).
  * @param  pBuffer: Pointer to uint16_t array (size 16) to store data.
  * @retval None
  */
void DM_App_Rom_Get_ResultData(uint8_t bChannel, uint16_t* pBuffer);

/**
  * @brief  Saves 16 result words to the next slot in the six-slot ring.
  * @note   Ring position is independent of Insert Count.
  * @param  pBuffer: Pointer to uint16_t array (size 16) containing result to write.
  * @retval None
  */
void DM_App_Rom_Save_NextResult(const uint16_t* pBuffer);

/**
  * @brief  Reads the slot index of the most recently saved result.
  * @note   Valid range 0 ~ 5. Returns 0 if unwritten (0xFF) or out of range.
  * @param  None
  * @retval uint8_t: Slot index (0 ~ 6).
  */
uint8_t DM_App_Rom_Get_DumpPos(void);

/**
  * @brief  Writes the slot index of the most recently saved result.
  * @param  bPos: Slot index (0 ~ 6).
  * @retval None
  */
void DM_App_Rom_Set_DumpPos(uint8_t bPos);

/** Restores the complete 0~255 data EEPROM map to its initial values. */
void DM_App_Rom_Reset_AllData(void);

#endif /* __DM_ROM_HAND_APP_H__ */
