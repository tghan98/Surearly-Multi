/***************************************************************************
*
*               Marking Version - ���� ������ ��� ��Ƽ�� �߿���
*               Date : 2023. 07. 13
*               Adjusted Hardware : -
*               Software version : v2.1.1_alpha.2
*               PCB marking : -
*               PCB gerber : -
*
***************** COPYRIGHT Sugentech. Inc., 2014 *************************/

/* Includes ------------------------------------------------------------------*/

#include "main.h"
#include "User_Main.h"


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

//-----------------------------------------------------------------------------

/**
  * @brief  The application entry point.
  * @retval int
  */
void main(void)
{
  SYSCFG_REMAPDeInit();
  /* Initialize Application */
  User_Main_Init();
  
  while (1)
  {
    /* Run Application Loop */
    User_Main_Run();
  }
}














/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
