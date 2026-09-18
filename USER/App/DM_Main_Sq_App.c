/**
  ******************************************************************************
  * @file           : DM_Main_Sq_App.c
  * @brief          : Main Sequence Control Implementation
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "DM_Main_Sq_App.h"
#include "DM_LCD_Stick_Check_App.h"
#include "DM_Rom_Handl_App.h"
#include "DM_Optic_Handle_App.h"
#include "Parameter_define.h"
#include "DM_HW_Drv.h"
#include "DM_Utill.h"
#include "User_Main.h"

/* Private Variables ---------------------------------------------------------*/
static MAIN_SQ_STEP_t gs_tMain_Sq_Step = MAIN_SQ_IDLE;
static uint16_t gs_wMain_Sq_Timer = 0;

/* Global variables for measurement and tracking */
static uint8_t gs_bStickInsertCnt = 0;
static uint8_t gs_bTestCompleteCount = 0;
static uint8_t gs_bErrorCode = 0;
static uint8_t gs_bErrorSequence = 0;
static uint16_t gs_awEmptyBand_LowData[BAND_MAX];
static uint16_t gs_awResultBand_LowData[BAND_MAX];
static uint16_t gs_awInitialStickPtr[8];

/* Result Tracking */
static uint16_t gs_awResult[RES_MAX];

/* Private Function Prototypes -----------------------------------------------*/
static void DM_App_Main_Sq_Enter_Error(uint8_t bErrCode, LCD_SQ_STEP_t tLcdErrStep);
static void DM_App_Main_Sq_Idle_Step_Handler(void);
static void DM_App_Main_Sq_Stick_Insert_Step_Handler(void);
static void DM_App_Main_Sq_Sample_Load_Wait_Step_Handler(void);
static void DM_App_Main_Sq_Over_Sample_Check_Step_Handler(void);
static void DM_App_Main_Sq_Low_Sample_Check_Step_Handler(void);
static void DM_App_Main_Sq_Reaction_Wait_Step_Handler(void);
static void DM_App_Main_Sq_Result_Scan_Step_Handler(void);
static void DM_App_Main_Sq_Result_Out_Step_Handler(void);
static void DM_App_Main_Sq_Error_Wait_Step_Handler(void);

/* Functions -----------------------------------------------------------------*/

/**
  * @brief  Process Main Sequence Logic.
  * @note   This function is called from the main loop in User_Main.c.
  * @param  None
  * @retval None
  */
void DM_App_Main_Sq_Process(void)
{
    if (gs_wMain_Sq_Timer > 0)
    {
        /* 
           Note: This timer is in units of main loop iterations or 
           manually decremented in certain steps. 
           In this implementation, we use SystemSleep_10ms() to drive it.
        */
    }

    switch (gs_tMain_Sq_Step)
    {
        case MAIN_SQ_IDLE:
            /* MCU Halt Mode entry and Wakeup process */
            DM_App_Main_Sq_Idle_Step_Handler();
            break;
            
        case MAIN_SQ_STICK_INSERT:
            DM_App_Main_Sq_Stick_Insert_Step_Handler();
            break;
            
        case MAIN_SQ_SAMPLE_LOAD_WAIT:
            DM_App_Main_Sq_Sample_Load_Wait_Step_Handler();
            break;
            
        case MAIN_SQ_OVER_SAMPLE_CHECK:
            DM_App_Main_Sq_Over_Sample_Check_Step_Handler();
            break;
            
        case MAIN_SQ_LOW_SAMPLE_CHECK:
            DM_App_Main_Sq_Low_Sample_Check_Step_Handler();
            break;
            
        case MAIN_SQ_REACTION_WAIT:
            DM_App_Main_Sq_Reaction_Wait_Step_Handler();
            break;
            
        case MAIN_SQ_RESULT_SCAN:
            DM_App_Main_Sq_Result_Scan_Step_Handler();
            break;
            
        case MAIN_SQ_RESULT_OUT:
            DM_App_Main_Sq_Result_Out_Step_Handler();
            break;
            
        case MAIN_SQ_ERROR_WAIT:
            DM_App_Main_Sq_Error_Wait_Step_Handler();
            break;
            
        default:
            gs_tMain_Sq_Step = MAIN_SQ_IDLE;
            break;
    }
}

/**
  * @brief  Enters the ERROR_WAIT state: records the error code/step/sequence
  *         and switches the LCD to the matching error animation.
  * @param  bErrCode: Error code to store (MAIN_ERROR_CODE_t value).
  * @param  tLcdErrStep: LCD sequence step for the error animation.
  * @retval None
  */
static void DM_App_Main_Sq_Enter_Error(uint8_t bErrCode, LCD_SQ_STEP_t tLcdErrStep)
{
    gs_bErrorCode = bErrCode;
    gs_bErrorSequence = gs_tMain_Sq_Step;
    DM_App_LCD_Sq_Set_Step(tLcdErrStep);
    DM_App_Main_Sq_Set_Step(MAIN_SQ_ERROR_WAIT);
    gs_wMain_Sq_Timer = ERR_RUN_TIME;
}

/**
  * @brief  Sleeps for 250ms (25 x 10ms) while checking stick presence,
  *         entering an error state and returning 1 if the stick is removed.
  * @param  bErrCode: Error code to set if the stick is removed.
  * @param  tLcdErrStep: LCD sequence step to set if the stick is removed.
  * @retval 1 if stick removed (error entered), 0 otherwise.
  */
static uint8_t DM_App_Main_Sq_Sleep_And_CheckStick_250ms(uint8_t bErrCode, LCD_SQ_STEP_t tLcdErrStep)
{
    static uint8_t i;

    for (i = 0; i < 25; i++)
    {
        DM_HW_Drv_SystemSleep_10ms();
    }

    if (DM_App_Stick_Get_Status() != STICK_STATUS_INSERTED)
    {
        DM_App_Main_Sq_Enter_Error(bErrCode, tLcdErrStep);
        return 1;
    }

    return 0;
}

/**
  * @brief  Handles the IDLE step: Low power entry and wakeup recovery.
  * @param  None
  * @retval None
  */
static void DM_App_Main_Sq_Idle_Step_Handler(void)
{
    //uint8_t i;
  
    DM_App_LCD_SetState(CH_LCD_ALL, LCD_STATE_OFF);
    DM_HW_Drv_SystemSleep_10ms();

    /* 1. Prepare for Ultra-Low Power (Turn off LEDs, LCD, Peripherals) */
    DM_HW_Drv_Power_PrepareSleep();
    
    /* 2. Set LCD Sequence to IDLE */
    DM_App_LCD_Sq_Set_Step(LCD_SQ_IDLE);
    
    /* 3. Enter Halt Mode (Wakeup on STP_CK Falling Edge / Stick Insertion) */
    /* This function configures EXTI and calls halt() */
    DM_HW_Drv_SystemHalt_WakeupOnStick();
    
    /* --- MCU Wakes Up Here --- */
    
    /* 4. Restore MCU Clock and Essential Peripherals (ADC, USART) */
    DM_HW_Drv_Power_Resume();
    
    /* 5. Restore Stick Check Pin to standard Input (Disable Interrupt) */
    DM_HW_Drv_STP_CK_Set_GPIO_Input();
    
    /* 6. TIM4 is kept DISABLED here as requested. 
          Tick updates will be driven by DM_HW_Drv_SystemSleep_10ms(). */
    
    /* 7. Let ADC/clock settle for one tick after Power_Resume, then take a
       fresh stick-presence reading. Uses DM_App_Optic_Check_StickPresent()
       directly (not DM_App_Stick_Get_Status()), since that function's cache
       could still hold a stale pre-Halt value at this point. */
    DM_HW_Drv_SystemSleep_10ms();

    if (DM_App_Optic_Check_StickPresent())
    {
        /* Transition to Stick Insert Step */
        DM_App_LCD_Sq_Set_Step(LCD_SQ_STICK_INSERT);
        DM_App_Main_Sq_Set_Step(MAIN_SQ_STICK_INSERT);
    }
    else
    {
        /* Spurious wake or stick pulled back out immediately: stay in IDLE
           so the next call to DM_App_Main_Sq_Process() re-enters this
           handler and goes back to Halt mode. */
        DM_App_Main_Sq_Set_Step(MAIN_SQ_IDLE);
    }
}

/**
  * @brief  Handles the STICK_INSERT step.
  * @param  None
  * @retval None
  */
static void DM_App_Main_Sq_Stick_Insert_Step_Handler(void)
{
#if _DEBUG_LCD_SEQ_ONLY
    /* Test mode: skip EEPROM/optic work entirely. Wait for the STICK_INSERT
       LCD animation to finish on its own (~7.5s, 15 steps x 500ms) instead of
       using the fixed _DEBUG_SEQ_STEP_DELAY, so the animation isn't cut short. */
    while (DM_App_LCD_Sq_Get_Step() != LCD_SQ_IDLE)
    {
        DM_HW_Drv_SystemSleep_10ms();
    }

    DM_App_LCD_Sq_Set_Step(LCD_SQ_SAMPLE_LOAD_WAIT);
    DM_App_Main_Sq_Set_Step(MAIN_SQ_SAMPLE_LOAD_WAIT);
#else
    uint8_t bResult;
    static uint8_t i;

    /* 0. Reset stale measurement/result data from the previous cycle so an
       early failure below doesn't save leftover data from a prior test to ROM. */
    DM_App_Optic_ResetPWM();
    for (i = 0; i < BAND_MAX; i++)
    {
        gs_awEmptyBand_LowData[i]  = 0;
        gs_awResultBand_LowData[i] = 0;
    }
    for (i = 0; i < RES_MAX; i++)
    {
        gs_awResult[i] = 0;
    }

    /* 1. Read gs_bStickInsertCnt and gs_bTestCompleteCount from EEPROM */
    gs_bStickInsertCnt = DM_App_Rom_Get_InsertCount();
    gs_bTestCompleteCount = DM_App_Rom_Get_TestCompleteCount();

    /* 2. Increment gs_bStickInsertCnt */
    gs_bStickInsertCnt++;
    DM_App_Rom_Set_InsertCount(gs_bStickInsertCnt);

    /* Check if the device usage count exceeds _nSCAN_MAX_CNT */
    if (gs_bTestCompleteCount >= _nSCAN_MAX_CNT)
    {
        DM_App_Main_Sq_Enter_Error(ERROR_OVER_USED, LCD_SQ_ERR_OVER_USED); /* 120 seconds timeout */
        return;
    }

    /* 3. Read initial stick PTR values into local buffer */
    bResult = DM_App_Optic_MeasureInitialStick(gs_awInitialStickPtr);
    if (bResult == OPTIC_ERR_STICK_REMOVED)
    {
        DM_App_Main_Sq_Enter_Error(ERROR_STICK_REMOVE, LCD_SQ_ERR_STICK_REMOVE);
        return;
    }
    else if (bResult != OPTIC_SUCCESS)
    {
        /* Store error code (bResult) for out-of-range initial channel readings */
        DM_App_Main_Sq_Enter_Error(bResult, LCD_SQ_ERR_STICK_FAIL);
        return;
    }

    /* 4. Execute DM_App_Optic_TuneTargetADC for calibration using local init buffer */
    bResult = DM_App_Optic_TuneTargetADC(gs_awInitialStickPtr, _nTAGET_LIGHT);
    if (bResult == OPTIC_ERR_STICK_REMOVED)
    {
        DM_App_Main_Sq_Enter_Error(ERROR_STICK_REMOVE, LCD_SQ_ERR_STICK_REMOVE);
        return;
    }
    else if (bResult != OPTIC_SUCCESS)
    {
        /* Store error code (bResult) for other tuning failures */
        DM_App_Main_Sq_Enter_Error(bResult, LCD_SQ_ERR_STICK_FAIL);
        return;
    }
    
    /* 5. Measure Empty PTR and store result directly in gs_awEmptyBand_LowData */
    bResult = DM_App_Optic_MeasureEmptyPTR(gs_awEmptyBand_LowData);
    if (bResult == OPTIC_ERR_STICK_REMOVED)
    {
        DM_App_Main_Sq_Enter_Error(ERROR_STICK_REMOVE, LCD_SQ_ERR_STICK_REMOVE);
        return;
    }

    /* 6. Wait for LCD_SQ_STICK_INSERT animation to finish (LCD_SQ_IDLE) before transitioning */
    while (DM_App_LCD_Sq_Get_Step() != LCD_SQ_IDLE)
    {
        if (DM_App_Main_Sq_Sleep_And_CheckStick_250ms(ERROR_STICK_REMOVE, LCD_SQ_ERR_STICK_REMOVE))
        {
            return;
        }
    }

    /* All conditions met: Move to next sequence */
    DM_App_LCD_Sq_Set_Step(LCD_SQ_SAMPLE_LOAD_WAIT);
    DM_App_Main_Sq_Set_Step(MAIN_SQ_SAMPLE_LOAD_WAIT);
#endif
}

/**
  * @brief  Returns the current Main Sequence Step.
  * @param  None
  * @retval Current main sequence step.
  */
MAIN_SQ_STEP_t DM_App_Main_Sq_Get_Step(void)
{
    return gs_tMain_Sq_Step;
}

/**
  * @brief  Sets the Main Sequence Step.
  * @param  tStep: Target step.
  * @retval None
  */
void DM_App_Main_Sq_Set_Step(MAIN_SQ_STEP_t tStep)
{
    gs_tMain_Sq_Step = tStep;
}

/**
  * @brief  Returns the current Error Sequence.
  * @param  None
  * @retval Current error sequence.
  */
uint8_t DM_App_Main_Sq_Get_ErrorSequence(void)
{
    return gs_bErrorSequence;
}

/**
  * @brief  Sets the Error Sequence.
  * @param  bErrorSq: Target error sequence.
  * @retval None
  */
void DM_App_Main_Sq_Set_ErrorSequence(uint8_t bErrorSq)
{
    gs_bErrorSequence = bErrorSq;
}

/**
  * @brief  Handles the SAMPLE_LOAD_WAIT step: 5 mins wait for sample loading.
  * @param  None
  * @retval None
  */
uint32_t g_TickCk;
static void DM_App_Main_Sq_Sample_Load_Wait_Step_Handler(void)
{
#if _DEBUG_LCD_SEQ_ONLY
    static uint16_t i;

    for (i = 0; i < _DEBUG_SEQ_STEP_DELAY; i++)
    {
        DM_HW_Drv_SystemSleep_10ms();
    }

    DM_App_Main_Sq_Set_Step(MAIN_SQ_OVER_SAMPLE_CHECK);
#else
    uint16_t wT0_Ratio, wFinal_T0_Ratio;
    uint16_t wT1_Ratio;
    uint16_t wMeasured_T, wMeasured_BT;
    uint16_t wPWM_T, wPWM_BT;
    uint32_t dwStartTick;
    static uint8_t i;

    /* 1. Calculate Initial T0 Ratio from empty band data */
    wT0_Ratio = CalculateIntensity(gs_awEmptyBand_LowData[BAND_T], gs_awEmptyBand_LowData[BAND_BT]);
    wFinal_T0_Ratio = Cal_Ratio(wT0_Ratio, _nEMPTY_BTB_RATIO);

    /* 2. Track elapsed time via GetSystemTick() (10ms units) rather than
       counting loop iterations - DM_App_Main_Sq_Sleep_And_CheckStick_250ms() can
       occasionally take much longer than 250ms (throttled OPTIC_CH_3 re-check),
       which would otherwise inflate this 5-minute wait well past 5 minutes. */
    dwStartTick = GetSystemTick();

    /* 3. Get current PWM for measurements */
    wPWM_T = DM_App_Optic_GetPWM(OPTIC_CH_0);
    wPWM_BT = DM_App_Optic_GetPWM(OPTIC_CH_1);

    /* 4. Monitoring Loop: up to SAMPLE_LOAD_WAIT_TIME x 10ms (5 minutes) of real elapsed time */
    while ((GetSystemTick() - dwStartTick) < SAMPLE_LOAD_WAIT_TIME)
    {
        /* ~1 second measurement cycle: 4 x 250ms stick-checked sleeps */
        for (i = 0; i < 4; i++)
        {
            if (DM_App_Main_Sq_Sleep_And_CheckStick_250ms(ERROR_STICK_REMOVE, LCD_SQ_ERR_STICK_REMOVE))
            {
                return;
            }
        }

        if ((GetSystemTick() - dwStartTick) >= SAMPLE_LOAD_WAIT_TIME) break;
        
        /* Measure T Band (OPTIC_CH_0) and BT Band (OPTIC_CH_1) */
        wMeasured_T = DM_App_Optic_Measure(OPTIC_CH_0, wPWM_T);
        wMeasured_BT = DM_App_Optic_Measure(OPTIC_CH_1, wPWM_BT);

        /* Calculate current T1 Ratio */
        wT1_Ratio = CalculateIntensity(wMeasured_T, wMeasured_BT);

        /* Check if sample loading is detected */
        if (wT1_Ratio < wFinal_T0_Ratio)
        {
            /* Store current T and BT that triggered the detection */
            gs_awResultBand_LowData[BAND_T] = wMeasured_T;
            gs_awResultBand_LowData[BAND_BT] = wMeasured_BT;

            /* Measure and store BC (OPTIC_CH_2) and C (OPTIC_CH_3) */
            gs_awResultBand_LowData[BAND_BC] = DM_App_Optic_Measure(OPTIC_CH_2, DM_App_Optic_GetPWM(OPTIC_CH_2));
            gs_awResultBand_LowData[BAND_C] = DM_App_Optic_Measure(OPTIC_CH_3, DM_App_Optic_GetPWM(OPTIC_CH_3));

            /* Transition to Next Step */
            DM_App_Main_Sq_Set_Step(MAIN_SQ_OVER_SAMPLE_CHECK);
            return;
        }
    }
    
    g_TickCk = GetSystemTick() - dwStartTick;

    /* 4. Timeout: No change for 5 minutes, go back to IDLE (Sleep) */
    /* Record final measured T and BT for diagnostics */
    gs_awResultBand_LowData[BAND_T] = wMeasured_T;
    gs_awResultBand_LowData[BAND_BT] = wMeasured_BT;

    /* Set Error Info */
    gs_bErrorCode = ERROR_SAMPLE_LOAD_FAIL;
    gs_bErrorSequence = gs_tMain_Sq_Step;

    /* Save to EEPROM */
    DM_App_Rom_Set_ErrorCode(gs_bErrorCode);
    DM_App_Rom_Set_ErrorStep(gs_bErrorSequence);

    /* Save measurement data to ROM (Index: Insert Count - 1, if > 0) */
    {
        uint8_t bSaveIndex = (gs_bStickInsertCnt > 0) ? (gs_bStickInsertCnt - 1) : 0;
        DM_App_Main_Sq_Save_Result_To_Rom(bSaveIndex);
    }

    /* Transition to IDLE which enters Halt Mode (Sleep) */
    DM_App_Main_Sq_Set_Step(MAIN_SQ_IDLE);
#endif
}

/**
  * @brief  Handles the OVER_SAMPLE_CHECK step: 5s observation for overflow detection.
  * @param  None
  * @retval None
  */
static void DM_App_Main_Sq_Over_Sample_Check_Step_Handler(void)
{
#if _DEBUG_LCD_SEQ_ONLY
    static uint16_t i;

    for (i = 0; i < _DEBUG_SEQ_STEP_DELAY; i++)
    {
        DM_HW_Drv_SystemSleep_10ms();
    }

    DM_App_LCD_Sq_Set_Step(LCD_SQ_REACTION_WAIT);
    DM_App_Main_Sq_Set_Step(MAIN_SQ_LOW_SAMPLE_CHECK);
#else
    uint16_t wThreshold;
    uint16_t wMeasured_C;
    uint32_t dwStartTick;

    /* 1. Calculate Threshold: (Initial_C * OVER_FLOW_RATIO) / 100 */
    wThreshold = Cal_Ratio(gs_awResultBand_LowData[BAND_C], _nOVER_FLOW_RATIO);

    /* 2. Wait for 5 seconds, tracked via real elapsed ticks (see
       Sample_Load_Wait_Step_Handler for why iteration counting is unsafe) */
    dwStartTick = GetSystemTick();
    while ((GetSystemTick() - dwStartTick) < OVER_SAMPLE_CHECK_TIME)
    {
        if (DM_App_Main_Sq_Sleep_And_CheckStick_250ms(ERROR_STICK_FAIL, LCD_SQ_ERR_STICK_FAIL))
        {
            return;
        }
    }

    /* 3. Measure Current C value (OPTIC_CH_3) */
    wMeasured_C = DM_App_Optic_Measure(OPTIC_CH_3, DM_App_Optic_GetPWM(OPTIC_CH_3));

    /* 4. Comparison Logic */
    /* If Current_C is larger than Threshold (Dropped less than 92%) */
    if (wMeasured_C > wThreshold)
    {
        /* Normal flow: Proceed to Low Sample Check */
        DM_App_LCD_Sq_Set_Step(LCD_SQ_REACTION_WAIT);
        DM_App_Main_Sq_Set_Step(MAIN_SQ_LOW_SAMPLE_CHECK);
    }
    else
    {
        /* Over-flow detected: C value dropped too much/fast (Small C value) */
        DM_App_Main_Sq_Enter_Error(ERROR_OVER_FLOW, LCD_SQ_ERR_STICK_FAIL); /* Set to Fail animation */
    }
#endif
}

/**
  * @brief  Handles the LOW_SAMPLE_CHECK step: 20s wait to confirm C-line drop.
  * @param  None
  * @retval None
  */
static void DM_App_Main_Sq_Low_Sample_Check_Step_Handler(void)
{
#if _DEBUG_LCD_SEQ_ONLY
    static uint16_t i;

    for (i = 0; i < _DEBUG_SEQ_STEP_DELAY; i++)
    {
        DM_HW_Drv_SystemSleep_10ms();
    }

    DM_App_Main_Sq_Set_Step(MAIN_SQ_REACTION_WAIT);
#else
    uint16_t wThreshold;
    uint16_t wMeasured_C;
    uint32_t dwStartTick;

    /* 1. Calculate Threshold: (Empty_C * SMALL_SAMPLE_RATIO) / 100 */
    wThreshold = Cal_Ratio(gs_awEmptyBand_LowData[BAND_C], _nSMALL_SAMPLE_RATIO);

    /* 2. Wait for 20 seconds, tracked via real elapsed ticks (see
       Sample_Load_Wait_Step_Handler for why iteration counting is unsafe) */
    dwStartTick = GetSystemTick();
    while ((GetSystemTick() - dwStartTick) < LOW_SAMPLE_CHECK_TIME)
    {
        if (DM_App_Main_Sq_Sleep_And_CheckStick_250ms(ERROR_STICK_FAIL, LCD_SQ_ERR_STICK_FAIL))
        {
            return;
        }
    }

    /* 3. Measure Current C value (OPTIC_CH_3) */
    wMeasured_C = DM_App_Optic_Measure(OPTIC_CH_3, DM_App_Optic_GetPWM(OPTIC_CH_3));

    /* 4. Comparison Logic */
    /* If Current_C is smaller than Threshold (C value dropped enough) */
    if (wMeasured_C < wThreshold)
    {
        /* Success: Proceed to Reaction Wait */
        DM_App_Main_Sq_Set_Step(MAIN_SQ_REACTION_WAIT);
    }
    else
    {
        /* Low sample volume detected: C value didn't drop enough (Remains high) */
        /* Note: Using ERROR_LOW_SAMPLE for semantic correctness in LOW_SAMPLE_CHECK step. */
        DM_App_Main_Sq_Enter_Error(ERROR_LOW_SAMPLE, LCD_SQ_ERR_STICK_FAIL); /* Set to Fail animation */
    }
#endif
}

/**
  * @brief  Handles the REACTION_WAIT step: Remaining time to reach total 3 mins.
  * @param  None
  * @retval None
  */
static void DM_App_Main_Sq_Reaction_Wait_Step_Handler(void)
{
#if _DEBUG_LCD_SEQ_ONLY
    static uint16_t i;

    for (i = 0; i < _DEBUG_SEQ_STEP_DELAY; i++)
    {
        DM_HW_Drv_SystemSleep_10ms();
    }
#else
    {
        uint32_t dwStartTick = GetSystemTick();

        /* Wait for the calculated remaining time (REACTION_WAIT_TIME), tracked
           via real elapsed ticks (see Sample_Load_Wait_Step_Handler for why
           iteration counting is unsafe) */
        while ((GetSystemTick() - dwStartTick) < REACTION_WAIT_TIME)
        {
            if (DM_App_Main_Sq_Sleep_And_CheckStick_250ms(ERROR_STICK_FAIL, LCD_SQ_ERR_STICK_FAIL))
            {
                return;
            }
        }
    }
#endif

    /* Total 3 minutes elapsed: Proceed to Result Scan */
    DM_App_Main_Sq_Set_Step(MAIN_SQ_RESULT_SCAN);
}

/**
  * @brief  Handles the RESULT_SCAN step: Final measurement and result determination.
  * @param  None
  * @retval None
  */
static void DM_App_Main_Sq_Result_Scan_Step_Handler(void)
{
#if _DEBUG_LCD_SEQ_ONLY
    /* Test mode: skip real measurement, just alternate YES/NO so both
       result LCD animations can be checked across repeated cycles. */
    static uint8_t bToggle;

    bToggle = !bToggle;

    if (bToggle)
    {
        DM_App_LCD_Sq_Set_Step(LCD_SQ_RESULT_YES);
    }
    else
    {
        DM_App_LCD_Sq_Set_Step(LCD_SQ_RESULT_NO);
    }

    DM_App_Main_Sq_Set_Step(MAIN_SQ_RESULT_OUT);
#else
    uint16_t wFinal_BT, wFinal_T, wFinal_BC, wFinal_C;
    uint16_t wResult_T, wResult_C;
    uint8_t  bResult_Yes_No;

    /* 1. Final measurement from C to T band (Reverse order) */
    /* Store raw ADC values in gs_awResultBand_LowData */
    gs_awResultBand_LowData[BAND_C]  = DM_App_Optic_Measure(OPTIC_CH_3, DM_App_Optic_GetPWM(OPTIC_CH_3));
    gs_awResultBand_LowData[BAND_BC] = DM_App_Optic_Measure(OPTIC_CH_2, DM_App_Optic_GetPWM(OPTIC_CH_2));
    gs_awResultBand_LowData[BAND_BT] = DM_App_Optic_Measure(OPTIC_CH_1, DM_App_Optic_GetPWM(OPTIC_CH_1));
    gs_awResultBand_LowData[BAND_T]  = DM_App_Optic_Measure(OPTIC_CH_0, DM_App_Optic_GetPWM(OPTIC_CH_0));

    /* 2. Calculate Normalized Intensities (Normalized to Empty Values) */
    /* Final BT = (Current BT / Empty BT) * 1000 */
    wFinal_BT = CalculateIntensity(gs_awResultBand_LowData[BAND_BT], gs_awEmptyBand_LowData[BAND_BT]);
    /* Final T = (Current T / Empty T) * 1000 */
    wFinal_T  = CalculateIntensity(gs_awResultBand_LowData[BAND_T], gs_awEmptyBand_LowData[BAND_T]);
    /* Result T = (Final BT / Final T) * 1000 */
    wResult_T = CalculateIntensity(wFinal_BT, wFinal_T);

    /* Final BC = (Current BC / Empty BC) * 1000 */
    wFinal_BC = CalculateIntensity(gs_awResultBand_LowData[BAND_BC], gs_awEmptyBand_LowData[BAND_BC]);
    /* Final C = (Current C / Empty C) * 1000 */
    wFinal_C  = CalculateIntensity(gs_awResultBand_LowData[BAND_C], gs_awEmptyBand_LowData[BAND_C]);
    /* Result C = (Final BC / Final C) * 1000 */
    wResult_C = CalculateIntensity(wFinal_BC, wFinal_C);

    /* 3. Store measured intensities first so they are saved to ROM even if the C Line check below fails */
    gs_awResult[RES_T_INTENSITY] = wResult_T;
    gs_awResult[RES_C_INTENSITY] = wResult_C;

    /* 4. Validity Check: Result C Line Intensity */
    if (wResult_C < _nC_LINE_INT_THRESHOLD)
    {
        /* C Line failed: Control line error */
        DM_App_Main_Sq_Enter_Error(ERROR_C_LINE_FAIL, LCD_SQ_ERR_STICK_FAIL);
        return;
    }

    /* 5. Determine YES/NO result based on T Line Intensity */
    if (wResult_T > _nT_LINE_INT_THRESHOLD)
    {
        bResult_Yes_No = 1; /* YES */
        DM_App_LCD_Sq_Set_Step(LCD_SQ_RESULT_YES);
    }
    else
    {
        bResult_Yes_No = 0; /* NO */
        DM_App_LCD_Sq_Set_Step(LCD_SQ_RESULT_NO);
    }

    /* 6. Store final YES/NO decision */
    gs_awResult[RES_YES_NO] = (uint16_t)bResult_Yes_No;

    /* 7. Transition to Result Output phase */
    DM_App_Main_Sq_Set_Step(MAIN_SQ_RESULT_OUT);
#endif
}

/**
  * @brief  Packs current measurement and result data into 32 bytes and saves to ROM.
  * @note   Data composition (Total 32 bytes):
  *         - PWM Values: 8 bytes
  *         - EmptyBandData: 8 bytes
  *         - ResultBandData: 8 bytes
  *         - FinalResults: 6 bytes
  *         - Dummy: 2 bytes
  * @param  bSaveIndex: Target storage index (0 ~ 6).
  * @retval None
  */
void DM_App_Main_Sq_Save_Result_To_Rom(uint8_t bSaveIndex)
{
    uint16_t awRomBuffer[16];
    static uint8_t i, idx;

    idx = 0;

    /* 1. Pack Optic PWM Values (8 bytes) */
    for (i = 0; i < (uint8_t)OPTIC_CH_MAX; i++)
    {
        awRomBuffer[idx++] = DM_App_Optic_GetPWM((OPTIC_CH_t)i);
    }

    /* 2. Pack gs_awEmptyBand_LowData (8 bytes) */
    for (i = 0; i < BAND_MAX; i++)
    {
        awRomBuffer[idx++] = gs_awEmptyBand_LowData[i];
    }

    /* 3. Pack gs_awResultBand_LowData (8 bytes) */
    for (i = 0; i < BAND_MAX; i++)
    {
        awRomBuffer[idx++] = gs_awResultBand_LowData[i];
    }

    /* 4. Pack gs_awResult (6 bytes) */
    for (i = 0; i < RES_MAX; i++)
    {
        awRomBuffer[idx++] = gs_awResult[i];
    }

    /* 5. Pack Dummy (2 bytes) */
    awRomBuffer[idx++] = 0x0000;

    /* 6. Save to ROM using index */
    DM_App_Rom_Save_CurrentResult(bSaveIndex, awRomBuffer);
}

/**
  * @brief  Handles the RESULT_OUT step: Increment test count, save data, and wait before sleep.
  * @param  None
  * @retval None
  */
static void DM_App_Main_Sq_Result_Out_Step_Handler(void)
{
    static uint16_t i;

#if _DEBUG_LCD_SEQ_ONLY
    for (i = 0; i < _DEBUG_SEQ_STEP_DELAY; i++)
    {
        DM_HW_Drv_SystemSleep_10ms();
    }
#else
    uint8_t bSaveIndex;

    /* 1. Increment Test Complete Count and Save to EEPROM */
    gs_bTestCompleteCount++;
    DM_App_Rom_Set_TestCompleteCount(gs_bTestCompleteCount);

    /* 2. Calculate Save Index (Insert Count - 1, if > 0) */
    bSaveIndex = (gs_bStickInsertCnt > 0) ? (gs_bStickInsertCnt - 1) : 0;

    /* 3. Save all measurement and calculation data to ROM */
    DM_App_Main_Sq_Save_Result_To_Rom(bSaveIndex);

    /* 4. Wait for 5 minutes (Animation / Visibility) */
    for (i = 0; i < RESULT_OUT_WAIT_TIME; i++)
    {
        DM_HW_Drv_SystemSleep_10ms();
    }
#endif

    /* 5. Transition to IDLE which enters Halt Mode (Sleep) */
    DM_App_Main_Sq_Set_Step(MAIN_SQ_IDLE);
}

/**
  * @brief  Handles the ERROR_WAIT step: Save error info and wait before sleep.
  * @param  None
  * @retval None
  */
static void DM_App_Main_Sq_Error_Wait_Step_Handler(void)
{
    uint8_t bSaveIndex;

    /* 1. First entry to Error Wait: Save Error Code and Step to EEPROM */
    if (gs_wMain_Sq_Timer == ERR_RUN_TIME)
    {
        DM_App_Rom_Set_ErrorCode(gs_bErrorCode);
        DM_App_Rom_Set_ErrorStep(gs_bErrorSequence);

        /* Calculate Save Index (Insert Count - 1, if > 0) */
        bSaveIndex = (gs_bStickInsertCnt > 0) ? (gs_bStickInsertCnt - 1) : 0;

        /* Save measurement data at the time of error for diagnostics */
        DM_App_Main_Sq_Save_Result_To_Rom(bSaveIndex);
    }

    /* 2. Run Wait Timer (2 minutes Animation/Delay) */
    if (gs_wMain_Sq_Timer > 0)
    {
        DM_HW_Drv_SystemSleep_10ms();
        gs_wMain_Sq_Timer--;
    }
    else
    {
        /* 3. Timeout: Transition to IDLE which enters Halt Mode (Sleep) */
        DM_App_Main_Sq_Set_Step(MAIN_SQ_IDLE);
    }
}

/**
  * @brief  Sends exactly bLen bytes of a string via USART - truncated if
  *         longer, zero-padded if shorter than bLen.
  * @param  pcStr: Null-terminated string to send.
  * @param  bLen: Exact number of bytes to send.
  * @retval None
  */
static void DM_App_Main_Sq_Send_FixedStr(const char* pcStr, uint8_t bLen)
{
    static uint8_t i;
    static uint8_t bDone;

    bDone = 0;
    for (i = 0; i < bLen; i++)
    {
        if (!bDone && (pcStr[i] == '\0'))
        {
            bDone = 1;
        }
        DM_HW_Drv_USART_SendByte(bDone ? 0 : (uint8_t)pcStr[i]);
    }
}

/**
  * @brief  Sends stored EEPROM data as raw uint16_t words via USART (no ASCII/CSV conversion).
  * @note   Leading header (21 bytes, fixed-width, truncated/padded as needed):
  *         _strFIRMWARE_VER (4), _REAL_FIRMWARE_VER (8), _strLOT_UPPER_4CHAR (4),
  *         _strLOT_LOWER_5NUM (5). Then word order: InsertCnt, TestCompCnt,
  *         ErrStep, ErrCode (4 words), Initial Stick PTR data (8 words), then
  *         7 x 16-word Result Records. Each word is sent High-byte-first.
  *         Total payload: 21 + (4+8+7*16)*2 = 21 + 248 = 269 bytes.
  * @param  None
  * @retval None
  */
void DM_App_Main_Sq_Send_RawDump(void)
{
    static uint8_t i, j;
    uint16_t awResultRec[16];

    /* 0. Firmware version / LOT info (fixed-width header, sent first) */
    DM_App_Main_Sq_Send_FixedStr(_strFIRMWARE_VER, 4);
    DM_App_Main_Sq_Send_FixedStr(_REAL_FIRMWARE_VER, 8);
    DM_App_Main_Sq_Send_FixedStr(_strLOT_UPPER_4CHAR, 4);
    DM_App_Main_Sq_Send_FixedStr(_strLOT_LOWER_5NUM, 5);

    /* 1. Core status counters/codes, sent as raw words */
    DM_HW_Drv_USART_SendWord((uint16_t)DM_App_Rom_Get_InsertCount());
    DM_HW_Drv_USART_SendWord((uint16_t)DM_App_Rom_Get_TestCompleteCount());
    DM_HW_Drv_USART_SendWord((uint16_t)DM_App_Rom_Get_ErrorStep());
    DM_HW_Drv_USART_SendWord((uint16_t)DM_App_Rom_Get_ErrorCode());

    /* 2. Initial Stick PTR data (8 words) */
    DM_App_Rom_Get_StickPtrData(gs_awInitialStickPtr);
    for (i = 0; i < 8; i++)
    {
        DM_HW_Drv_USART_SendWord(gs_awInitialStickPtr[i]);
    }

    /* 3. All 7 stored result records (16 words each, including dummy) */
    for (i = 0; i < 7; i++)
    {
        DM_App_Rom_Get_ResultData(i, awResultRec);

        for (j = 0; j < 16; j++)
        {
            DM_HW_Drv_USART_SendWord(awResultRec[j]);
        }
    }
}

/**
  * @brief  Resets the test complete count to 0 (EEPROM + in-RAM cache),
  *         clearing the ERROR_OVER_USED (_nSCAN_MAX_CNT) usage limit.
  * @note   Triggered by the 'R'/'r' + Enter USART command (service reset).
  * @param  None
  * @retval None
  */
void DM_App_Main_Sq_Reset_TestCompleteCount(void)
{
    gs_bTestCompleteCount = 0;
    DM_App_Rom_Set_TestCompleteCount(0);
}
