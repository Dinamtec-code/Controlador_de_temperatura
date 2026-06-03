/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include <stdio.h>
#include "usart.h"
#include "hrtim.h"
#include "scpi.h"

extern float temperatureSetpoint;
extern float pidKp, pidKi, pidKd;
extern float temperatureActual;
extern HRTIM_HandleTypeDef hhrtim1;
extern void lcd_init(void);
extern void lcd_display(float temp, float setpoint);
extern float read_temperature(void);
extern void init_scpi_interface(void);
#define HRTIM_PERIOD 64000

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
/* USER CODE BEGIN Variables */
/* USER CODE END Variables */
/* Definitions for commTask */
osThreadId_t commTaskHandle;
const osThreadAttr_t commTask_attributes = {
  .name = "commTask",
  .stack_size = 96 * 4,
  .priority = (osPriority_t) osPriorityNormal2,
};
/* Definitions for pidTask */
osThreadId_t pidTaskHandle;
const osThreadAttr_t pidTask_attributes = {
  .name = "pidTask",
  .stack_size = 96 * 4,
  .priority = (osPriority_t) osPriorityNormal4,
};
/* Definitions for sysTask */
osThreadId_t sysTaskHandle;
const osThreadAttr_t sysTask_attributes = {
  .name = "sysTask",
  .stack_size = 64 * 4,
  .priority = (osPriority_t) osPriorityNormal1,
};
/* Definitions for cmdQueue */
osMessageQueueId_t cmdQueueHandle;
const osMessageQueueAttr_t cmdQueue_attributes = {
  .name = "cmdQueue"
};
/* Definitions for uiQueue */
osMessageQueueId_t uiQueueHandle;
const osMessageQueueAttr_t uiQueue_attributes = {
  .name = "uiQueue"
};
/* Definitions for adcQueue */
osMessageQueueId_t adcQueueHandle;
const osMessageQueueAttr_t adcQueue_attributes = {
  .name = "adcQueue"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void parse_task(void);
void comm_task(void);
/* USER CODE END FunctionPrototypes */

void configTask(void *argument);
void controlTask(void *argument);
void systemTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  cmdQueueHandle = osMessageQueueNew (8, sizeof(uint16_t), &cmdQueue_attributes);
  uiQueueHandle = osMessageQueueNew (4, sizeof(uint16_t), &uiQueue_attributes);
  adcQueueHandle = osMessageQueueNew (4, sizeof(uint16_t), &adcQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  commTaskHandle = osThreadNew(configTask, NULL, &commTask_attributes);
  pidTaskHandle = osThreadNew(controlTask, NULL, &pidTask_attributes);
  sysTaskHandle = osThreadNew(systemTask, NULL, &sysTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* USER CODE END RTOS_EVENTS */
}

/* USER CODE BEGIN Header_configTask */
/**
* @brief Function implementing the commTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_configTask */
void configTask(void *argument)
{
	/* USER CODE BEGIN configTask */
	init_scpi_interface();
	ReceptionStart();
	printf("Ready\r\n");
	/* Infinite loop */
	for(;;)
	{
		parse_task();
		comm_task();
		osDelay(10);
	}
	/* USER CODE END configTask */
}

/* USER CODE BEGIN Header_controlTask */
/**
* @brief Function implementing the pidTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_controlTask */
void controlTask(void *argument)
{
	/* USER CODE BEGIN controlTask */
	lcd_init();
	float error, integral = 0, derivative, last_error = 0;
	
	for(;;)
	{
		read_temperature();
		error = temperatureSetpoint - temperatureActual;
		integral += error * 0.02f; /* 20ms period */
		derivative = (error - last_error) / 0.02f;
		last_error = error;

		float output = pidKp * error + pidKi * integral + pidKd * derivative;
		if (output > HRTIM_PERIOD - 1) output = HRTIM_PERIOD - 1;
		if (output < 0) output = 0;

		__HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_COMPAREUNIT_1, (uint16_t)output);
		__HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_B, HRTIM_COMPAREUNIT_1, (uint16_t)output);
		__HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_E, HRTIM_COMPAREUNIT_1, (uint16_t)output);

		lcd_display(temperatureActual, temperatureSetpoint);
		osDelay(20); /* 20ms period = 50Hz */
	}
	/* USER CODE END controlTask */
}

/* USER CODE BEGIN Header_systemTask */
/**
* @brief Function implementing the sysTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_systemTask */
void systemTask(void *argument)
{
  /* USER CODE BEGIN systemTask */
  /* Infinite loop - system monitoring */
  for(;;)
  {
    /* TODO: Status monitoring, error handling */
    osDelay(1000);
  }
  /* USER CODE END systemTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/