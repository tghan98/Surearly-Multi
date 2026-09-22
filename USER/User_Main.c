/**
  ******************************************************************************
  * @file           : User_Main.c
  * @brief          : Main User Application Logic
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "User_Main.h"
#include "DM_HW_Drv.h"
#include "DM_LCD_Stick_Check_App.h"
#include "DM_Main_Sq_App.h"

#include "Parameter_define.h"

/* Private variables ---------------------------------------------------------*/
static volatile uint32_t gs_dwSystemTick_10ms = 0;   /* Written only by the TIM4 ISR */
static uint32_t gs_dwUntrackedTime_us = 0;            /* Written only by foreground code; remainder < 10000 */
static uint32_t gs_dwUntrackedTicks = 0;               /* Written only by foreground code; whole ticks folded in */

/* Functions -----------------------------------------------------------------*/

/**
  * @brief  Returns the current system tick count (10ms unit).
  * @note   Plain addition only - no division here. gs_dwUntrackedTicks already
  *         holds whole ticks (see SystemTick_AddUntracked_us()); a 32-bit
  *         division has no STM8 hardware support and previously ran here on
  *         every call (this function is called ~3x per
  *         DM_HW_Drv_SystemSleep_10ms(), which itself runs ~30000x per 5-minute
  *         wait) - measured on real hardware as ~33-35s of extra real time
  *         over a 5-minute wait from that alone.
  * @param  None
  * @retval Current system tick value.
  */
uint32_t GetSystemTick(void)
{
    return gs_dwSystemTick_10ms + gs_dwUntrackedTicks;
}

/**
  * @brief  Accumulates known, deterministic elapsed time (see header for
  *         rationale and why this never writes gs_dwSystemTick_10ms).
  * @note   The 10000us->1 tick fold-in uses a subtract loop, not division -
  *         cheap here since this runs far less often (once per
  *         DM_HW_Drv_ADC_Read() call) than GetSystemTick() does.
  * @param  wMicroseconds: Known elapsed time to add, in microseconds.
  * @retval None
  */
void SystemTick_AddUntracked_us(uint16_t wMicroseconds)
{
    gs_dwUntrackedTime_us += wMicroseconds;

    while (gs_dwUntrackedTime_us >= 10000)
    {
        gs_dwUntrackedTime_us -= 10000;
        gs_dwUntrackedTicks++;
    }
}

/**
  * @brief  User-level initialization. Called once from main.c.
  * @param  None
  * @retval 0: Success, Others: Error code
  */
int32_t User_Main_Init(void)
{
    /* Initialize Hardware */
    DM_HW_Drv_SystemClock_Init();
    DM_HW_Drv_GPIO_Init();

    /* Initialize Peripherals */
    DM_HW_Drv_SystemTick_TIM4_Init();
    DM_HW_Drv_ADC_Init();
    DM_HW_Drv_USART_Init();
    
    /* Enable Global Interrupts */
    enableInterrupts();
    
    //LED_ON_Test();

    return 0;
}

/**
  * @brief  User-level main loop. Called repeatedly from main.c.
  * @param  None
  * @retval 0: Success, Others: Error code
  */
int32_t User_Main_Run(void)
{
    /* 1) Main Loop Sequence Step Process */
    /* Handlers: ADC, Logic Control, Stick Insertion State */
    DM_App_Main_Sq_Process();
    
    return 0;
}

/* Interrupt Handlers --------------------------------------------------------*/

/**
  * @brief  TIM4 Update/Overflow/Trigger Interrupt routine.
  * @note   Occurs every 10ms as configured in DM_HW_Drv_SystemTick_TIM4_Init.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(TIM4_UPD_OVF_TRG_IRQHandler, 25)
{
    /* Clear TIM4 Update Interrupt Pending Bit */
    TIM4_ClearITPendingBit(TIM4_IT_Update);
    
    /* Increment System 10ms Tick */
    gs_dwSystemTick_10ms++;

    /* 2) LCD Sequence Animation Process (10ms 주기) */
    DM_App_LCD_Sq_Process();

    /* Refresh LCD Pins (Phase inversion and Blinking) */
    //if(DM_App_Main_Sq_Get_Step() != MAIN_SQ_IDLE) DM_App_LCD_Refresh();
    DM_App_LCD_Refresh();
}

/**
  * @brief  ADC1 / Comparator Interrupt routine.
  * @note   Used to wake up from sleep during ADC conversion.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(ADC1_COMP_IRQHandler, 18)
{
    /* Clear ADC1 End of Conversion (EOC) Interrupt Pending Bit */
    ADC_ClearITPendingBit(ADC1, ADC_IT_EOC);
    
    /* Set global flag via setter function to notify that conversion is complete */
    DM_HW_Drv_ADC_SetConvDone();
}

/**
  * @brief  External IT PIN1 Interrupt routine.
  * @note   Triggered by Stick Insertion (PB1 Falling Edge).
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(EXTI1_IRQHandler, 9)
{
    /* Clear Interrupt Pending Bit for Pin 1 */
    EXTI_ClearITPendingBit(EXTI_IT_Pin1);
}

/**
  * @brief  USART1 RX Interrupt routine.
  * @note   Occurs when data is received in the USART1 RX register.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(USART1_RX_TIM5_CC_IRQHandler, 28)
{
    uint8_t bReceivedData = 0;
    static uint8_t bCommandChar = 0;

    /* Check if RXNE (Receive Data Register Not Empty) interrupt occurred */
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        /* Read the received byte (Clears the RXNE flag) */
        bReceivedData = USART_ReceiveData8(USART1);

        /* Detect 'M'/'m' (dump) or 'R'/'r' (reset test complete count) */
        if (bReceivedData == 'M' || bReceivedData == 'm' ||
            bReceivedData == 'R' || bReceivedData == 'r')
        {
            bCommandChar = bReceivedData;
        }
        /* Detect 'Enter' (CR or LF) */
        else if (bReceivedData == 0x0D || bReceivedData == 0x0A)
        {
            if (bCommandChar == 'M' || bCommandChar == 'm')
            {
                /* Command recognized: Send raw EEPROM dump */
                DM_App_Main_Sq_Send_RawDump();
            }
            else if (bCommandChar == 'R' || bCommandChar == 'r')
            {
                /* Command recognized: Reset test complete count */
                DM_App_Main_Sq_Reset_TestCompleteCount();
            }
            bCommandChar = 0; /* Reset */
        }
        else
        {
            /* Any other character resets the command buffer */
            bCommandChar = 0;
        }
    }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
