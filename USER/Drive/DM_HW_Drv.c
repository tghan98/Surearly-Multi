/**
  ******************************************************************************
  * @file           : DM_HW_Drv.c
  * @brief          : Hardware Driver Implementation for MFx System
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "DM_HW_Drv.h"

#include "User_Main.h"

/* Private variables ---------------------------------------------------------*/
static volatile uint8_t gs_bADC_Conv_Done = 0;

/* Functions -----------------------------------------------------------------*/

/**
  * @brief  Sets the ADC conversion completion flag.
  */
void DM_HW_Drv_ADC_SetConvDone(void)
{
    gs_bADC_Conv_Done = 1;
}

/**
  * @brief  Returns the status of the ADC conversion completion flag.
  */
uint8_t DM_HW_Drv_ADC_GetConvDone(void)
{
    return gs_bADC_Conv_Done;
}

/**
  * @brief  Clears the ADC conversion completion flag.
  */
void DM_HW_Drv_ADC_ClearConvDone(void)
{
    gs_bADC_Conv_Done = 0;
}

/**
  * @brief  Initializes the system clock to 2MHz (HSI 16MHz / 8).
  * @param  None
  * @retval None
  */
void DM_HW_Drv_SystemClock_Init(void)
{
    /* Use HSI as system clock source, divided by 8 to set it to 2MHz */
    CLK_SYSCLKDivConfig(CLK_SYSCLKDiv_8);
    
    /* Enable peripheral clocks (essential peripherals) */
    /* TIM2 (LED PWM) removed on v1.1 board - no LED_PWM pin. */
    CLK_PeripheralClockConfig(CLK_Peripheral_TIM4, ENABLE);   /* For 10ms system timer */
    CLK_PeripheralClockConfig(CLK_Peripheral_ADC1, ENABLE);   /* For sensor measurement */
    CLK_PeripheralClockConfig(CLK_Peripheral_USART1, ENABLE); /* For communication */
}

/**
  * @brief  Initializes the initial state of all GPIO pins used in the system.
  * @param  None
  * @retval None
  */
void DM_HW_Drv_GPIO_Init(void)
{
    /* 1. LED Configuration */
    /* nLED1 (PB4), nLED2 (PB3): cathode drive, Push-Pull. Init High = OFF. */
    GPIO_Init(LED1_GPIO_PORT, LED1_GPIO_PIN, GPIO_Mode_Out_PP_High_Fast);
    GPIO_Init(LED2_GPIO_PORT, LED2_GPIO_PIN, GPIO_Mode_Out_PP_High_Fast);

    /* 2. LCD Configuration (Default Low) */
    GPIO_Init(LCD_COM0_GPIO_PORT, LCD_COM0_GPIO_PIN, GPIO_Mode_Out_PP_Low_Slow);
    
    GPIO_Init(LCD_SEG0_GPIO_PORT, LCD_SEG0_GPIO_PIN, GPIO_Mode_Out_PP_Low_Slow);
    GPIO_Init(LCD_SEG1_GPIO_PORT, LCD_SEG1_GPIO_PIN, GPIO_Mode_Out_PP_Low_Slow);
    GPIO_Init(LCD_SEG2_GPIO_PORT, LCD_SEG2_GPIO_PIN, GPIO_Mode_Out_PP_Low_Slow);
    GPIO_Init(LCD_SEG3_GPIO_PORT, LCD_SEG3_GPIO_PIN, GPIO_Mode_Out_PP_Low_Slow);
    
    
    
    /* 3. USART Configuration */
    /* TX (PA2): Output Push-Pull High Fast */
    GPIO_Init(USART_PORT, USART_TX_PIN, GPIO_Mode_Out_PP_High_Fast);
    /* RX (PA3): Input Pull-up */
    GPIO_Init(USART_PORT, USART_RX_PIN, GPIO_Mode_In_PU_No_IT);

    /* 4. ADC Configuration (PB5, PB6, PB7) */
    GPIO_Init(ADC_FRNT_PORT, ADC_FRNT_PIN, GPIO_Mode_In_FL_No_IT);
    GPIO_Init(ADC_MIDL_PORT, ADC_MIDL_PIN, GPIO_Mode_In_FL_No_IT);
    GPIO_Init(ADC_REAR_PORT, ADC_REAR_PIN, GPIO_Mode_In_FL_No_IT);

    /* 5. PTR Power removed on v1.1 board (PTR collector wired directly to VCC). */

    /* 6. Stick Check (STP_CK, PB1) */
    /* Initially set as standard input to prevent accidental wakeups during boot */
    DM_HW_Drv_STP_CK_Set_GPIO_Input();
}

/**
  * @brief  Controls specific LED channel (LED1, LED2).
  * @note   v1.1 board: nLED1/nLED2 (PB4/PB3) are the cathode drive, Push-Pull.
  *         ENABLE turns LED ON (Low, sink), DISABLE turns LED OFF (High).
  *         Strict mutual exclusion is applied: Turning ON one LED will turn OFF the other.
  * @param  tCh: LED channel selection (CH_LED_1 or CH_LED_2).
  * @param  NewState: Target state (ENABLE or DISABLE).
  * @retval None
  */
void DM_HW_Drv_LED_Control(LED_CH_t tCh, FunctionalState NewState)
{
    if (NewState == ENABLE)
    {
        /* Guard-rail: Turn OFF both first or ensure the other is OFF to maintain exclusivity */
        if (tCh == CH_LED_1)
        {
            GPIO_SetBits(LED2_GPIO_PORT, LED2_GPIO_PIN);    /* Ensure LED2 is OFF (High) */
            GPIO_ResetBits(LED1_GPIO_PORT, LED1_GPIO_PIN);  /* Turn ON LED1 (Low, sink) */
        }
        else if (tCh == CH_LED_2)
        {
            GPIO_SetBits(LED1_GPIO_PORT, LED1_GPIO_PIN);    /* Ensure LED1 is OFF (High) */
            GPIO_ResetBits(LED2_GPIO_PORT, LED2_GPIO_PIN);  /* Turn ON LED2 (Low, sink) */
        }
    }
    else
    {
        /* Simply turn OFF the selected channel (High) */
        if (tCh == CH_LED_1)
        {
            GPIO_SetBits(LED1_GPIO_PORT, LED1_GPIO_PIN);
        }
        else if (tCh == CH_LED_2)
        {
            GPIO_SetBits(LED2_GPIO_PORT, LED2_GPIO_PIN);
        }
    }
}

/**
  * @brief  Initializes TIM4 for 10ms System Tick (Wakeup timer).
  * @note   Nominal calc (assumes exact 2MHz fMASTER): System Clock = 2MHz,
  *         Target = 100Hz (10ms), Prescaler = 128,
  *         ARR = (2,000,000 / 128 / 100) - 1 = 155.25 -> 155.
  *         An earlier fix used ARR=144, calibrated from a Sample_Load_Wait
  *         measurement (30000-tick / 5-minute wait via GetSystemTick()) that
  *         mixed in ADC untracked-tick compensation (DM_App_Optic_Measure()
  *         calls), so it wasn't a clean read of the pure TIM4 rate.
  *         ARR=149 is empirically corrected from a clean ERROR_WAIT
  *         (ERR_RUN_TIME, 12000-tick / 2-minute wait) measurement instead -
  *         pure TIM4 ticks, no ADC involved. Confirmed via real-hardware
  *         re-test. Being calibrated to a single room-temperature
  *         measurement, it may drift slightly with temperature (see HSI
  *         accuracy table in the MCU datasheet) - re-tune if a future
  *         hardware measurement shows further drift.
  * @param  None
  * @retval None
  */
void DM_HW_Drv_SystemTick_TIM4_Init(void)
{
    /* Time Base configuration: Prescaler = 128, ARR = 149 (empirically
       corrected for actual measured clock rate - see note above) */
    TIM4_TimeBaseInit(TIM4_Prescaler_128, 149);

    /* Enable Update Interrupt */
    TIM4_ITConfig(TIM4_IT_Update, ENABLE);

    /* TIM4 remains disabled initially as requested */
    TIM4_Cmd(DISABLE);
}

/* Inner-loop repeats per requested microsecond. PLACEHOLDER - Stage 2 must
   re-measure on real hardware (scope an LED pin) since fMASTER=2MHz gives only
   0.5us/cycle and the true loop cost depends on the IAR build. */
#define DELAY_US_CAL    1

/**
  * @brief  Blocking busy-wait for the LED on-time (approximate).
  * @note   Interrupts are left as-is: during a measurement TIM4 is disabled
  *         (only enabled inside DM_HW_Drv_SystemSleep_10ms()), so no LCD-refresh
  *         tick perturbs this delay. The intended time is folded into the system
  *         tick for the 5-minute wait accounting.
  * @param  wMicroseconds: On-time to wait, in microseconds.
  * @retval None
  */
void DM_HW_Drv_Delay_us(uint16_t wMicroseconds)
{
    uint16_t wUs;
    volatile uint16_t n;

    for (wUs = wMicroseconds; wUs > 0; wUs--)
    {
        for (n = 0; n < DELAY_US_CAL; n++)
        {
            /* busy spin - calibrate DELAY_US_CAL in Stage 2 */
        }
    }

    SystemTick_AddUntracked_us(wMicroseconds);
}

/**
  * @brief  Enters Core Sleep mode for 10ms using TIM4.
  * @note   TIM4 is enabled before sleep and disabled after wakeup.
  * @param  None
  * @retval None
  */
void DM_HW_Drv_SystemSleep_10ms(void)
{
  static uint32_t dwTickCheck = 0;
    /* 1. Ensure TIM4 counter starts from 0 */
    TIM4_SetCounter(0);
    
    /* 2. Clear pending interrupt flag to prevent immediate wakeup */
    TIM4_ClearITPendingBit(TIM4_IT_Update);
    
    /* 3. Enable TIM4 */
    TIM4_Cmd(ENABLE);
    
    /* 4. Enter Wait Mode (Core Sleep)
       The CPU stops here until an interrupt (TIM4) occurs. */
    
    dwTickCheck = GetSystemTick();
    
    while(dwTickCheck >= GetSystemTick())
    {
      wfi();
    }
    
    /* 5. Disable TIM4 after wakeup to save power and stay in Disable state */
    TIM4_Cmd(DISABLE);
}

/**
  * @brief  Initializes ADC1 for sensor measurement.
  * @note   Uses 12-bit resolution and maximum sampling time (384 cycles)
  *         due to 100K Pull-down and 22nF capacitor (High RC constant).
  * @param  None
  * @retval None
  */
void DM_HW_Drv_ADC_Init(void)
{
    /* 1. Configure ADC1: Single Conversion, 12-bit, No clock division */
    ADC_Init(ADC1, ADC_ConversionMode_Single, ADC_Resolution_12Bit, ADC_Prescaler_1);
    
    /* 2. Sampling time = 16 cycles (v1.1 board). The node cap (C12~C14, tens of
       nF) supplies the S/H charge, so the 100k source does not gate sampling;
       short sampling keeps the burst << on-time (see plan 2-1). Verify against
       384 cycles in Stage 2. */
    ADC_SamplingTimeConfig(ADC1, ADC_Group_SlowChannels, ADC_SamplingTime_16Cycles);
    
    /* 3. Wake up ADC from Power Down mode */
    ADC_Cmd(ADC1, ENABLE);
}

  void DM_HW_Drv_ADC_SetSamplingTime(ADC_Group_TypeDef tGroup,
                     ADC_SamplingTime_TypeDef tSamplingTime)
  {
    ADC_SamplingTimeConfig(ADC1, tGroup, tSamplingTime);
  }

/**
  * @brief  Selects an ADC channel for a burst of reads (see header for rationale).
  * @param  ADC_Channel: The channel to select.
  * @retval None
  */
void DM_HW_Drv_ADC_ChannelSelect(ADC_Channel_TypeDef ADC_Channel)
{
    /* Enable the specified ADC channel (EOC is polled, not interrupt-driven) */
    ADC_ChannelCmd(ADC1, ADC_Channel, ENABLE);
}

/* Sampling 16 cycles -> nominal (12+16)/2MHz ~= 14us per read. TENTATIVE: must
   be re-measured on real hardware in Stage 4 (feeds the 5-minute wait tick
   accounting, see GetSystemTick()). */
#define ADC_READ_TIME_US    14

/**
  * @brief  Performs a single 12-bit conversion on the currently selected channel.
  * @note   Busy-polls the EOC flag (no wfi/EOC interrupt).
  * @param  None
  * @retval 12-bit ADC conversion result.
  */
uint16_t DM_HW_Drv_ADC_Read(void)
{
    uint16_t wADC_Result;

    /* Start Software Conversion */
    ADC_SoftwareStartConv(ADC1);

    /* Busy-poll the EOC flag until the conversion completes */
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET)
    {
    }

    /* Read Result (reading clears the EOC flag) */
    wADC_Result = ADC_GetConversionValue(ADC1);

    /* Account for this conversion's real elapsed time (see GetSystemTick()) */
    SystemTick_AddUntracked_us(ADC_READ_TIME_US);

    return wADC_Result;
}

/**
  * @brief  Deselects an ADC channel after a burst of reads, to save power.
  * @param  ADC_Channel: The channel to deselect.
  * @retval None
  */
void DM_HW_Drv_ADC_ChannelDeselect(ADC_Channel_TypeDef ADC_Channel)
{
    /* Disable the channel to save power */
    ADC_ChannelCmd(ADC1, ADC_Channel, DISABLE);
}

/**
  * @brief  Initializes USART1 for communication.
  * @note   9600 Baud, 8 bits, 1 Stop bit, No parity.
  *         RX: Interrupt-based, TX: Polling-based.
  * @param  None
  * @retval None
  */
void DM_HW_Drv_USART_Init(void)
{
    /* USART1's true default pins are PC3(TX)/PC2(RX). Remap it to PA2(TX)/
       PA3(RX) to match the GPIO configuration in DM_HW_Drv_GPIO_Init(). */
    SYSCFG_REMAPPinConfig(REMAP_Pin_USART1TxRxPortA, ENABLE);

    /* Deinitialize USART1 to start from a clean state */
    USART_DeInit(USART1);

    /* Configure USART1: 9600 Baud, 8 bits, 1 Stop, No Parity, Both TX/RX enabled */
    USART_Init(USART1, 
               (uint32_t)9600, 
               USART_WordLength_8b, 
               USART_StopBits_1, 
               USART_Parity_No, 
               (USART_Mode_TypeDef)(USART_Mode_Tx | USART_Mode_Rx));

    /* Enable Receive Data Register Not Empty (RXNE) Interrupt */
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    /* Enable USART1 */
    USART_Cmd(USART1, ENABLE);
}

/**
  * @brief  Sends a single byte via USART1 using polling.
  * @param  bData: Byte to transmit.
  * @retval None
  */
void DM_HW_Drv_USART_SendByte(uint8_t bData)
{
    /* Wait for Transmission Complete (TC) flag to be set */
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);

    /* Send the byte */
    USART_SendData8(USART1, bData);
}

/**
  * @brief  Sends a 16-bit value as 2 raw bytes via USART1 (High byte first, no ASCII conversion).
  * @param  wVal: Value to send.
  * @retval None
  */
void DM_HW_Drv_USART_SendWord(uint16_t wVal)
{
    DM_HW_Drv_USART_SendByte((uint8_t)(wVal >> 8));   /* High byte */
    DM_HW_Drv_USART_SendByte((uint8_t)(wVal & 0xFF)); /* Low byte */
}

/**
  * @brief  Enters Halt mode and waits for a stick insertion (Falling edge on PB1).
  * @param  None
  * @retval None
  */
void DM_HW_Drv_SystemHalt_WakeupOnStick(void)
{
    /* Configure STP_CK as Interrupt input */
    GPIO_Init(STP_CK_PORT, STP_CK_PIN, GPIO_Mode_In_PU_IT);

    /* STM8L requires global interrupts disabled when writing EXTI sensitivity registers */
    disableInterrupts();
    EXTI_SetPinSensitivity(EXTI_Pin_1, EXTI_Trigger_Falling);
    enableInterrupts();

    /* Enter Halt Mode */
    //halt();
    wfi();
}

/**
  * @brief  Configures the Stick Check pin (PB1) as a standard GPIO Input (No IT).
  * @param  None
  * @retval None
  */
void DM_HW_Drv_STP_CK_Set_GPIO_Input(void)
{
    /* Configure PB1 as Input Pull-up without Interrupt */
    GPIO_Init(STP_CK_PORT, STP_CK_PIN, GPIO_Mode_In_PU_No_IT);
}

/**
  * @brief  Reads the current status of the Stick Check pin.
  * @param  None
  * @retval BitStatus: SET (High, Stick Removed) or RESET (Low, Stick Inserted).
  */
BitStatus DM_HW_Drv_STP_CK_Get_Status(void)
{
    return GPIO_ReadInputDataBit(STP_CK_PORT, STP_CK_PIN);
}

/**
  * @brief  Prepares the system for Ultra-Low Power consumption before entering Halt.
  * @param  None
  * @retval None
  */
void DM_HW_Drv_Power_PrepareSleep(void)
{
    /* 1. Ensure all LEDs are OFF (Push-Pull: High = OFF) */
    GPIO_SetBits(LED1_GPIO_PORT, LED1_GPIO_PIN);
    GPIO_SetBits(LED2_GPIO_PORT, LED2_GPIO_PIN);

    /* 2. Ensure LCD pins are LOW to avoid ghosting/leakage */
    GPIO_ResetBits(LCD_COM0_GPIO_PORT, LCD_COM0_GPIO_PIN);
    GPIO_ResetBits(LCD_SEG0_GPIO_PORT, LCD_SEG0_GPIO_PIN);
    GPIO_ResetBits(LCD_SEG1_GPIO_PORT, LCD_SEG1_GPIO_PIN);
    GPIO_ResetBits(LCD_SEG2_GPIO_PORT, LCD_SEG2_GPIO_PIN);
    GPIO_ResetBits(LCD_SEG3_GPIO_PORT, LCD_SEG3_GPIO_PIN);

    /* 3. Disable ADC */
    ADC_Cmd(ADC1, DISABLE);
    
    /* 4. Disable USART */
    USART_Cmd(USART1, DISABLE);

    /* 5. Enable Ultra-Low Power mode for the internal regulator during Halt */
    PWR_UltraLowPowerCmd(ENABLE);
    
    /* 6. Disable all peripheral clocks to minimize consumption (TIM2 unused) */
    CLK_PeripheralClockConfig(CLK_Peripheral_TIM4, DISABLE);
    CLK_PeripheralClockConfig(CLK_Peripheral_ADC1, DISABLE);
    CLK_PeripheralClockConfig(CLK_Peripheral_USART1, DISABLE);
}

/**
  * @brief  Resumes system peripherals after waking up from Halt mode.
  * @param  None
  * @retval None
  */
void DM_HW_Drv_Power_Resume(void)
{
    /* 1. Re-initialize System Clock and re-enable essential peripheral clocks */
    DM_HW_Drv_SystemClock_Init();

    /* 2. Disable Ultra-Low Power mode to restore normal operation */
    PWR_UltraLowPowerCmd(DISABLE);

    /* 3. Re-initialize ADC and USART as they were disabled during sleep */
    DM_HW_Drv_GPIO_Init();
    DM_HW_Drv_ADC_Init();
    DM_HW_Drv_USART_Init();
}

/**
  * @brief  Writes a block of data to the internal EEPROM.
  * @param  wAddrOffset: Address offset from the start of EEPROM (0 to 255).
  * @param  pBuffer: Pointer to the data buffer to write.
  * @param  wLength: Number of bytes to write.
  * @retval None
  */
void DM_HW_Drv_EEPROM_Write(uint16_t wAddrOffset, uint8_t* pBuffer, uint16_t wLength)
{
    static uint16_t i;
    uint32_t dwBaseAddr = EEPROM_START_ADDR;
    
    /* Unlock Data EEPROM */
    FLASH_Unlock(FLASH_MemType_Data);

    for (i = 0; i < wLength; i++)
    {
        /* Check boundary (256 bytes total) */
        if ((wAddrOffset + i) < 256)
        {
            FLASH_ProgramByte(dwBaseAddr + wAddrOffset + i, pBuffer[i]);
            
            /* Wait for the end of programming */
            FLASH_WaitForLastOperation(FLASH_MemType_Data);
        }
    }

    /* Lock Data EEPROM again to protect data */
    FLASH_Lock(FLASH_MemType_Data);
}

/**
  * @brief  Reads a block of data from the internal EEPROM.
  * @param  wAddrOffset: Address offset from the start of EEPROM (0 to 255).
  * @param  pBuffer: Pointer to the buffer to store the read data.
  * @param  wLength: Number of bytes to read.
  * @retval None
  */
void DM_HW_Drv_EEPROM_Read(uint16_t wAddrOffset, uint8_t* pBuffer, uint16_t wLength)
{
    static uint16_t i;
    uint32_t dwBaseAddr = EEPROM_START_ADDR;

    for (i = 0; i < wLength; i++)
    {
        /* Check boundary */
        if ((wAddrOffset + i) < 256)
        {
            pBuffer[i] = FLASH_ReadByte(dwBaseAddr + wAddrOffset + i);
        }
    }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
