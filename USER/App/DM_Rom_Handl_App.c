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
#define ROM_OFFS_RESULT_DATA            20  /**< Offset for Results (6 slots * 32 Bytes = 192 Bytes) */
#define ROM_OFFS_QC_REFERENCE           212 /**< QC reference (4 channels * 2 Bytes) */

#define RESULT_DATA_SIZE                32  /**< size of 16 shorts in bytes */

/* 최근 결과가 저장된 칸 번호(0~6). 덤프를 최신순으로 내보내기 위한 시작점.
   삽입 횟수에서 계산해도 되지만, InsertCount는 1바이트라 255에서 0으로
   초기화되므로 그 시점에 최신 위치를 잘못 짚는다. 별도로 적어둔다. (2026-10-19) */
#define ROM_OFFS_DUMP_POS               244 /**< Offset for latest result slot (1 Byte) */

/* 깨운 직후 광학 관문(DM_Main_Sq_App.c)을 통과하지 못한 회차의 CH3 원본값 범위.
   248 부터 시작하는 이유 - STM8L 의 EEPROM 은 1바이트만 써도 물리적으로 4바이트
   블록이 함께 소모된다. 245 에서 시작하면 244번(ROM_OFFS_DUMP_POS)과 같은 블록
   (244~247)을 공유하여, 이 값을 갱신할 때마다 덤프 위치 칸도 함께 닳는다.
   덤프 위치가 망가지면 레코드가 최신순으로 나오지 않는다. (2026-10-21) */
#define ROM_OFFS_WAKE_FAIL_MIN          248 /**< Offset for wake-gate fail min (2 Byte) */
#define ROM_OFFS_WAKE_FAIL_MAX          250 /**< Offset for wake-gate fail max (2 Byte) */
#define ROM_OFFS_RESERVED_TAIL          252 /**< Reserved final 4-byte block */

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

void DM_App_Rom_Get_QcReference(uint16_t* pBuffer)
{
    DM_HW_Drv_EEPROM_Read(ROM_OFFS_QC_REFERENCE, (uint8_t*)pBuffer,
                          ROM_QC_REFERENCE_WORDS * 2);
}

void DM_App_Rom_Set_QcReference(const uint16_t* pBuffer)
{
    DM_HW_Drv_EEPROM_Write(ROM_OFFS_QC_REFERENCE, (uint8_t*)pBuffer,
                           ROM_QC_REFERENCE_WORDS * 2);
}

/**
  * @brief  Reads 16 result data (short) from a specific channel.
  * @param  bChannel: Channel index (0 ~ 6).
  * @param  pBuffer: Pointer to uint16_t array (size 16).
  * @retval None
  */
void DM_App_Rom_Get_ResultData(uint8_t bChannel, uint16_t* pBuffer)
{
    static uint8_t i;
    uint16_t wAddr;
    
    if (bChannel >= ROM_RESULT_SLOT_COUNT)
    {
        for (i = 0; i < 16; i++)
        {
            pBuffer[i] = 0;
        }
        return;
    }
    
    wAddr = ROM_OFFS_RESULT_DATA + (uint16_t)(bChannel * RESULT_DATA_SIZE);
    DM_HW_Drv_EEPROM_Read(wAddr, (uint8_t*)pBuffer, RESULT_DATA_SIZE);
}

/**
  * @brief  Reads the slot index of the most recently saved result.
  * @note   유효 범위는 0 ~ 6. 미기록(0xFF)이거나 범위를 벗어나면 0으로 본다.
  * @param  None
  * @retval uint8_t: Slot index (0 ~ 6).
  */
uint8_t DM_App_Rom_Get_DumpPos(void)
{
    uint8_t bData = 0;
    DM_HW_Drv_EEPROM_Read(ROM_OFFS_DUMP_POS, &bData, 1);

    /* Initial state / out-of-range check.
       구형 펌웨어로 이미 결과를 쌓아둔 기기는 이 자리가 0xFF로 비어 있다.
       그때만 삽입 횟수에서 역산한다. 한 번 저장하고 나면 적어둔 값을 쓴다. */
    if (bData >= ROM_RESULT_SLOT_COUNT)
    {
        bData = 0;
    }
    return bData;
}

/**
  * @brief  Writes the slot index of the most recently saved result.
  * @param  bPos: Slot index (0 ~ 6).
  * @retval None
  */
void DM_App_Rom_Set_DumpPos(uint8_t bPos)
{
    DM_HW_Drv_EEPROM_Write(ROM_OFFS_DUMP_POS, &bPos, 1);
}

/**
  * @brief  Reads the wake-gate fail reading range (min, max) from EEPROM.
  * @note   Factory-fresh EEPROM reads 0xFFFF everywhere. That is already the
  *         value min must start from, but max must start from 0 - otherwise
  *         (reading > max) is false forever and max never updates.
  *         So the factory state is normalised here, on read, and written back,
  *         the same way DM_App_Rom_Get_StickPtrData() does it.
  * @param  pMin: Receives the lowest reading seen. 0xFFFF means none recorded.
  * @param  pMax: Receives the highest reading seen. 0 means none recorded.
  * @retval None
  */
void DM_App_Rom_Get_WakeFail(uint16_t* pMin, uint16_t* pMax)
{
    uint16_t wMin = 0;
    uint16_t wMax = 0;

    DM_HW_Drv_EEPROM_Read(ROM_OFFS_WAKE_FAIL_MIN, (uint8_t*)&wMin, 2);
    DM_HW_Drv_EEPROM_Read(ROM_OFFS_WAKE_FAIL_MAX, (uint8_t*)&wMax, 2);

    /* Guard-rail: Initial state check (0xFFFF) */
    if ((wMin == 0xFFFF) && (wMax == 0xFFFF))
    {
        wMax = 0;
        DM_HW_Drv_EEPROM_Write(ROM_OFFS_WAKE_FAIL_MAX, (uint8_t*)&wMax, 2);
    }

    *pMin = wMin;
    *pMax = wMax;
}

/**
  * @brief  Widens the stored wake-gate fail range with one new reading.
  * @note   Writes only when the range actually widens. A run of identical
  *         readings therefore costs one write, not one per wake-up - which is
  *         why a reading range is stored here instead of an occurrence count.
  * @param  wADC: Raw CH3 reading taken when the post-wake-up gate failed.
  * @retval None
  */
void DM_App_Rom_Update_WakeFail(uint16_t wADC)
{
    uint16_t wMin = 0;
    uint16_t wMax = 0;

    DM_App_Rom_Get_WakeFail(&wMin, &wMax);

    if (wADC < wMin)
    {
        DM_HW_Drv_EEPROM_Write(ROM_OFFS_WAKE_FAIL_MIN, (uint8_t*)&wADC, 2);
    }

    if (wADC > wMax)
    {
        DM_HW_Drv_EEPROM_Write(ROM_OFFS_WAKE_FAIL_MAX, (uint8_t*)&wADC, 2);
    }
}

/**
  * @brief  Saves 16 result words to the next slot in the six-slot ring.
  * @note   DumpPos is the latest saved slot. A reset value of 0 therefore makes
  *         slot 1 the first physical write; this does not affect latest-first
  *         dump ordering and all six slots participate after one full turn.
  * @param  pBuffer: Pointer to uint16_t array (size 16) containing result.
  * @retval None
  */
void DM_App_Rom_Save_NextResult(const uint16_t* pBuffer)
{
    uint8_t bChannel;
    uint16_t wAddr;

    bChannel = (uint8_t)((DM_App_Rom_Get_DumpPos() + 1) % ROM_RESULT_SLOT_COUNT);
    
    wAddr = ROM_OFFS_RESULT_DATA + (uint16_t)(bChannel * RESULT_DATA_SIZE);
    
    DM_HW_Drv_EEPROM_Write(wAddr, (uint8_t*)pBuffer, RESULT_DATA_SIZE);

    /* 방금 쓴 칸 번호를 남긴다. 덤프를 최신순으로 내보낼 때의 시작점이 된다.
       호출처(442/749/781행)가 아니라 여기서 적는 이유는, 저장 위치를 계산하는
       자리가 여기 하나뿐이라 두 값이 어긋날 틈이 없기 때문이다. */
    DM_App_Rom_Set_DumpPos(bChannel);
}

void DM_App_Rom_Reset_AllData(void)
{
    uint8_t abZero[4] = {0, 0, 0, 0};
    uint8_t abErased[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t abDumpBlock[4] = {0, 0xFF, 0xFF, 0xFF};
    uint16_t wAddr;

    /* 0~211: counters, status, StickPtrData and six result slots. */
    for (wAddr = 0; wAddr < ROM_OFFS_QC_REFERENCE; wAddr += 4)
    {
        DM_HW_Drv_EEPROM_Write(wAddr, abZero, 4);
    }

    /* 212~243: blank QC reference plus the retired tail of old slot 7. */
    for (wAddr = ROM_OFFS_QC_REFERENCE; wAddr < ROM_OFFS_DUMP_POS; wAddr += 4)
    {
        DM_HW_Drv_EEPROM_Write(wAddr, abErased, 4);
    }

    /* 244: latest slot reset to 0. Keep the rest erased, including WakeFail. */
    DM_HW_Drv_EEPROM_Write(ROM_OFFS_DUMP_POS, abDumpBlock, 4);
    DM_HW_Drv_EEPROM_Write(ROM_OFFS_WAKE_FAIL_MIN, abErased, 4);
    DM_HW_Drv_EEPROM_Write(ROM_OFFS_RESERVED_TAIL, abErased, 4);
}

/************************ (C) COPYRIGHT Surearly Multi *****END OF FILE****/
