/**
  ******************************************************************************
  * @file           : DM_HW_Drv.h
  * @brief          : Hardware Driver Header for MFx System
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DM_HW_DRV_H__
#define __DM_HW_DRV_H__

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/**
  * @brief LED Channel Enumeration
  */
typedef enum
{
    CH_LED_1 = 0,
    CH_LED_2,
    
    CH_LED_MAX
} LED_CH_t;

/* Exported Functions --------------------------------------------------------*/

/**
  * @brief  Sets the ADC conversion completion flag.
  * @param  None
  * @retval None
  */
void DM_HW_Drv_ADC_SetConvDone(void);

/**
  * @brief  Returns the status of the ADC conversion completion flag.
  * @param  None
  * @retval 1 if done, 0 otherwise.
  */
uint8_t DM_HW_Drv_ADC_GetConvDone(void);

/**
  * @brief  Clears the ADC conversion completion flag.
  * @param  None
  * @retval None
  */
void DM_HW_Drv_ADC_ClearConvDone(void);

/**
  * @brief  Initializes the system clock to 2MHz (HSI 16MHz / 8).
  * @param  None
  * @retval None
  */
void DM_HW_Drv_SystemClock_Init(void);

/**
  * @brief  Initializes the initial state of all GPIO pins used in the system.
  * @param  None
  * @retval None
  */
void DM_HW_Drv_GPIO_Init(void);

/**
  * @brief  Controls specific LED channel (LED1, LED2).
  * @note   nLED1/nLED2 (PB4/PB3) are the cathode drive, Push-Pull.
  *         ENABLE turns LED ON (Low, sink), DISABLE turns LED OFF (High).
  * @param  tCh: LED channel selection (CH_LED_1 or CH_LED_2).
  * @param  NewState: Target state (ENABLE or DISABLE).
  * @retval None
  */
void DM_HW_Drv_LED_Control(LED_CH_t tCh, FunctionalState NewState);

/**
  * @brief  Initializes TIM4 for 10ms System Tick (Wakeup timer).
  * @param  None
  * @retval None
  */
void DM_HW_Drv_SystemTick_TIM4_Init(void);

/**
  * @brief  Blocking busy-wait for the LED on-time, in microseconds (approximate).
  * @param  wMicroseconds: On-time to wait.
  * @retval None
  */
void DM_HW_Drv_Delay_us(uint16_t wMicroseconds);

/**
  * @brief  Enters Core Sleep mode for 10ms using TIM4.
  * @note   TIM4 is enabled before sleep and disabled after wakeup.
  * @param  None
  * @retval None
  */
void DM_HW_Drv_SystemSleep_10ms(void);

/**
  * @brief  Initializes ADC1 for sensor measurement.
  * @note   Uses 12-bit resolution and maximum sampling time (384 cycles).
  * @param  None
  * @retval None
  */
void DM_HW_Drv_ADC_Init(void);

void DM_HW_Drv_ADC_SetSamplingTime(ADC_Group_TypeDef tGroup,
                                   ADC_SamplingTime_TypeDef tSamplingTime);

/**
  * @brief  Selects an ADC channel for a burst of reads.
  * @note   Call once before a series of DM_HW_Drv_ADC_Read() calls on the same
  *         channel, not per-read - re-selecting every read would needlessly
  *         double the conversion count (see DM_HW_Drv_ADC_Read()).
  * @param  ADC_Channel: The channel to select (e.g., ADC_Channel_11, 12, 13).
  * @retval None
  */
void DM_HW_Drv_ADC_ChannelSelect(ADC_Channel_TypeDef ADC_Channel);

/**
  * @brief  Performs a single 12-bit conversion on the currently selected channel.
  * @note   DM_HW_Drv_ADC_ChannelSelect() must be called first. The first
  *         conversion right after selecting a channel can be unstable (mux/
  *         sample-and-hold settling); callers taking multiple samples per
  *         channel (e.g. DM_App_Optic_Measure()'s sort-and-trim) rely on that
  *         outlier rejection to absorb it rather than discarding it here.
  * @param  None
  * @retval 12-bit ADC conversion result.
  */
uint16_t DM_HW_Drv_ADC_Read(void);

/**
  * @brief  Deselects an ADC channel after a burst of reads, to save power.
  * @param  ADC_Channel: The channel to deselect (must match the one selected).
  * @retval None
  */
void DM_HW_Drv_ADC_ChannelDeselect(ADC_Channel_TypeDef ADC_Channel);

/**
  * @brief  Initializes USART1 for communication.
  * @note   9600 Baud, 8 bits, 1 Stop bit, No parity.
  *         RX: Interrupt-based, TX: Polling-based.
  * @param  None
  * @retval None
  */
void DM_HW_Drv_USART_Init(void);

/**
  * @brief  Sends a single byte via USART1 using polling.
  * @param  bData: Byte to transmit.
  * @retval None
  */
void DM_HW_Drv_USART_SendByte(uint8_t bData);

/**
  * @brief  Sends a 16-bit value as 2 raw bytes via USART1 (High byte first, no ASCII conversion).
  * @param  wVal: Value to send.
  * @retval None
  */
void DM_HW_Drv_USART_SendWord(uint16_t wVal);

/**
  * @brief  Enters Halt mode and waits for a stick insertion (Falling edge on PB1).
  * @param  None
  * @retval None
  */
void DM_HW_Drv_SystemHalt_WakeupOnStick(void);

/**
  * @brief  Configures the Stick Check pin (PB1) as a standard GPIO Input (No IT).
  * @param  None
  * @retval None
  */
void DM_HW_Drv_STP_CK_Set_GPIO_Input(void);

/**
  * @brief  Reads the current status of the Stick Check pin.
  * @param  None
  * @retval BitStatus: SET (High, Stick Removed) or RESET (Low, Stick Inserted).
  */
BitStatus DM_HW_Drv_STP_CK_Get_Status(void);

/**
  * @brief  Prepares the system for Ultra-Low Power consumption before entering Halt.
  * @param  None
  * @retval None
  */
void DM_HW_Drv_Power_PrepareSleep(void);

/**
  * @brief  Resumes system peripherals after waking up from Halt mode.
  * @param  None
  * @retval None
  */
void DM_HW_Drv_Power_Resume(void);

/**
  * @brief  Writes a block of data to the internal EEPROM.
  * @param  wAddrOffset: Address offset from the start of EEPROM (0 to 255).
  * @param  pBuffer: Pointer to the data buffer to write.
  * @param  wLength: Number of bytes to write.
  * @retval None
  */
void DM_HW_Drv_EEPROM_Write(uint16_t wAddrOffset, uint8_t* pBuffer, uint16_t wLength);

/**
  * @brief  Reads a block of data from the internal EEPROM.
  * @param  wAddrOffset: Address offset from the start of EEPROM (0 to 255).
  * @param  pBuffer: Pointer to the buffer to store the read data.
  * @param  wLength: Number of bytes to read.
  * @retval None
  */
void DM_HW_Drv_EEPROM_Read(uint16_t wAddrOffset, uint8_t* pBuffer, uint16_t wLength);

#endif /* __DM_HW_DRV_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
