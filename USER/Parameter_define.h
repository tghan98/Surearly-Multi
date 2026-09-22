/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __PARAMETER_HD__
#define __PARAMETER_HD__

//------------------------------------------------------------------------------
// Firmware Version
#define _strFIRMWARE_VER        "v2.1"           // 4 byteVersion shown externally (display/label) - not the real build version
#define _REAL_FIRMWARE_VER      "v2.1.1.2"       // 8 byte Actual build version (the one that matters for tracking)
//------------------------------------------------------------------------------

#define _strLOT_UPPER_4CHAR             "HCGM"  // LOT number - 4 letter prefix
#define _strLOT_LOWER_5NUM              "19001" // LOT number - 5 digit suffix

#define _nT_LINE_INT_THRESHOLD          1040    // T line intensity threshold
#define _nC_LINE_INT_THRESHOLD          1050    // C line intensity threshold

#define _nTAGET_LIGHT                   800    // Target ADC count for LED tuning

#define OPTIC_TUNE_TOLERANCE        15  /**< Default tolerance for ADC tuning */

/* Stick presence check via OPTIC_CH_3 (innermost sensor, LED1) at PWM 399 (Duty 100%).
   A genuinely removed stick leaves no reflective body in front of this channel,
   so the ADC reading collapses well below a normally-inserted stick's value. */
#define OPTIC_STICK_REMOVE_THRESHOLD    1000    /**< Below this ADC value @ Duty100% => stick considered removed */
#define OPTIC_STICK_CHECK_INTERVAL_MS   250     /**< Minimum interval between re-measurements (throttles LED/PTR cycling) */

/* Acceptable range for DM_App_Optic_MeasureInitialStick()'s per-channel readings.
   Outside either range => that channel is reported as ERROR_OPTIC_FAIL_CHx. */
#define OPTIC_INITIAL_MIN_50            900     /**< Min acceptable ADC @ Duty 50% (PWM199) */
#define OPTIC_INITIAL_MAX_50            2100    /**< Max acceptable ADC @ Duty 50% (PWM199) */
#define OPTIC_INITIAL_MIN_100           1800    /**< Min acceptable ADC @ Duty 100% (PWM399) */
#define OPTIC_INITIAL_MAX_100           4200    /**< Max acceptable ADC @ Duty 100% (PWM399) */

/* On-time (us) for LED measurement (v1.1 board - replaces PWM duty control).
   Stage 1 uses a fixed default; X1/X2/MAX are placeholders to be finalized from
   the Stage 2 on-time sweep. Choose X1 < target on-time < X2 to bracket the
   target (avoid extrapolation on the RC charging curve). */
#define OPTIC_ONTIME_DEFAULT            800     /**< Fixed on-time for Stage 1 bring-up */
#define OPTIC_ONTIME_MAX                1500    /**< Max on-time (stick-present check) - placeholder */
#define OPTIC_ONTIME_X1                 500     /**< 2-point interpolation low point - placeholder */
#define OPTIC_ONTIME_X2                 1000    /**< 2-point interpolation high point - placeholder */

#define _nSCAN_MAX_CNT                  30      // Max allowed device usage/scan count

#define _nEMPTY_BTB_RATIO               90

#define _nOVER_FLOW_RATIO               92

#define _nSMALL_SAMPLE_RATIO            120

//------------------------------------------------------------------------------
// Debug
// 1: Skip real measurement/EEPROM work in DM_Main_Sq_App step handlers and just
//    delay-then-advance, so only the LCD sequence animations can be checked.
#define _DEBUG_LCD_SEQ_ONLY             0
#define _DEBUG_SEQ_STEP_DELAY           500     // 10ms units (500 = 5 sec)
//------------------------------------------------------------------------------

#endif /* __PARAMETER_HD__ */

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
