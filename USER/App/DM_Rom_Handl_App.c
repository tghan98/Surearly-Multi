/**
  ******************************************************************************
  * @file           : DM_Rom_Handl_App.c
  * @brief          : EEPROM Data Handling Application Implementation
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

/* Includes ------------------------------------------------------------------*/
#include "DM_Rom_Handl_App.h"
#include "DM_HW_Drv.h"

/* Private Defines -----------------------------------------------------------*/
#define ROM_OFFS_INSERT_COUNT           0   /**< Offset for Stick Insert Count (1 Byte) */
#define ROM_OFFS_TEST_COMPLETE_COUNT    1   /**< Offset for Test Complete Count (1 Byte) */
#define ROM_OFFS_ERROR_STEP             2   /**< Offset for Error Step (1 Byte) */
#define ROM_OFFS_ERROR_CODE             3   /**< Offset for Error Code (1 Byte) */
#define ROM_OFFS_STICK_PTR_DATA         4   /**< Offset for Stick PTR Data Block (16 Bytes / 8 Shorts) */
#define ROM_OFFS_RESULT_DATA            20  /**< Offset for Results (7 Channels * 32 Bytes = 224 Bytes) */

#define RESULT_CHANNEL_MAX              7   /**< Maximum number of stored results */
#define RESULT_DATA_SIZE                32  /**< size of 16 shorts in bytes */

/* Functions -----------------------------------------------------------------*/

/**
  * @brief  Reads the stick insert count from EEPROM.
  * @param  None
  * @retval uint8_t: 1-byte insert count.
  */
uint8_t DM_App_Rom_Get_InsertCount(void)
{
    uint8_t bData = 0;
    DM_HW_Drv_EEPROM_Read(ROM_OFFS_INSERT_COUNT, &bData, 1);

    /* Initial state check */
    if (bData == 0xFF)
    {
        bData = 0;
        DM_App_Rom_Set_InsertCount(bData);
    }
    return bData;
}

/**
  * @brief  Writes the stick insert count to EEPROM.
  * @param  bCount: 1-byte value to write.
  * @retval None
  */
void DM_App_Rom_Set_InsertCount(uint8_t bCount)
{
    DM_HW_Drv_EEPROM_Write(ROM_OFFS_INSERT_COUNT, &bCount, 1);
}

/**
  * @brief  Reads the test complete count from EEPROM.
  * @param  None
  * @retval uint8_t: 1-byte complete count.
  */
uint8_t DM_App_Rom_Get_TestCompleteCount(void)
{
    uint8_t bData = 0;
    DM_HW_Drv_EEPROM_Read(ROM_OFFS_TEST_COMPLETE_COUNT, &bData, 1);

    /* Initial state check */
    if (bData == 0xFF)
    {
        bData = 0;
        DM_App_Rom_Set_TestCompleteCount(bData);
    }
    return bData;
}

/**
  * @brief  Writes the test complete count to EEPROM.
  * @param  bCount: 1-byte value to write.
  * @retval None
  */
void DM_App_Rom_Set_TestCompleteCount(uint8_t bCount)
{
    DM_HW_Drv_EEPROM_Write(ROM_OFFS_TEST_COMPLETE_COUNT, &bCount, 1);
}

/**
  * @brief  Reads the error step from EEPROM.
  * @param  None
  * @retval uint8_t: 1-byte error step.
  */
uint8_t DM_App_Rom_Get_ErrorStep(void)
{
    uint8_t bData = 0;
    DM_HW_Drv_EEPROM_Read(ROM_OFFS_ERROR_STEP, &bData, 1);

    /* Initial state check */
    if (bData == 0xFF)
    {
        bData = 0;
        DM_App_Rom_Set_ErrorStep(bData);
    }
    return bData;
}

/**
  * @brief  Writes the error step to EEPROM.
  * @param  bStep: 1-byte step value to write.
  * @retval None
  */
void DM_App_Rom_Set_ErrorStep(uint8_t bStep)
{
    DM_HW_Drv_EEPROM_Write(ROM_OFFS_ERROR_STEP, &bStep, 1);
}

/**
  * @brief  Reads the error code from EEPROM.
  * @param  None
  * @retval uint8_t: 1-byte error code.
  */
uint8_t DM_App_Rom_Get_ErrorCode(void)
{
    uint8_t bData = 0;
    DM_HW_Drv_EEPROM_Read(ROM_OFFS_ERROR_CODE, &bData, 1);

    /* Initial state check */
    if (bData == 0xFF)
    {
        bData = 0;
        DM_App_Rom_Set_ErrorCode(bData);
    }
    return bData;
}

/**
  * @brief  Writes the error code to EEPROM.
  * @param  bCode: 1-byte code value to write.
  * @retval None
  */
void DM_App_Rom_Set_ErrorCode(uint8_t bCode)
{
    DM_HW_Drv_EEPROM_Write(ROM_OFFS_ERROR_CODE, &bCode, 1);
}

/**
  * @brief  Reads 8 stick PTR data (short) from EEPROM.
  * @param  pBuffer: Pointer to uint16_t array (size 8).
  * @retval None
  */
void DM_App_Rom_Get_StickPtrData(uint16_t* pBuffer)
{
    static uint8_t i;
    DM_HW_Drv_EEPROM_Read(ROM_OFFS_STICK_PTR_DATA, (uint8_t*)pBuffer, 16);
    
    /* Guard-rail: Initial state check (0xFFFF) */
    if (pBuffer[0] == 0xFFFF)
    {
        for (i = 0; i < 8; i++) 
        {
            pBuffer[i] = 0;
        }
        DM_App_Rom_Set_StickPtrData(pBuffer);
    }
}

/**
  * @brief  Writes 8 stick PTR data (short) to EEPROM.
  * @param  pBuffer: Pointer to uint16_t array (size 8).
  * @retval None
  */
void DM_App_Rom_Set_StickPtrData(const uint16_t* pBuffer)
{
    DM_HW_Drv_EEPROM_Write(ROM_OFFS_STICK_PTR_DATA, (uint8_t*)pBuffer, 16);
}

/**
  * @brief  Reads 16 result data (short) from a specific channel.
  * @param  bChannel: Channel index (0 ~ 6).
  * @param  pBuffer: Pointer to uint16_t array (size 16).
  * @retval None
  */
void DM_App_Rom_Get_ResultData(uint8_t bChannel, uint16_t* pBuffer)
{
    uint16_t wAddr;
    
    if (bChannel >= RESULT_CHANNEL_MAX) 
    {
        return;
    }
    
    wAddr = ROM_OFFS_RESULT_DATA + (uint16_t)(bChannel * RESULT_DATA_SIZE);
    DM_HW_Drv_EEPROM_Read(wAddr, (uint8_t*)pBuffer, RESULT_DATA_SIZE);
}

/**
  * @brief  Saves 16 result data (short) to the current channel.
  * @note   Channel index is wrap-around using (bIndex % 7).
  * @param  bIndex: Target channel index or current count.
  * @param  pBuffer: Pointer to uint16_t array (size 16) containing result.
  * @retval None
  */
void DM_App_Rom_Save_CurrentResult(uint8_t bIndex, const uint16_t* pBuffer)
{
    uint8_t bChannel;
    uint16_t wAddr;
    
    /* Calculate channel index (0 ~ 6) */
    bChannel = (uint8_t)(bIndex % RESULT_CHANNEL_MAX);
    
    wAddr = ROM_OFFS_RESULT_DATA + (uint16_t)(bChannel * RESULT_DATA_SIZE);
    
    DM_HW_Drv_EEPROM_Write(wAddr, (uint8_t*)pBuffer, RESULT_DATA_SIZE);
}

/************************ (C) COPYRIGHT Surearly Multi *****END OF FILE****/
