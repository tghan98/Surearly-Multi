/**
  ******************************************************************************
  * @file           : DM_LCD_Stick_Check_App.h
  * @brief          : LCD Segment and COM Control Application
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DM_LCD_STICK_CHECK_APP_H__
#define __DM_LCD_STICK_CHECK_APP_H__

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported Types ------------------------------------------------------------*/

/**
  * @brief LCD Channel Enumeration for specific icons
  */
typedef enum
{
    CH_LCD_BOOK = 0,
    CH_LCD_DROP,
    CH_LCD_YES,
    CH_LCD_NO,
    
    CH_LCD_MAX,
    CH_LCD_ALL
} LCD_CH_t;

/**
  * @brief LCD State Enumeration
  */
typedef enum
{
    LCD_STATE_OFF = 0,
    LCD_STATE_ON,
    LCD_STATE_BLINK
} LCD_STATE_t;

/**
  * @brief Stick Insertion Status Enumeration
  */
typedef enum
{
    STICK_STATUS_REMOVED = 0,
    STICK_STATUS_INSERTED
} STICK_STATUS_t;

/**
  * @brief LCD Sequence Step Enumeration
  */
typedef enum
{
    LCD_SQ_IDLE = 0,
    LCD_SQ_STICK_INSERT,
    LCD_SQ_SAMPLE_LOAD_WAIT,
    LCD_SQ_OVER_SAMPLE_CHECK,
    LCD_SQ_LOW_SAMPLE_CHECK,
    LCD_SQ_REACTION_WAIT,
    LCD_SQ_RESULT_SCAN,
    LCD_SQ_RESULT_YES,
    LCD_SQ_RESULT_NO,
    LCD_SQ_ERR_STICK_REMOVE,
    LCD_SQ_ERR_STICK_FAIL,
    LCD_SQ_ERR_OVER_USED,
    LCD_SQ_ERR_DEV_FAIL
} LCD_SQ_STEP_t;

/* Exported Functions --------------------------------------------------------*/

/**
  * @brief  Process LCD Animation/State Sequence.
  * @note   This function is called from the TIM4 interrupt (10ms).
  * @param  None
  * @retval None
  */
void DM_App_LCD_Sq_Process(void);

/**
  * @brief  Sets the LCD Sequence Step.
  * @param  tStep: Target step.
  * @retval None
  */
void DM_App_LCD_Sq_Set_Step(LCD_SQ_STEP_t tStep);

/**
  * @brief  Returns the current LCD Sequence Step.
  * @param  None
  * @retval LCD_SQ_STEP_t: Current step.
  */
LCD_SQ_STEP_t DM_App_LCD_Sq_Get_Step(void);

/**
  * @brief  Returns the current stick presence status.
  * @note   Determined via OPTIC_CH_3 (innermost sensor) ADC reading at Duty 100%,
  *         not the STP_CK switch (which is used only as the Halt wake-up trigger).
  *         Internally throttled to OPTIC_STICK_CHECK_INTERVAL_MS to avoid
  *         re-measuring (LED/PTR cycling) on every call.
  * @param  None
  * @retval Current stick status (INSERTED or REMOVED).
  */
STICK_STATUS_t DM_App_Stick_Get_Status(void);

/**
  * @brief  Sets the target state of a specific LCD channel.
  * @param  tCh: LCD channel (BOOK, DROP, YES, NO, or ALL).
  * @param  tState: Target state (OFF, ON, BLINK).
  * @retval None
  */
void DM_App_LCD_SetState(LCD_CH_t tCh, LCD_STATE_t tState);

/**
  * @brief  Refreshes the LCD pins based on COM phase and target states.
  * @note   This function MUST be called every 10ms in the TIM4 interrupt.
  * @param  None
  * @retval None
  */
void DM_App_LCD_Refresh(void);

/**
  * @brief  Controls a specific LCD segment (icon) - Low level.
  * @param  tCh: LCD channel (BOOK, DROP, YES, NO).
  * @param  tState: Pin state (SET or RESET).
  * @retval None
  */
void DM_App_LCD_SEG_Control(LCD_CH_t tCh, BitStatus tState);

/**
  * @brief  Controls the LCD COM pin.
  * @param  tState: Pin state (SET or RESET).
  * @retval None
  */
void DM_App_LCD_COM_Control(BitStatus tState);

#endif /* __DM_LCD_STICK_CHECK_APP_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
