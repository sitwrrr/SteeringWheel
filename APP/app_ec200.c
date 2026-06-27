/**
 * @file app_ec200.c
 * @brief EC200 MQTT应用实现
 * @version 1.0.0
 * @date 2025-01-01
 */

#include "app_ec200.h"
#include "app_motec.h"
#include "bsp_ec200.h"
#include "Variable.h"
#include "cmsis_os2.h"  /* SW-H5修复: 使用mutex */
#include <string.h>
#include <stdio.h>

/* Private defines ----------------------------------------------------------*/
/* MQTT连接配置 - 修改以下参数连接到你的MQTT服务器 */
#define MQTT_SERVER     "123.57.107.238"  /* MQTT服务器IP地址 */
#define MQTT_PORT       1883              /* MQTT端口号（默认1883） */
#define MQTT_CLIENT_ID  "hello"           /* 客户端ID（需唯一） */
#define MQTT_USERNAME   "Rain"            /* MQTT用户名 */
#define MQTT_PASSWORD   "20031010"        /* MQTT密码 */
#define MQTT_TOPIC      "mqtt"            /* 发布Topic */

/* Private variables --------------------------------------------------------*/
static uint8_t mqttConnected = 0;

/* Private functions --------------------------------------------------------*/

/**
 * @brief 发送MQTT数据
 */
static void MQTT_Publish(const char *data)
{
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "AT+QMTPUBEX=0,0,0,0,\"%s\",%d", MQTT_TOPIC, strlen(data));
    BSP_EC200_SendAT(cmd);
    /* SW-H4修复: 等待EC200返回'>'提示符后再发数据 */
    if (!BSP_EC200_WaitResponse(">", 2000))
    {
        printf("MQTT > prompt timeout!\r\n");
        return;
    }
    BSP_EC200_SendAT(data);
}

/**
 * @brief 打包电车JSON数据
 */
static void jsonPackElectric(void)
{
    static uint8_t changeFlag = 0;
    char json[300];
    
    osMutexAcquire(g_dataMutex, osWaitForever);  /* SW-H5修复: 保护g_vehicleData */
    if (!changeFlag)
    {
        snprintf(json, sizeof(json), "{1,%d,%d,%d,%d,%d,%.1f,%d,%d,%.1f,%.1f,%.1f,%.1f,%d,%d}",
                g_vehicleData.speed,
                g_vehicleData.throttle,
                g_vehicleData.bms_safe,
                g_vehicleData.rpm_left,
                g_vehicleData.rpm_right,
                (float)g_vehicleData.current / 10.0f,
                g_vehicleData.gear,
                g_vehicleData.gear,
                (float)g_vehicleData.accel_x / 100.0f,
                (float)g_vehicleData.accel_y / 100.0f,
                (float)g_vehicleData.accel_z / 100.0f,
                (float)g_vehicleData.yaw_rate / 100.0f,
                0, 0);
        changeFlag = 1;
    }
    else
    {
        snprintf(json, sizeof(json), "{2,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%.1f,%.1f,%d,%d}",
                g_vehicleData.torque_left,
                g_vehicleData.torque_right,
                (int)g_vehicleData.current,
                (int)g_vehicleData.car_travel,
                g_vehicleData.temp_controller_left,
                g_vehicleData.temp_controller_right,
                g_vehicleData.brake,
                g_vehicleData.temp_motor_left,
                g_vehicleData.temp_motor_right,
                g_vehicleData.steering_angle,
                0.0f, 0.0f, 0, 0);
        changeFlag = 0;
    }
    osMutexRelease(g_dataMutex);  /* SW-H5修复 */
    
    MQTT_Publish(json);
}

/**
 * @brief 打包油车JSON数据
 */
static void jsonPackFuel(void)
{
    char json[300];
    
    osMutexAcquire(g_dataMutex, osWaitForever);  /* SW-H5修复: 保护g_motecData */
    snprintf(json, sizeof(json), "{1,%d,%d,%d,%d,%d,%.1f,%d,%d,%.1f,%.1f,%.1f,%.1f,%d,%d}",
            (int)g_motecData.frontSpeed,
            (int)g_motecData.throttlePosition,
            0,
            (int)g_motecData.engineRPM,
            0,
            0.0f,
            g_motecData.gear,
            g_motecData.gear,
            0.0f, 0.0f, 0.0f, 0.0f,
            0, 0);
    osMutexRelease(g_dataMutex);  /* SW-H5修复 */
    
    MQTT_Publish(json);
}

/**
 * @brief 打包JSON数据（自动切换油车/电车）
 */
static void jsonPack(void)
{
    if (APP_Motec_GetCarType() == 1)
    {
        jsonPackFuel();
    }
    else
    {
        jsonPackElectric();
    }
}

/* Exported functions -------------------------------------------------------*/

/**
 * @brief 初始化EC200 MQTT
 * @return 1=成功, 0=失败
 */
uint8_t APP_EC200_Init(void)
{
    /* 初始化EC200模块 */
    if (!BSP_EC200_Init())
    {
        printf("EC200 Failed\r\n");
        return 0;
    }
    printf("EC200 Ready\r\n");
    
    /* 清除MQTT标志 */
    BSP_EC200_ClearMQTTFlags();
    
    /* MQTT服务器连接 */
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "AT+QMTOPEN=0,\"%s\",%d", MQTT_SERVER, MQTT_PORT);
    BSP_EC200_SendAT(cmd);
    
    /* 等待连接成功（带超时） */
    printf("Waiting for MQTT Open...\r\n");
    uint32_t timeout = 0;
    while (!BSP_EC200_GetMQTTOpenFlag() && timeout < 50)
    {
        HAL_Delay(200);
        timeout++;
    }
    if (timeout >= 50)
    {
        printf("MQTT Open timeout!\r\n");
        return 0;
    }
    printf("MQTT Open OK\r\n");
    
    HAL_Delay(500);
    
    /* MQTT客户端连接 */
    snprintf(cmd, sizeof(cmd), "AT+QMTCONN=0,\"%s\",\"%s\",\"%s\"", 
            MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD);
    BSP_EC200_SendAT(cmd);
    
    /* 等待连接成功（带超时） */
    printf("Waiting for MQTT Connect...\r\n");
    timeout = 0;
    while (!BSP_EC200_GetMQTTConnFlag() && timeout < 50)
    {
        HAL_Delay(200);
        timeout++;
    }
    if (timeout >= 50)
    {
        printf("MQTT Connect timeout!\r\n");
        return 0;
    }
    printf("MQTT Connected\r\n");
    
    mqttConnected = 1;
    g_mqttConnected = 1;
    
    return 1;
}

/**
 * @brief MQTT数据上传
 */
void APP_EC200_Upload(void)
{
    if (!mqttConnected) return;
    
    jsonPack();
}

/**
 * @brief MQTT数据上传（带参数）
 * @param data: 车辆数据指针
 */
void APP_EC200_UploadData(VehicleData_t *data)
{
    if (!mqttConnected) return;
    
    char json[300];
    snprintf(json, sizeof(json), "{%d,%d,%d,%d,%d,%d,%d,%d}",
            data->speed,
            data->rpm_left,
            data->rpm_right,
            data->gear,
            data->soc,
            data->temp_motor_left,
            data->temp_motor_right,
            data->brake);
    
    MQTT_Publish(json);
}

/**
 * @brief 获取MQTT连接状态
 * @return 1=已连接, 0=未连接
 */
uint8_t APP_EC200_IsConnected(void)
{
    return mqttConnected;
}
