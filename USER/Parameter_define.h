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

#define _nTAGET_LIGHT                   1200   // Target ADC count for LED tuning

/* Gain is an integer percent. Cal_Ratio() divides by 100 internally, so a
   different scale here would need a different apply helper. */
#define OPTIC_GAIN_SCALE                100

/* Allowed deviation of the gain-applied value from _nTAGET_LIGHT. Rounding the
   gain still leaves up to ~20.5 of error near the top of the 1500..3900 range. */
#define OPTIC_TUNED_TOLERANCE           25

/* Stick presence check via OPTIC_CH_3 (innermost sensor, LED1) at 100% drive.
   A genuinely removed stick leaves no reflective body in front of this channel,
   so the ADC reading collapses well below a normally-inserted stick's value.
   Compared against the raw reading: after gain every channel lands near
   _nTAGET_LIGHT and the present/removed gap disappears. */
#define OPTIC_STICK_REMOVE_THRESHOLD    2600    /**< Midpoint of measured 1500 (absent) / 3700 (present) */
#define OPTIC_STICK_CHECK_INTERVAL_MS   250     /**< Minimum interval between re-measurements (throttles LED/PTR cycling) */

/* Acceptable range for DM_App_Optic_MeasureInitialStick()'s per-channel reading.
   Raw (pre-gain) scale; outside => that channel is reported as ERROR_OPTIC_FAIL_CHx. */
#define OPTIC_INITIAL_MIN_100           1500    /**< Min acceptable raw ADC @ 100% */
#define OPTIC_INITIAL_MAX_100           3900    /**< Max acceptable raw ADC @ 100% */

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
