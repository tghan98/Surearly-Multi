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
 * @brief PWM Duty Cycle for each Optical Channel (Configurable)
 */
static uint16_t gs_wOptic_PWM[OPTIC_CH_MAX] = {
    199, 199, 199, 199
};

/* Functions -----------------------------------------------------------------*/

/**
  * @brief  Measures ADC value for a specific optical channel set.
  * @param  tCh: Optical channel selection.
  * @param  wPWM: PWM duty cycle.
  * @retval uint16_t: Filtered ADC measurement result.
  */
uint16_t  DM_App_Optic_Measure(OPTIC_CH_t tCh, uint16_t wPWM)
{
    uint16_t awSamples[20];
    uint32_t dwSum = 0;
    static uint8_t i, j;
    uint16_t wTemp = 0;
 
    if (tCh >= OPTIC_CH_MAX) 
    {
        return 0;
    }

    /* 1. Turn ON PTR Power */
    DM_HW_Drv_PTR_Power_On();

    /* 2. Setup PWM Duty from parameter */
    DM_HW_Drv_LED_Duty_Set(wPWM);

    /* 3. Turn ON the mapped LED (Mutual exclusion handled in Drv) */
    DM_HW_Drv_LED_Control(gs_tOptic_LED[tCh], ENABLE);

    /* 4. Start PWM Output */
    DM_HW_Drv_LED_PWM_Start();

    /* 5. Select the ADC channel once for the whole burst (not per-sample) -
       halves conversion count vs. re-selecting on every sample, since the
       sort/trim below (discards 3 lowest + 3 highest) already absorbs a
       first-sample channel-switch settling artifact if one occurs. */
    DM_HW_Drv_ADC_ChannelSelect(gs_tOptic_ADC[tCh]);

    /* 6. Wait once for LED/input-filter stabilization (100K + 22nF ADC input
       filter, ~2.2ms time constant - 10ms covers ~4.5 tau). The LED and
       filter stay settled for the rest of the burst, so later samples don't
       need to repeat this wait. */
    DM_HW_Drv_SystemSleep_10ms();

    /* 7. Burst-read 20 samples back-to-back (~200us each per ADC_READ_TIME_US) */
    /* Total measurement time: ~10ms settle + ~4ms burst + 10ms recovery (below) */
    for (i = 0; i < 20; i++)
    {
        awSamples[i] = DM_HW_Drv_ADC_Read();
    }

    /* 8. Deselect the ADC channel */
    DM_HW_Drv_ADC_ChannelDeselect(gs_tOptic_ADC[tCh]);

    /* 9. Stop PWM and Turn OFF LED */
    DM_HW_Drv_LED_Stop();
    DM_HW_Drv_LED_Control(gs_tOptic_LED[tCh], DISABLE);

    /* 10. Turn OFF PTR Power */
    DM_HW_Drv_PTR_Power_Off();

    /* 11. Recovery wait: LED load is now off, let the 10uF supply capacitor
       recharge from the battery before the next measurement burst starts
       (one 10ms timer tick, per battery internal-resistance recovery time). */
    DM_HW_Drv_SystemSleep_10ms();

    /* 12. Sort samples (Bubble Sort) */
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

    /* 13. Sum the middle 14 values (Index 3 to 16) */
    for (i = 3; i <= 16; i++)
    {
        dwSum += awSamples[i];
    }

    /* 14. Return average */
    //return (uint16_t)(dwSum / 6);
    return (uint16_t)(dwSum);
}

/**
  * @brief  Returns the current PWM value for a specific optical channel.
  * @param  tCh: Optical channel selection.
  * @retval uint16_t: Current PWM duty cycle.
  */
uint16_t DM_App_Optic_GetPWM(OPTIC_CH_t tCh)
{
    if (tCh < OPTIC_CH_MAX)
    {
        return gs_wOptic_PWM[tCh];
    }
    return 0;
}

/**
  * @brief  Resets all channels' PWM values to 0.
  * @note   Called at the start of a new stick-insert cycle so a stale PWM
  *         value from the previous cycle isn't saved to ROM if this cycle
  *         fails before tuning runs.
  * @param  None
  * @retval None
  */
void DM_App_Optic_ResetPWM(void)
{
    static uint8_t i;

    for (i = 0; i < (uint8_t)OPTIC_CH_MAX; i++)
    {
        gs_wOptic_PWM[i] = 0;
    }
}

/**
  * @brief  Measures initial PTR values for all 4 optic channels at PWM 199 and 399.
  * @note   Each channel's readings are validated against OPTIC_INITIAL_MIN/MAX_50
  *         and _100; out of range fails that channel.
  * @param  pBuffer: Pointer to uint16_t array (size 8) to store measurements.
  * @retval uint8_t: OPTIC_SUCCESS, OPTIC_ERR_STICK_REMOVED, or FAIL_OPTIC_CH0..CH3
  *         if a channel's reading is out of the expected range.
  */
uint8_t DM_App_Optic_MeasureInitialStick(uint16_t* pBuffer)
{
    static uint8_t bCh;
    static uint8_t bIdx;
    const uint16_t awPWM_Set[2] = {199, 399};
    static uint8_t i;

    bIdx = 0;

    /* Loop through all 4 optical channels */
    for (bCh = 0; bCh < (uint8_t)OPTIC_CH_MAX; bCh++)
    {
        /* Guard-rail: check once per channel. Uses DM_App_Optic_Check_StickPresent()
           directly (not DM_App_Stick_Get_Status()) for an always-fresh reading -
           calls here are already spaced ~1.2s apart (2 measurements/channel),
           wider than the Get_Status() throttle window, so bypassing its cache
           costs nothing extra and avoids acting on a stale cached value. */
        if (!DM_App_Optic_Check_StickPresent())
        {
            return OPTIC_ERR_STICK_REMOVED;
        }

        /* For each channel, measure at 2 PWM levels */
        for (i = 0; i < 2; i++)
        {
            /* Measure using the set of hardware and PWM value */
            pBuffer[bIdx] = DM_App_Optic_Measure((OPTIC_CH_t)bCh, awPWM_Set[i]);
            bIdx++;
        }

        /* Validate this channel's readings against the expected range at each
           duty level. Out of range at either level fails this channel. */
        if ((pBuffer[bCh * 2] < OPTIC_INITIAL_MIN_50) || (pBuffer[bCh * 2] > OPTIC_INITIAL_MAX_50) ||
            (pBuffer[bCh * 2 + 1] < OPTIC_INITIAL_MIN_100) || (pBuffer[bCh * 2 + 1] > OPTIC_INITIAL_MAX_100))
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
  * @brief  Tunes PWM values for all channels to reach a target ADC value within tolerance.
  * @param  pInitBuffer: Pointer to the 8 initial measurement values.
  * @param  wTargetADC: The target ADC value to reach.
  * @retval uint8_t: OPTIC_SUCCESS or OPTIC_ERR_STICK_REMOVED.
  */
uint8_t DM_App_Optic_TuneTargetADC(uint16_t* pInitBuffer, uint16_t wTargetADC)
{
    static uint8_t bCh;
    uint16_t wADC_Low, wADC_High;
    int32_t wCurrentPWM;
    uint16_t wMeasured;
    int32_t iError;
    static uint8_t bIter;
    int32_t iSlope_Inv;

    for (bCh = 0; bCh < (uint8_t)OPTIC_CH_MAX; bCh++)
    {
        /* Guard-rail: check once per channel (not per tuning iteration), using
           DM_App_Optic_Check_StickPresent() directly for an always-fresh
           reading - calls are spaced up to ~3s apart (5 iterations/channel),
           so there's no throttle-cache benefit to gain here, only staleness
           risk to avoid. */
        if (!DM_App_Optic_Check_StickPresent())
        {
            return OPTIC_ERR_STICK_REMOVED;
        }

        wADC_Low = pInitBuffer[bCh * 2];
        wADC_High = pInitBuffer[bCh * 2 + 1];

        /* Calculate initial slope (inverse form: PWM delta / ADC delta) */
        if (wADC_High > wADC_Low)
        {
            iSlope_Inv = (399 - 199); // PWM Delta
            /* Compute initial PWM estimate via linear interpolation */
            wCurrentPWM = 199 + (int32_t)(wTargetADC - wADC_Low) * iSlope_Inv / (int32_t)(wADC_High - wADC_Low);
        }
        else
        {
            iSlope_Inv = 0;
            wCurrentPWM = 199; /* Default value when initial calibration data is invalid */
        }

        /* Tuning loop: up to 5 attempts to converge precisely */
        for (bIter = 0; bIter < 5; bIter++)
        {
            /* Clamp PWM to valid range (0 ~ 399) */
            if (wCurrentPWM < 0) wCurrentPWM = 0;
            if (wCurrentPWM > 399) wCurrentPWM = 399;

            /* Measure ADC at the current PWM (~200ms) */
            wMeasured = DM_App_Optic_Measure((OPTIC_CH_t)bCh, (uint16_t)wCurrentPWM);

            iError = (int32_t)wTargetADC - (int32_t)wMeasured;

            /* Check whether the result is within tolerance */
            if ((iError <= OPTIC_TUNE_TOLERANCE) && (iError >= -OPTIC_TUNE_TOLERANCE))
            {
                break;
            }

            /* Still out of tolerance after 5 attempts: return error */
            if (bIter == 4)
            {
                return (uint8_t)(FAIL_OPTIC_CH0 + bCh);
            }

            /* [Key] Recompute PWM from the measured result for the next iteration */
            if (iSlope_Inv > 0)
            {
                /* Proportional control (P-control): adjust PWM by the current error */
                wCurrentPWM += (iError * iSlope_Inv) / (int32_t)(wADC_High - wADC_Low);
            }
            else
            {
                /* No slope info available: adjust by a fixed step */
                wCurrentPWM += (iError > 0) ? 5 : -5;
            }
        }

        /* Store the final tuned PWM value */
        gs_wOptic_PWM[bCh] = (uint16_t)wCurrentPWM;
    }

    return OPTIC_SUCCESS;
}

/**
  * @brief  Measures Empty PTR values for all 4 optic channels using the currently tuned PWM.
  * @param  pBuffer: Pointer to uint16_t array (size 4) to store measurements.
  * @retval uint8_t: OPTIC_SUCCESS or OPTIC_ERR_STICK_REMOVED.
  */
uint8_t DM_App_Optic_MeasureEmptyPTR(uint16_t* pBuffer)
{
    static uint8_t bCh;

    for (bCh = 0; bCh < (uint8_t)OPTIC_CH_MAX; bCh++)
    {
        /* Guard-rail: check once per channel, using DM_App_Optic_Check_StickPresent()
           directly for an always-fresh reading (calls are ~0.6s apart here). */
        if (!DM_App_Optic_Check_StickPresent())
        {
            return OPTIC_ERR_STICK_REMOVED;
        }

        /* Measure using the currently tuned PWM for each channel */
        pBuffer[bCh] = DM_App_Optic_Measure((OPTIC_CH_t)bCh, gs_wOptic_PWM[bCh]);
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
    uint16_t wADC = DM_App_Optic_Measure(OPTIC_CH_3, 399);

    return (wADC < OPTIC_STICK_REMOVE_THRESHOLD) ? 0 : 1;
}

//LED test
void LED_ON_Test(void)
{
    /* 1. Setup PWM Duty from parameter */
    DM_HW_Drv_LED_Duty_Set(399);

    /* 2. Turn ON the mapped LED (Mutual exclusion handled in Drv) */
    DM_HW_Drv_LED_Control(gs_tOptic_LED[0], ENABLE);

    /* 3. Start PWM Output */
    DM_HW_Drv_LED_PWM_Start();
    
    DM_HW_Drv_SystemSleep_10ms();

}



/************************ (C) COPYRIGHT Surearly Multi *****END OF FILE****/
