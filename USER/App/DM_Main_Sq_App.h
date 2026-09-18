/**
  ******************************************************************************
  * @file           : DM_Main_Sq_App.h
  * @brief          : Main Sequence Control Application Header
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

#ifndef __DM_MAIN_SQ_APP_H__
#define __DM_MAIN_SQ_APP_H__

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported Constants --------------------------------------------------------*/

/**
  * @brief Error Runtime Definition (120 seconds * 100 per 10ms = 12000)
  */
#define ERR_RUN_TIME                12000

/**
  * @brief Sequence Timing Definitions (in 10ms units)
  */
#define SAMPLE_LOAD_WAIT_NOMINAL     30000  /**< 5 minutes (300 seconds), nominal target */
/* Historical note: repeated DM_App_Optic_Measure() calls inside this wait
   spend real time in DM_HW_Drv_ADC_Read()'s ADC conversion wait, which never
   went through DM_HW_Drv_SystemSleep_10ms() and so was invisible to
   GetSystemTick() - measured on real hardware as 5 real minutes taking ~5:37
   (37s over). Now fixed at the source: DM_HW_Drv_ADC_Read() explicitly folds
   its known ~200us conversion time into the tick base via
   SystemTick_AddUntracked_us(), so GetSystemTick() already accounts for it.
   This correction is 0 for that reason - do not reintroduce a flat offset
   here without first re-measuring, since doing both would double-correct. */
#define SAMPLE_LOAD_WAIT_CORRECTION  0      /**< No longer needed - see note above */
#define SAMPLE_LOAD_WAIT_TIME        (SAMPLE_LOAD_WAIT_NOMINAL - SAMPLE_LOAD_WAIT_CORRECTION)
#define OVER_SAMPLE_CHECK_TIME      500     /**< 5 seconds */
#define LOW_SAMPLE_CHECK_TIME       2000    /**< 20 seconds */
#define TOTAL_REACTION_TIME         18000   /**< 3 minutes (180 seconds) */
#define REACTION_WAIT_TIME          (TOTAL_REACTION_TIME - OVER_SAMPLE_CHECK_TIME - LOW_SAMPLE_CHECK_TIME)
#define RESULT_OUT_WAIT_TIME        30000   /**< 5 minutes (300 seconds) */

/* Exported Types ------------------------------------------------------------*/

/**
  * @brief Error Code Definitions for Main Sequence
  */
typedef enum
{
    ERROR_NONE = 0x00,          /**< No error */
    ERROR_STICK_REMOVE = 0x01,  /**< Stick removed before C-Band change in Sample Wait */
    ERROR_STICK_FAIL = 0x02,    /**< Stick removed after C-Band change or other fail */
    ERROR_OVER_FLOW = 0x03,     /**< Sample over-flow detected */
    ERROR_LOW_SAMPLE = 0x04,    /**< Insufficient sample volume detected */
    ERROR_OVER_USED = 0x05,     /**< Device used more than 30 times */
    ERROR_C_LINE_FAIL = 0x06,   /**< Control line intensity below threshold */
    ERROR_SAMPLE_LOAD_FAIL = 0x07, /**< No sample loading detected within 5 minutes */
    
    /* Optic Tuning Failures (Aligned with DM_Optic_Handle_App.h definitions) */
    ERROR_OPTIC_FAIL_CH0 = 0x10, /**< Channel 0 tuning failed */
    ERROR_OPTIC_FAIL_CH1 = 0x11, /**< Channel 1 tuning failed */
    ERROR_OPTIC_FAIL_CH2 = 0x12, /**< Channel 2 tuning failed */
    ERROR_OPTIC_FAIL_CH3 = 0x13  /**< Channel 3 tuning failed */
} MAIN_ERROR_CODE_t;

/**
  * @brief Band Type Enumeration for Optical Channels
  */
typedef enum
{
    BAND_T = 0,                 /**< T Band (Corresponding to OPTIC_CH_0) */
    BAND_BT,                    /**< BT Band (Blank T, Corresponding to OPTIC_CH_1) */
    BAND_BC,                    /**< BC Band (Blank C, Corresponding to OPTIC_CH_2) */
    BAND_C,                     /**< C Band (Corresponding to OPTIC_CH_3) */
    
    BAND_MAX
} BAND_TYPE_t;

/**
  * @brief Result Type Enumeration for Final Results Array
  */
typedef enum
{
    RES_T_INTENSITY = 0,         /**< Calculated T-Line Intensity */
    RES_C_INTENSITY,             /**< Calculated C-Line Intensity */
    RES_YES_NO,                  /**< Final Decision: 1 for YES, 0 for NO */
    
    RES_MAX
} RESULT_TYPE_t;

/**
  * @brief Main Sequence Step Enumeration
  */
typedef enum
{
    MAIN_SQ_IDLE = 0,           /**< MCU Halt Mode and Wakeup process */
    MAIN_SQ_STICK_INSERT,       /**< Stick status check and initial measurement */
    MAIN_SQ_SAMPLE_LOAD_WAIT,   /**< Waiting for sample loading (5 mins) */
    MAIN_SQ_OVER_SAMPLE_CHECK,  /**< Check for sample over-flow */
    MAIN_SQ_LOW_SAMPLE_CHECK,   /**< Check for insufficient sample */
    MAIN_SQ_REACTION_WAIT,      /**< Waiting for chemical reaction */
    MAIN_SQ_RESULT_SCAN,        /**< Final sensor scanning for results */
    MAIN_SQ_RESULT_OUT,         /**< Outputting YES/NO result */
    MAIN_SQ_ERROR_WAIT          /**< Error state and waiting for recovery */
} MAIN_SQ_STEP_t;

/* Exported Functions --------------------------------------------------------*/

/**
  * @brief  Process Main Sequence Logic.
  * @note   This function is called from the main loop in User_Main.c.
  * @param  None
  * @retval None
  */
void DM_App_Main_Sq_Process(void);

/**
  * @brief  Returns the current Main Sequence Step.
  * @param  None
  * @retval MAIN_SQ_STEP_t: Current main sequence step.
  */
MAIN_SQ_STEP_t DM_App_Main_Sq_Get_Step(void);

/**
  * @brief  Sets the Main Sequence Step.
  * @param  tStep: Target step to set.
  * @retval None
  */
void DM_App_Main_Sq_Set_Step(MAIN_SQ_STEP_t tStep);

/**
  * @brief  Returns the current Error Sequence.
  * @param  None
  * @retval uint8_t: Current error sequence.
  */
uint8_t DM_App_Main_Sq_Get_ErrorSequence(void);

/**
  * @brief  Sets the Error Sequence.
  * @param  bErrorSq: Target error sequence to set.
  * @retval None
  */
void DM_App_Main_Sq_Set_ErrorSequence(uint8_t bErrorSq);

/**
  * @brief  Packs current measurement and result data into 32 bytes and saves to ROM.
  * @param  bInsertCount: Current stick insert count.
  * @retval None
  */
void DM_App_Main_Sq_Save_Result_To_Rom(uint8_t bInsertCount);

/**
  * @brief  Sends stored EEPROM data as raw uint16_t words via USART (no ASCII/CSV conversion).
  * @note   Word order: InsertCnt, TestCompCnt, ErrStep, ErrCode (4 words),
  *         Initial Stick PTR data (8 words), then 7 x 16-word Result Records.
  *         Each word is sent High-byte-first.
  * @param  None
  * @retval None
  */
void DM_App_Main_Sq_Send_RawDump(void);

/**
  * @brief  Resets the test complete count to 0 (EEPROM + in-RAM cache),
  *         clearing the ERROR_OVER_USED (_nSCAN_MAX_CNT) usage limit.
  * @note   Triggered by the 'R'/'r' + Enter USART command (service reset).
  * @param  None
  * @retval None
  */
void DM_App_Main_Sq_Reset_TestCompleteCount(void);

#endif /* __DM_MAIN_SQ_APP_H__ */

/************************ (C) COPYRIGHT Surearly Multi *****END OF FILE****/
