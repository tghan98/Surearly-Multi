/**
  ******************************************************************************
  * @file           : DM_LCD_Stick_Check_App.c
  * @brief          : LCD Segment and COM Control Implementation
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "DM_LCD_Stick_Check_App.h"
#include "DM_HW_Drv.h"
#include "DM_Optic_Handle_App.h"
#include "Parameter_define.h"

/* Private Defines -----------------------------------------------------------*/
#define LCD_BLINK_INTERVAL_MS    500
#define LCD_TICK_10MS            10
#define LCD_BLINK_COUNT_MAX      (LCD_BLINK_INTERVAL_MS / LCD_TICK_10MS)

#define OPTIC_STICK_CHECK_INTERVAL_COUNT (OPTIC_STICK_CHECK_INTERVAL_MS / LCD_TICK_10MS)

/* Private Variables ---------------------------------------------------------*/

/**
 * @brief Port mapping array for LCD segments
 */
static GPIO_TypeDef* const gs_pLCD_PORT[CH_LCD_MAX] = {
    LCD_BOOK_PORT,
    LCD_DROP_PORT,
    LCD_YES_PORT,
    LCD_NO_PORT
};

/**
 * @brief Pin mapping array for LCD segments
 */
static const uint8_t gs_bLCD_PIN[CH_LCD_MAX] = {
    LCD_BOOK_PIN,
    LCD_DROP_PIN,
    LCD_YES_PIN,
    LCD_NO_PIN
};

/**
 * @brief Current target state for each LCD channel
 */
static LCD_STATE_t gs_tLCD_TargetState[CH_LCD_MAX] = {
    LCD_STATE_OFF, 
    LCD_STATE_OFF, 
    LCD_STATE_OFF, 
    LCD_STATE_OFF
};

static BitStatus gs_tLCD_COM_Phase = RESET;
static uint16_t gs_wLCD_BlinkCounter = 0;
static uint8_t gs_bLCD_BlinkToggle = 0;

static LCD_SQ_STEP_t gs_tLCD_Sq_Step = LCD_SQ_IDLE;
static uint8_t gs_bLCD_Sq_SubStep = 0;
static uint16_t gs_wLCD_Sq_Timer = 0;

/**
 * @brief One step of a table-driven icon animation.
 * @note  bDelay == 0 means "no wait": the next table entry is applied
 *        immediately within the same call (replicates the original
 *        switch-case fallthrough behavior).
 */
typedef struct
{
    uint8_t bCh;     /**< LCD_CH_t: target channel (or CH_LCD_ALL) */
    uint8_t bState;  /**< LCD_STATE_t: target state */
    uint8_t bDelay;  /**< Delay in 10ms ticks before the next step */
} LCD_ANIM_STEP_t;

/**
 * @brief Animation table for LCD_SQ_STICK_INSERT.
 */
static const LCD_ANIM_STEP_t gs_atStickInsertAnim[] =
{
    { CH_LCD_ALL,  LCD_STATE_ON,  50 },
    { CH_LCD_ALL,  LCD_STATE_OFF, 50 },
    { CH_LCD_ALL,  LCD_STATE_ON,  50 },
    { CH_LCD_ALL,  LCD_STATE_OFF, 50 },
    { CH_LCD_ALL,  LCD_STATE_ON,  50 },
    { CH_LCD_ALL,  LCD_STATE_OFF,  0 },
    { CH_LCD_DROP, LCD_STATE_ON,  50 },
    { CH_LCD_ALL,  LCD_STATE_OFF,  0 },
    { CH_LCD_YES,  LCD_STATE_ON,  50 },
    { CH_LCD_ALL,  LCD_STATE_OFF,  0 },
    { CH_LCD_NO,   LCD_STATE_ON,  50 },
    { CH_LCD_ALL,  LCD_STATE_OFF,  0 },
    { CH_LCD_BOOK, LCD_STATE_ON,  50 },
    { CH_LCD_ALL,  LCD_STATE_OFF,  0 },
    { CH_LCD_DROP, LCD_STATE_ON,  50 },
};
#define STICK_INSERT_ANIM_STEPS  (sizeof(gs_atStickInsertAnim) / sizeof(gs_atStickInsertAnim[0]))

/**
 * @brief Animation table for LCD_SQ_ERR_OVER_USED (repeats indefinitely).
 */
static const LCD_ANIM_STEP_t gs_atErrOverUsedAnim[] =
{
    { CH_LCD_ALL,  LCD_STATE_OFF,  0 },
    { CH_LCD_BOOK, LCD_STATE_ON,  50 },
    { CH_LCD_ALL,  LCD_STATE_OFF,  0 },
    { CH_LCD_NO,   LCD_STATE_ON,  50 },
    { CH_LCD_ALL,  LCD_STATE_OFF,  0 },
    { CH_LCD_YES,  LCD_STATE_ON,  50 },
    { CH_LCD_ALL,  LCD_STATE_OFF,  0 },
    { CH_LCD_DROP, LCD_STATE_ON,  50 },
};
#define ERR_OVER_USED_ANIM_STEPS  (sizeof(gs_atErrOverUsedAnim) / sizeof(gs_atErrOverUsedAnim[0]))

/* Private Function Prototypes -----------------------------------------------*/
static void DM_App_LCD_Sq_StickInsert_Process(void);
static void DM_App_LCD_Sq_SampleLoadWait_Process(void);
static void DM_App_LCD_Sq_ReactionWait_Process(void);
static void DM_App_LCD_Sq_ErrOverUsed_Process(void);
static void DM_App_LCD_Sq_ErrStickRemove_Process(void);
static void DM_App_LCD_Sq_ErrStickFail_Process(void);
static void DM_App_LCD_Sq_ResultYes_Process(void);
static void DM_App_LCD_Sq_ResultNo_Process(void);

/* Functions -----------------------------------------------------------------*/

/**
  * @brief  Process LCD Animation/State Sequence.
  * @note   This function is called from the TIM4 interrupt (10ms).
  * @param  None
  * @retval None
  */
void DM_App_LCD_Sq_Process(void)
{
    switch (gs_tLCD_Sq_Step)
    {
        case LCD_SQ_IDLE:
            break;

        case LCD_SQ_STICK_INSERT:
            DM_App_LCD_Sq_StickInsert_Process();
            break;

        case LCD_SQ_SAMPLE_LOAD_WAIT:
            DM_App_LCD_Sq_SampleLoadWait_Process();
            break;

        case LCD_SQ_OVER_SAMPLE_CHECK:
            break;

        case LCD_SQ_LOW_SAMPLE_CHECK:
            break;

        case LCD_SQ_REACTION_WAIT:
            DM_App_LCD_Sq_ReactionWait_Process();
            break;

        case LCD_SQ_RESULT_SCAN:
            break;

        case LCD_SQ_RESULT_YES:
            DM_App_LCD_Sq_ResultYes_Process();
            break;

        case LCD_SQ_RESULT_NO:
            DM_App_LCD_Sq_ResultNo_Process();
            break;

        case LCD_SQ_ERR_STICK_REMOVE:
            DM_App_LCD_Sq_ErrStickRemove_Process();
            break;

        case LCD_SQ_ERR_STICK_FAIL:
            DM_App_LCD_Sq_ErrStickFail_Process();
            break;

        case LCD_SQ_ERR_OVER_USED:
            DM_App_LCD_Sq_ErrOverUsed_Process();
            break;

        case LCD_SQ_ERR_DEV_FAIL:
            /* TODO: Implement Error Animation */
            break;
            
        default:
            gs_tLCD_Sq_Step = LCD_SQ_IDLE;
            break;
    }
}

/**
  * @brief  Sets the LCD Sequence Step.
  * @param  tStep: Target step.
  * @retval None
  */
void DM_App_LCD_Sq_Set_Step(LCD_SQ_STEP_t tStep)
{
    gs_tLCD_Sq_Step = tStep;
    gs_bLCD_Sq_SubStep = 0;
    gs_wLCD_Sq_Timer = 0;
}

/**
  * @brief  Returns the current LCD Sequence Step.
  * @param  None
  * @retval LCD_SQ_STEP_t: Current step.
  */
LCD_SQ_STEP_t DM_App_LCD_Sq_Get_Step(void)
{
    return gs_tLCD_Sq_Step;
}

/**
  * @brief  Returns the current stick presence status.
  * @note   STP_CK switch is used only as the Halt wake-up trigger (unreliable for
  *         continuous status: mechanical play lets contact momentarily separate
  *         while the stick is still inserted). Presence is instead judged from
  *         OPTIC_CH_3 (innermost sensor, LED1) at Duty 100% (PWM 399): a genuinely
  *         removed stick leaves no reflective body in front of that channel, so
  *         the ADC reading collapses well below OPTIC_STICK_REMOVE_THRESHOLD.
  *         Re-measurement is throttled to OPTIC_STICK_CHECK_INTERVAL_MS so calling
  *         this every 10ms in a wait loop doesn't cycle the LED/PTR that often.
  * @param  None
  * @retval Current stick status (INSERTED or REMOVED).
  */
STICK_STATUS_t DM_App_Stick_Get_Status(void)
{
    static uint16_t wCheckCounter = OPTIC_STICK_CHECK_INTERVAL_COUNT;
    static STICK_STATUS_t tCachedStatus = STICK_STATUS_INSERTED;

    if (wCheckCounter < OPTIC_STICK_CHECK_INTERVAL_COUNT)
    {
        wCheckCounter++;
        return tCachedStatus;
    }

    wCheckCounter = 0;
    tCachedStatus = DM_App_Optic_Check_StickPresent() ? STICK_STATUS_INSERTED : STICK_STATUS_REMOVED;

    return tCachedStatus;
}

/**
  * @brief  Sets the target state of a specific LCD channel.
  * @param  tCh: LCD channel (BOOK, DROP, YES, NO, or ALL).
  * @param  tState: Target state (OFF, ON, BLINK).
  * @retval None
  */
void DM_App_LCD_SetState(LCD_CH_t tCh, LCD_STATE_t tState)
{
    static uint8_t i;

    if (tCh == CH_LCD_ALL)
    {
        for (i = 0; i < CH_LCD_MAX; i++)
        {
            gs_tLCD_TargetState[i] = tState;
        }
    }
    else if (tCh < CH_LCD_MAX)
    {
        gs_tLCD_TargetState[tCh] = tState;
    }
}

/**
  * @brief  Refreshes the LCD pins based on COM phase and target states.
  * @note   This function MUST be called every 10ms in the TIM4 interrupt.
  * @param  None
  * @retval None
  */
void DM_App_LCD_Refresh(void)
{
    static uint8_t i;
    BitStatus tSegState;
    uint8_t bIsOn;

    /* 1. Toggle COM Phase (10ms interval) */
    gs_tLCD_COM_Phase = (gs_tLCD_COM_Phase == RESET) ? SET : RESET;
    DM_App_LCD_COM_Control(gs_tLCD_COM_Phase);

    /* 2. Update Blink Logic */
    gs_wLCD_BlinkCounter++;
    if (gs_wLCD_BlinkCounter >= LCD_BLINK_COUNT_MAX)
    {
        gs_wLCD_BlinkCounter = 0;
        gs_bLCD_BlinkToggle = !gs_bLCD_BlinkToggle;
    }

    /* 3. Control each SEG based on logical state and COM phase */
    for (i = 0; i < CH_LCD_MAX; i++)
    {
        bIsOn = 0;

        /* Determine if the icon should be logically ON */
        switch (gs_tLCD_TargetState[i])
        {
            case LCD_STATE_ON:
                bIsOn = 1;
                break;
            case LCD_STATE_BLINK:
                if (gs_bLCD_BlinkToggle)
                {
                    bIsOn = 1;
                }
                break;
            case LCD_STATE_OFF:
            default:
                bIsOn = 0;
                break;
        }

        /*
           Apply Phase Inversion Driving:
           - Logical OFF: SEG = COM (Zero voltage difference)
           - Logical ON : SEG = !COM (Maximum voltage difference)
        */
        if (bIsOn)
        {
            tSegState = (gs_tLCD_COM_Phase == RESET) ? SET : RESET;
        }
        else
        {
            tSegState = gs_tLCD_COM_Phase;
        }

        DM_App_LCD_SEG_Control((LCD_CH_t)i, tSegState);
    }
}

/**
  * @brief  Controls a specific LCD segment (icon) - Low level.
  * @param  tCh: LCD channel (BOOK, DROP, YES, NO).
  * @param  tState: Pin state (SET or RESET).
  * @retval None
  */
void DM_App_LCD_SEG_Control(LCD_CH_t tCh, BitStatus tState)
{
    /* Validation check for channel index */
    if (tCh < CH_LCD_MAX)
    {
        /* Drive the mapped GPIO pin */
        GPIO_WriteBit(gs_pLCD_PORT[tCh], (GPIO_Pin_TypeDef)gs_bLCD_PIN[tCh], tState);
    }
}

/**
  * @brief  Controls the LCD COM pin.
  * @param  tState: Pin state (SET or RESET).
  * @retval None
  */
void DM_App_LCD_COM_Control(BitStatus tState)
{
    /* Drive the COM0 pin (PB4, Push-Pull) */
    GPIO_WriteBit(LCD_COM0_GPIO_PORT, LCD_COM0_GPIO_PIN, tState);
}

/**
  * @brief  Private function to process Stick Insert Animation.
  * @param  None
  * @retval None
  */
static void DM_App_LCD_Sq_StickInsert_Process(void)
{
    /* This animation is called every 10ms */
    if (gs_wLCD_Sq_Timer > 0)
    {
        gs_wLCD_Sq_Timer--;
        return;
    }

    while (gs_bLCD_Sq_SubStep < STICK_INSERT_ANIM_STEPS)
    {
        const LCD_ANIM_STEP_t* ptStep = &gs_atStickInsertAnim[gs_bLCD_Sq_SubStep];

        DM_App_LCD_SetState((LCD_CH_t)ptStep->bCh, (LCD_STATE_t)ptStep->bState);
        gs_wLCD_Sq_Timer = ptStep->bDelay;
        gs_bLCD_Sq_SubStep++;

        /* bDelay == 0 means immediate fallthrough to the next step */
        if (gs_wLCD_Sq_Timer > 0)
        {
            return;
        }
    }

    /* End of Sequence -> To IDLE */
    DM_App_LCD_SetState(CH_LCD_ALL, LCD_STATE_OFF);
    gs_tLCD_Sq_Step = LCD_SQ_IDLE;
}

/**
  * @brief  Private function to process Sample Load Wait state.
  * @param  None
  * @retval None
  */
static void DM_App_LCD_Sq_SampleLoadWait_Process(void)
{
    /* 1. Clear all icons first */
    DM_App_LCD_SetState(CH_LCD_ALL, LCD_STATE_OFF);

    /* 2. Display DROP icon (Keep ON to indicate waiting for sample) */
    DM_App_LCD_SetState(CH_LCD_DROP, LCD_STATE_ON);

    /* 3. Transition to IDLE as the icon state is now set and persistent */
    gs_tLCD_Sq_Step = LCD_SQ_IDLE;
}

/**
  * @brief  Private function to process Reaction Wait state.
  * @param  None
  * @retval None
  */
static void DM_App_LCD_Sq_ReactionWait_Process(void)
{
    /* 1. Clear all icons first */
    DM_App_LCD_SetState(CH_LCD_ALL, LCD_STATE_OFF);

    /* 2. Display DROP icon (Blinking to indicate reaction is in progress) */
    DM_App_LCD_SetState(CH_LCD_DROP, LCD_STATE_BLINK);

    /* 3. Transition to IDLE as the icon state is now set and persistent */
    gs_tLCD_Sq_Step = LCD_SQ_IDLE;
}

/**
  * @brief  Private function to process Over Used Error Animation.
  * @param  None
  * @retval None
  */
static void DM_App_LCD_Sq_ErrOverUsed_Process(void)
{
    /* This animation is called every 10ms */
    if (gs_wLCD_Sq_Timer > 0)
    {
        gs_wLCD_Sq_Timer--;
        return;
    }

    while (gs_bLCD_Sq_SubStep < ERR_OVER_USED_ANIM_STEPS)
    {
        const LCD_ANIM_STEP_t* ptStep = &gs_atErrOverUsedAnim[gs_bLCD_Sq_SubStep];

        DM_App_LCD_SetState((LCD_CH_t)ptStep->bCh, (LCD_STATE_t)ptStep->bState);
        gs_wLCD_Sq_Timer = ptStep->bDelay;
        gs_bLCD_Sq_SubStep++;

        /* bDelay == 0 means immediate fallthrough to the next step */
        if (gs_wLCD_Sq_Timer > 0)
        {
            return;
        }
    }

    /* Reset to start for repeat */
    gs_bLCD_Sq_SubStep = 0;
}

/**
  * @brief  Private function to process Stick Remove Error state.
  * @param  None
  * @retval None
  */
static void DM_App_LCD_Sq_ErrStickRemove_Process(void)
{
    /* 1. Clear all icons first */
    DM_App_LCD_SetState(CH_LCD_ALL, LCD_STATE_OFF);

    /* 2. Display BOOK icon (ON) and DROP icon (Blinking) */
    DM_App_LCD_SetState(CH_LCD_BOOK, LCD_STATE_ON);
    DM_App_LCD_SetState(CH_LCD_DROP, LCD_STATE_BLINK);

    /* 3. Transition to IDLE as the icon state is now set and persistent */
    gs_tLCD_Sq_Step = LCD_SQ_IDLE;
}

/**
  * @brief  Private function to process Stick Fail Error state.
  * @param  None
  * @retval None
  */
static void DM_App_LCD_Sq_ErrStickFail_Process(void)
{
    /* 1. Clear all icons first */
    DM_App_LCD_SetState(CH_LCD_ALL, LCD_STATE_OFF);

    /* 2. Display BOOK icon (ON) */
    DM_App_LCD_SetState(CH_LCD_BOOK, LCD_STATE_ON);

    /* 3. Transition to IDLE as the icon state is now set and persistent */
    gs_tLCD_Sq_Step = LCD_SQ_IDLE;
}

/**
  * @brief  Private function to process YES Result state.
  * @param  None
  * @retval None
  */
static void DM_App_LCD_Sq_ResultYes_Process(void)
{
    /* 1. Clear all icons first */
    DM_App_LCD_SetState(CH_LCD_ALL, LCD_STATE_OFF);

    /* 2. Display YES icon (ON) */
    DM_App_LCD_SetState(CH_LCD_YES, LCD_STATE_ON);

    /* 3. Transition to IDLE as the icon state is now set and persistent */
    gs_tLCD_Sq_Step = LCD_SQ_IDLE;
}

/**
  * @brief  Private function to process NO Result state.
  * @param  None
  * @retval None
  */
static void DM_App_LCD_Sq_ResultNo_Process(void)
{
    /* 1. Clear all icons first */
    DM_App_LCD_SetState(CH_LCD_ALL, LCD_STATE_OFF);

    /* 2. Display NO icon (ON) */
    DM_App_LCD_SetState(CH_LCD_NO, LCD_STATE_ON);

    /* 3. Transition to IDLE as the icon state is now set and persistent */
    gs_tLCD_Sq_Step = LCD_SQ_IDLE;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
