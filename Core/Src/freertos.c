/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

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
/* Definitions for lvgl */
osThreadId_t lvglHandle;
const osThreadAttr_t lvgl_attributes = {
  .name = "lvgl",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityRealtime3,
};
/* Definitions for lvgl_meter */
osThreadId_t lvgl_meterHandle;
const osThreadAttr_t lvgl_meter_attributes = {
  .name = "lvgl_meter",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityRealtime2,
};
/* Definitions for can_process */
osThreadId_t can_processHandle;
const osThreadAttr_t can_process_attributes = {
  .name = "can_process",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityRealtime2,
};
/* Definitions for ec200_init */
osThreadId_t ec200_initHandle;
const osThreadAttr_t ec200_init_attributes = {
  .name = "ec200_init",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityRealtime,
};
/* Definitions for iot_upload */
osThreadId_t iot_uploadHandle;
const osThreadAttr_t iot_upload_attributes = {
  .name = "iot_upload",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityRealtime,
};
/* Definitions for ws2812b */
osThreadId_t ws2812bHandle;
const osThreadAttr_t ws2812b_attributes = {
  .name = "ws2812b",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for key_scan */
osThreadId_t key_scanHandle;
const osThreadAttr_t key_scan_attributes = {
  .name = "key_scan",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for usb_poll */
osThreadId_t usb_pollHandle;
const osThreadAttr_t usb_poll_attributes = {
  .name = "usb_poll",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for mode_detect */
osThreadId_t mode_detectHandle;
const osThreadAttr_t mode_detect_attributes = {
  .name = "mode_detect",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void lvgl_task(void *argument);
void lvgl_meter_task(void *argument);
void can_process_task(void *argument);
void ec200_init_task(void *argument);
void iot_upload_task(void *argument);
void ws2812b_task(void *argument);
void key_scan_task(void *argument);
void usb_poll_task(void *argument);
void mode_detect_task(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);
void vApplicationDaemonTaskStartupHook(void);

/* USER CODE BEGIN 4 */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
   /* Run time stack overflow checking is performed if
   configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
   called if a stack overflow is detected. */
}
/* USER CODE END 4 */

/* USER CODE BEGIN DAEMON_TASK_STARTUP_HOOK */
void vApplicationDaemonTaskStartupHook(void)
{
}
/* USER CODE END DAEMON_TASK_STARTUP_HOOK */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of lvgl */
  lvglHandle = osThreadNew(lvgl_task, NULL, &lvgl_attributes);

  /* creation of lvgl_meter */
  lvgl_meterHandle = osThreadNew(lvgl_meter_task, NULL, &lvgl_meter_attributes);

  /* creation of can_process */
  can_processHandle = osThreadNew(can_process_task, NULL, &can_process_attributes);

  /* creation of ec200_init */
  ec200_initHandle = osThreadNew(ec200_init_task, NULL, &ec200_init_attributes);

  /* creation of iot_upload */
  iot_uploadHandle = osThreadNew(iot_upload_task, NULL, &iot_upload_attributes);

  /* creation of ws2812b */
  ws2812bHandle = osThreadNew(ws2812b_task, NULL, &ws2812b_attributes);

  /* creation of key_scan */
  key_scanHandle = osThreadNew(key_scan_task, NULL, &key_scan_attributes);

  /* creation of usb_poll */
  usb_pollHandle = osThreadNew(usb_poll_task, NULL, &usb_poll_attributes);

  /* creation of mode_detect */
  mode_detectHandle = osThreadNew(mode_detect_task, NULL, &mode_detect_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_lvgl_task */
/**
  * @brief  Function implementing the lvgl thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_lvgl_task */
void lvgl_task(void *argument)
{
  /* USER CODE BEGIN lvgl_task */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END lvgl_task */
}

/* USER CODE BEGIN Header_lvgl_meter_task */
/**
* @brief Function implementing the lvgl_meter thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_lvgl_meter_task */
void lvgl_meter_task(void *argument)
{
  /* USER CODE BEGIN lvgl_meter_task */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END lvgl_meter_task */
}

/* USER CODE BEGIN Header_can_process_task */
/**
* @brief Function implementing the can_process thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_can_process_task */
void can_process_task(void *argument)
{
  /* USER CODE BEGIN can_process_task */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END can_process_task */
}

/* USER CODE BEGIN Header_ec200_init_task */
/**
* @brief Function implementing the ec200_init thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_ec200_init_task */
void ec200_init_task(void *argument)
{
  /* USER CODE BEGIN ec200_init_task */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END ec200_init_task */
}

/* USER CODE BEGIN Header_iot_upload_task */
/**
* @brief Function implementing the iot_upload thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_iot_upload_task */
void iot_upload_task(void *argument)
{
  /* USER CODE BEGIN iot_upload_task */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END iot_upload_task */
}

/* USER CODE BEGIN Header_ws2812b_task */
/**
* @brief Function implementing the ws2812b thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_ws2812b_task */
void ws2812b_task(void *argument)
{
  /* USER CODE BEGIN ws2812b_task */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END ws2812b_task */
}

/* USER CODE BEGIN Header_key_scan_task */
/**
* @brief Function implementing the key_scan thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_key_scan_task */
void key_scan_task(void *argument)
{
  /* USER CODE BEGIN key_scan_task */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END key_scan_task */
}

/* USER CODE BEGIN Header_usb_poll_task */
/**
* @brief Function implementing the usb_poll thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_usb_poll_task */
void usb_poll_task(void *argument)
{
  /* USER CODE BEGIN usb_poll_task */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END usb_poll_task */
}

/* USER CODE BEGIN Header_mode_detect_task */
/**
* @brief Function implementing the mode_detect thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_mode_detect_task */
void mode_detect_task(void *argument)
{
  /* USER CODE BEGIN mode_detect_task */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END mode_detect_task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

