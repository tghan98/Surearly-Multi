/**
  ******************************************************************************
  * @file           : DM_Optic_Handle_App.c
  * @brief          : Optical Sensor Handling Application Implementation
  * @author         : Gemini CLI
  * @date           : 2026-04-24
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "DM_Optic_Handle_App.h"
#include "DM_HW_Drv.h"
#include "DM_Rom_Handl_App.h"
#include "DM_Utill.h"

#include "Parameter_define.h"

/* Private Variables ---------------------------------------------------------*/

/**
 * @brief LED Mapping for each Optical Channel
 */
static const LED_CH_t gs_tOptic_LED[OPTIC_CH_MAX] = {
    CH_LED_2,   /* OPTIC_CH_0 */
    CH_LED_2,   /* OPTIC_CH_1 */
    CH_LED_1,   /* OPTIC_CH_2 */
    CH_LED_1    /* OPTIC_CH_3 */
};

/**
 * @brief ADC Channel Mapping for each Optical Channel
 */
static const ADC_Channel_TypeDef gs_tOptic_ADC[OPTIC_CH_MAX] = {
    ADC_IDD_FRNT_CHANNEL,   /* OPTIC_CH_0 */
    ADC_IDD_MIDL_CHANNEL,   /* OPTIC_CH_1 */
    ADC_IDD_MIDL_CHANNEL,   /* OPTIC_CH_2 */
    ADC_IDD_REAR_CHANNEL    /* OPTIC_CH_3 */
};

/**
 * @brief Tuned gain (integer percent, OPTIC_GAIN_SCALE = x1.0) per Optical Channel
 */
static uint16_t gs_wOptic_Gain[OPTIC_CH_MAX] = {0, 0, 0, 0};

/* Functions -----------------------------------------------------------------*/

/**
  * @brief  Measures one optical channel at 100% LED drive.
  * @param  tCh: Optical channel selection.
  * @retval uint16_t: Raw reading (mean of the middle 16 of 20 samples), or 0 on
  *         ADC timeout.
  */
uint16_t  DM_App_Optic_Measure(OPTIC_CH_t tCh)
{
    uint16_t awSamples[20];
    uint32_t dwSum = 0;
    static uint8_t i, j;
    uint16_t wTemp = 0;
 
    if (tCh >= OPTIC_CH_MAX) 
    {
        return 0;
    }

    /* 1. Select the ADC channel once for the whole burst */
    DM_HW_Drv_ADC_ChannelSelect(gs_tOptic_ADC[tCh]);

    /* 2. Turn ON the mapped LED (mutual exclusion handled in Drv) */
    DM_HW_Drv_LED_Control(gs_tOptic_LED[tCh], ENABLE);

    /* 3. 20ms settle: the PTR RC constant is ~4.7ms, so this lands at ~98.7% */
    DM_HW_Drv_SystemSleep_10ms();
    DM_HW_Drv_SystemSleep_10ms();

    /* 4. Discard the first conversion after the channel change, then burst-read 20 */
    if (DM_HW_Drv_ADC_Read() == ADC_READ_TIMEOUT)
    {
        DM_HW_Drv_LED_Control(gs_tOptic_LED[tCh], DISABLE);
        DM_HW_Drv_ADC_ChannelDeselect(gs_tOptic_ADC[tCh]);
        return 0;
    }

    for (i = 0; i < 20; i++)
    {
        awSamples[i] = DM_HW_Drv_ADC_Read();

        /* Bail out rather than let a stuck ADC hold the LED on (coin cell) */
        if (awSamples[i] == ADC_READ_TIMEOUT)
        {
            DM_HW_Drv_LED_Control(gs_tOptic_LED[tCh], DISABLE);
            DM_HW_Drv_ADC_ChannelDeselect(gs_tOptic_ADC[tCh]);
            return 0;
        }
    }

    /* 5. Turn OFF LED and deselect the ADC channel */
    DM_HW_Drv_LED_Control(gs_tOptic_LED[tCh], DISABLE);
    DM_HW_Drv_ADC_ChannelDeselect(gs_tOptic_ADC[tCh]);

    /* 6. Recovery wait: let the supply cap recharge before the next burst */
    DM_HW_Drv_SystemSleep_10ms();

    /* 7. Sort samples (Bubble Sort) */
    for (i = 0; i < 19; i++)
    {
        for (j = i + 1; j < 20; j++)
        {
            if (awSamples[i] > awSamples[j])
            {
                wTemp = awSamples[i];
                awSamples[i] = awSamples[j];
                awSamples[j] = wTemp;
            }
        }
    }

    /* 8. Mean of the middle 16 values (Index 2 to 17) */
    for (i = 2; i <= 17; i++)
    {
        dwSum += awSamples[i];
    }

    return (uint16_t)(dwSum / 16);
}

/**
  * @brief  Returns the tuned gain for a specific optical channel.
  * @param  tCh: Optical channel selection.
  * @retval uint16_t: Current gain, 0~OPTIC_GAIN_SCALE.
  */
uint16_t DM_App_Optic_GetGain(OPTIC_CH_t tCh)
{
    if (tCh < OPTIC_CH_MAX)
    {
        return gs_wOptic_Gain[tCh];
    }
    return 0;
}

/**
  * @brief  Resets all channels' gain values to 0.
  * @note   Called at the start of a new stick-insert cycle so a stale gain
  *         value from the previous cycle isn't saved to ROM if this cycle
  *         fails before tuning runs.
  * @param  None
  * @retval None
  */
void DM_App_Optic_ResetGain(void)
{
    static uint8_t i;

    for (i = 0; i < (uint8_t)OPTIC_CH_MAX; i++)
    {
        gs_wOptic_Gain[i] = 0;
    }
}

/**
  * @brief  Measures initial PTR values for all 4 optic channels at 100% drive.
  * @note   Each channel's raw reading is validated against OPTIC_INITIAL_MIN/MAX_100.
  * @param  pBuffer: Pointer to uint16_t array (size 8) to store measurements.
  * @retval uint8_t: OPTIC_SUCCESS, OPTIC_ERR_STICK_REMOVED, or FAIL_OPTIC_CH0..CH3
  *         if a channel's reading is out of the expected range.
  */
uint8_t DM_App_Optic_MeasureInitialStick(uint16_t* pBuffer)
{
    static uint8_t bCh;

    /* Channel count halved from the old 2-point sweep; the trailing words stay
       zeroed so the 16-byte EEPROM record and its PC-side parser are unchanged. */
    for (bCh = (uint8_t)OPTIC_CH_MAX; bCh < 8; bCh++)
    {
        pBuffer[bCh] = 0;
    }

    /* Loop through all 4 optical channels */
    for (bCh = 0; bCh < (uint8_t)OPTIC_CH_MAX; bCh++)
    {
        /* Guard-rail: uses DM_App_Optic_Check_StickPresent() directly instead of
           DM_App_Stick_Get_Status(), so the reading is never served from its cache. */
        if (!DM_App_Optic_Check_StickPresent())
        {
            return OPTIC_ERR_STICK_REMOVED;
        }

        pBuffer[bCh] = DM_App_Optic_Measure((OPTIC_CH_t)bCh);

        /* Raw (pre-gain) scale: after gain every channel lands near the target,
           which would make this range check meaningless. */
        if ((pBuffer[bCh] < OPTIC_INITIAL_MIN_100) || (pBuffer[bCh] > OPTIC_INITIAL_MAX_100))
        {
            return (uint8_t)(FAIL_OPTIC_CH0 + bCh);
        }
    }
    
    /* Final Check before EEPROM write */
    if (!DM_App_Optic_Check_StickPresent())
    {
        return OPTIC_ERR_STICK_REMOVED;
    }

    /* Backup the entire set to EEPROM */
    DM_App_Rom_Set_StickPtrData(pBuffer);
    
    return OPTIC_SUCCESS;
}

/**
  * @brief  Computes each channel's gain from the initial readings.
  * @note   Reuses pInitBuffer instead of re-measuring; the values were taken
  *         moments earlier under identical conditions.
  * @param  pInitBuffer: Pointer to the initial measurement buffer.
  * @param  wTargetADC: The target ADC value to reach (0-4095).
  * @retval uint8_t: OPTIC_SUCCESS or FAIL_OPTIC_CH0..CH3.
  */
uint8_t DM_App_Optic_TuneTargetADC(uint16_t* pInitBuffer, uint16_t wTargetADC)
{
    static uint8_t bCh;
    uint16_t wTuned;

    for (bCh = 0; bCh < (uint8_t)OPTIC_CH_MAX; bCh++)
    {
        /* No stick check here: the original checked per channel because P-control
           burned up to ~3s per channel, but gain tuning only computes (a few us).
           The final check at the end of MeasureInitialStick() already covers it. */

        /* Defensive re-check: the caller is expected to have run the range check
           already, but confirm here so this function does not depend on call order. */
        if ((pInitBuffer[bCh] < OPTIC_INITIAL_MIN_100) || (pInitBuffer[bCh] > OPTIC_INITIAL_MAX_100))
        {
            return (uint8_t)(FAIL_OPTIC_CH0 + bCh);
        }

        /* Round to nearest: truncation would bias the applied value low by up to
           ~32 at this target. Raw reading is guaranteed 1500..3900 here, so the
           gain stays within 31..80. */
        gs_wOptic_Gain[bCh] = (uint16_t)(((uint32_t)wTargetADC * OPTIC_GAIN_SCALE
                                          + pInitBuffer[bCh] / 2) / pInitBuffer[bCh]);

        /* Verify the exact value CalcEmptyPTR() will store (same expression), so a
           channel that cannot be brought near the target fails here rather than
           silently landing outside tolerance. */
        wTuned = Cal_Ratio(pInitBuffer[bCh], gs_wOptic_Gain[bCh]);
        if ((wTuned < (uint16_t)(wTargetADC - OPTIC_TUNED_TOLERANCE)) ||
            (wTuned > (uint16_t)(wTargetADC + OPTIC_TUNED_TOLERANCE)))
        {
            return (uint8_t)(FAIL_OPTIC_CH0 + bCh);
        }
    }

    return OPTIC_SUCCESS;
}

/**
  * @brief  Derives the Empty PTR values from the initial readings and the tuned gain.
  * @note   No measurement: gain mode keeps the LED at 100%, so the initial reading,
  *         taken under the same conditions, scaled by gain is the Empty value.
  * @param  pInitBuffer: Initial readings from DM_App_Optic_MeasureInitialStick().
  * @param  pBuffer: Pointer to uint16_t array (size 4) to store results.
  * @retval uint8_t: OPTIC_SUCCESS or FAIL_OPTIC_CH0..CH3 if a channel's gain is 0.
  */
uint8_t DM_App_Optic_CalcEmptyPTR(const uint16_t* pInitBuffer, uint16_t* pBuffer)
{
    static uint8_t bCh;

    for (bCh = 0; bCh < (uint8_t)OPTIC_CH_MAX; bCh++)
    {
        /* Zero gain means TuneTargetADC() has not run. Fail loudly rather than
           writing a plausible-looking number. Not reachable on the normal path
           (gain lands in 31..80), only if the call order is broken. */
        if (gs_wOptic_Gain[bCh] == 0)
        {
            return (uint8_t)(FAIL_OPTIC_CH0 + bCh);
        }

        /* Must match the expression checked in DM_App_Optic_TuneTargetADC(). */
        pBuffer[bCh] = Cal_Ratio(pInitBuffer[bCh], gs_wOptic_Gain[bCh]);
    }

    return OPTIC_SUCCESS;
}

/**
  * @brief  Immediately measures OPTIC_CH_3 (innermost sensor, LED1) at Duty 100%
  *         and judges whether the stick is physically present.
  * @param  None
  * @retval uint8_t: 1 if stick present, 0 if removed.
  */

uint8_t DM_App_Optic_Check_StickPresent(void)
{
    uint16_t wADC = DM_App_Optic_Measure(OPTIC_CH_3);

    return (wADC < OPTIC_STICK_REMOVE_THRESHOLD) ? 0 : 1;
}

/**
  * @brief  Measures OPTIC_CH_3 and judges stick presence on the GAIN-APPLIED scale.
  * @note   전제: 배율이 계산된 뒤에만 부를 것.
  *         배율이 0이면 Cal_Ratio() 결과가 0이 되어 항상 "빠짐"으로 나온다.
  *         현재 호출처는 DM_App_Stick_Get_Status() 하나뿐이고,
  *         그것은 대기 루프(배율 계산 이후)에서만 돌아간다.
  * @param  None
  * @retval uint8_t: 1 if stick present, 0 if removed.
  */
uint8_t DM_App_Optic_Check_StickPresent_Scaled(void)
{
    uint16_t wVal = Cal_Ratio(DM_App_Optic_Measure(OPTIC_CH_3),
                              DM_App_Optic_GetGain(OPTIC_CH_3));

    return (wVal < OPTIC_STICK_REMOVE_THRESHOLD_SCALED) ? 0 : 1;
}

//LED test
void LED_ON_Test(void)
{
    /* Turn ON the mapped LED (v1.1: cathode Low = ON) */
    DM_HW_Drv_LED_Control(gs_tOptic_LED[0], ENABLE);

    DM_HW_Drv_SystemSleep_10ms();
}


/************************ (C) COPYRIGHT Surearly Multi *****END OF FILE****/
