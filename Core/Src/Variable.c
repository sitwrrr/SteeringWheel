/**
 * @file Variable.c
 * @brief 全局变量定义
 * @version 1.0.0
 * @date 2025-01-01
 */

/* Includes ------------------------------------------------------------------*/
#include "Variable.h"
#include "cmsis_os2.h"
#include <string.h>

/* Global variables ----------------------------------------------------------*/
VehicleData_t g_vehicleData;
WorkMode_t g_workMode;

osMutexId_t g_dataMutex;
osEventFlagsId_t canEventHandle;

/* CAN接收标志 */
volatile uint8_t g_can1Received = 0;
volatile uint8_t g_can2Received = 0;

/* USB接收缓冲区 */
uint8_t g_usbRxBuffer[256];
volatile uint16_t g_usbRxLength = 0;

/* MQTT状态 */
volatile uint8_t g_mqttConnected = 0;
volatile uint8_t g_mqttUploading = 0;

/* Flash存储状态 */
volatile uint8_t g_storageActive = 0;
uint32_t g_storageCount = 0;

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 初始化全局变量
 */
void Variable_Init(void)
{
    /* 创建互斥锁（CMSIS-RTOS V2） */
    g_dataMutex = osMutexNew(NULL);
    
    /* 创建事件标志（CMSIS-RTOS V2） */
    canEventHandle = osEventFlagsNew(NULL);
    
    /* 重置车辆数据 */
    VehicleData_Reset();
    
    /* 默认待机模式 */
    g_workMode = MODE_STANDBY;
    
    /* 清空USB缓冲区 */
    memset(g_usbRxBuffer, 0, sizeof(g_usbRxBuffer));
    g_usbRxLength = 0;
    
    /* 清空状态标志 */
    g_can1Received = 0;
    g_can2Received = 0;
    g_mqttConnected = 0;
    g_mqttUploading = 0;
    g_storageActive = 0;
    g_storageCount = 0;
}

/**
 * @brief 重置车辆数据
 */
void VehicleData_Reset(void)
{
    memset(&g_vehicleData, 0, sizeof(VehicleData_t));
    
    /* 默认值 */
    g_vehicleData.gear = GEAR_N;
    g_vehicleData.safety_loop = 1;  /* 安全回路正常 */
}
