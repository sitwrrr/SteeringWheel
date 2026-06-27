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
#include "app_can.h"
#include "app_ec200.h"
#include "app_input.h"
#include "app_ws2812b.h"
#include "app_simhub.h"
#include "app_iap.h"
#include "bsp_usb.h"
#include "Variable.h"
#include "lvgl.h"
#include "iwdg.h"
#include <stdio.h>
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
  .priority = (osPriority_t) osPriorityRealtime4,
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

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);
void vApplicationDaemonTaskStartupHook(void);

/* USER CODE BEGIN 4 */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
    (void)xTask;
    printf("\r\n!!! STACK OVERFLOW in task: %s\r\n", pcTaskName);
    for(;;);
}
/* USER CODE END 4 */

/* USER CODE BEGIN LONG_PRESS_CALLBACK */
/**
 * @brief 长按回调：长按KEY5跳转系统BootLoader（用于IAP升级）
 */
void APP_Input_LongPressCallback(KeyId_t id)
{
    if (id == KEY_5)
    {
        printf("\r\nLong press KEY5, jumping to bootloader...\r\n");
        HAL_Delay(100);
        APP_IAP_JumpToBootloader();
    }
}
/* USER CODE END LONG_PRESS_CALLBACK */

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
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN lvgl_task */
  (void)argument;

  /* 初始化LVGL */
  // extern void ui_init(void);
  // ui_init();

  for(;;)
  {
    /* 获取LVGL互斥锁，保护lv_timer_handler */
    osMutexAcquire(g_lvglMutex, osWaitForever);

    lv_timer_handler();

    osMutexRelease(g_lvglMutex);

    /* 喂狗 */
    HAL_IWDG_Refresh(&hiwdg1);

    osDelay(5);
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
  (void)argument;
  for(;;)
  {
    /* 仪表盘UI更新（CAN数据→LVGL控件） */
    /* TODO: 在实现UI界面后，在此处添加仪表数据刷新逻辑 */
    osDelay(50);
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
  APP_CAN_Init();
  
  for(;;)
  {
    APP_CAN_TaskProcess();  /* 阻塞在队列上，有消息就解析 */

    /* 解析完后通知MQTT上传任务 */
    osThreadFlagsSet(iot_uploadHandle, 0x01);
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
  (void)argument;

  /* SW-M8修复: EC200初始化放在此任务，避免阻塞iot_upload_task */
  APP_EC200_Init();

  /* 初始化完成，通知iot_upload_task可以开始上传 */
  osThreadFlagsSet(iot_uploadHandle, 0x02);

  /* 初始化完成后自删除 */
  osThreadTerminate(ec200_initHandle);
  /* USER CODE END ec200_init_task */
}

/* USER CODE BEGIN Header_iot_upload_task */
/**
 * @brief MQTT数据上传任务
 *        定时读取g_vehicleData并上传到云端
 */
/* USER CODE END Header_iot_upload_task */
void iot_upload_task(void *argument)
{
  /* USER CODE BEGIN iot_upload_task */
  (void)argument;

  /* SW-M8修复: 等待ec200_init_task完成EC200初始化（flag 0x02） */
  osThreadFlagsWait(0x02U, osFlagsWaitAny, osWaitForever);

  for(;;)
  {
    /* 等待CAN任务通知（有新数据） */
    osThreadFlagsWait(0x01U, osFlagsWaitAny, osWaitForever);
    
    /* 上传数据 */
    APP_EC200_Upload();
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
  (void)argument;
  APP_WS2812B_Init();

  for(;;)
  {
    APP_WS2812B_Update(g_vehicleData.rpm_left, g_workMode);
    osDelay(50);
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
  (void)argument;
  APP_Input_Init();

  for(;;)
  {
    APP_Input_Scan();
    APP_Input_SendCAN();
    osDelay(20);
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
  (void)argument;

  for(;;)
  {
    /* 检查CDC接收（SimHub数据） */
    uint16_t rxLen = g_usbRxLength;
    if (rxLen > 0)
    {
      g_usbRxLength = 0;
      APP_SimHub_ReceiveData(g_usbRxBuffer, rxLen);
      APP_SimHub_Process();

      /* 收到SimHub数据，自动切换到模拟器模式 */
      if (g_workMode == MODE_STANDBY)
      {
        g_workMode = MODE_SIMULATOR;
      }
    }

    /* 模拟器模式下发送HID按键报告 */
    if (g_workMode == MODE_SIMULATOR)
    {
      BSP_USB_SendHIDReport((uint16_t)g_vehicleData.key_state);
    }

    osDelay(10);
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
  (void)argument;
  uint32_t lastCanTick = 0;
  uint32_t lastSimHubTick = 0;

  for(;;)
  {
    uint32_t now = HAL_GetTick();

    /* CAN有数据更新（实车模式） */
    if (g_vehicleData.timestamp != lastCanTick)
    {
      lastCanTick = g_vehicleData.timestamp;
      if (g_workMode != MODE_SIMULATOR)
      {
        g_workMode = MODE_VEHICLE;
      }
    }

    /* SimHub有数据更新（模拟器模式，由usb_poll_task设置） */
    if (g_workMode == MODE_SIMULATOR)
    {
      lastSimHubTick = now;
    }

    /* 超过5秒无任何数据，回到待机 */
    if (g_workMode == MODE_VEHICLE && (now - lastCanTick > 5000))
    {
      g_workMode = MODE_STANDBY;
    }
    if (g_workMode == MODE_SIMULATOR && (now - lastSimHubTick > 5000))
    {
      g_workMode = MODE_STANDBY;
    }

    osDelay(500);
  }
  /* USER CODE END mode_detect_task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

