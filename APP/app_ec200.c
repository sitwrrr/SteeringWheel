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
#include <string.h>
#include <stdio.h>

/* Private defines ----------------------------------------------------------*/
#define MQTT_SERVER     "123.57.107.238"
#define MQTT_PORT       1883
#define MQTT_CLIENT_ID  "hello"
#define MQTT_USERNAME   "Rain"
#define MQTT_PASSWORD   "20031010"
#define MQTT_TOPIC      "mqtt"

/* Private variables --------------------------------------------------------*/
static uint8_t mqttConnected = 0;

/* Private functions --------------------------------------------------------*/

/**
 * @brief 发送MQTT数据
 */
static void MQTT_Publish(const char *data)
{
    char cmd[256];
    sprintf(cmd, "AT+QMTPUBEX=0,0,0,0,\"%s\",%d", MQTT_TOPIC, strlen(data));
    BSP_EC200_SendAT(cmd);
    HAL_Delay(10);
    BSP_EC200_SendAT(data);
}

/**
 * @brief 打包电车JSON数据
 */
static void jsonPackElectric(void)
{
    static uint8_t changeFlag = 0;
    char json[300];
    
    if (!changeFlag)
    {
        sprintf(json, "{1,%d,%d,%d,%d,%d,%f,%d,%d,%f,%f,%f,%f,%d,%d}",
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
        sprintf(json, "{2,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%f,%f,%d,%d}",
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
    
    MQTT_Publish(json);
}

/**
 * @brief 打包油车JSON数据
 */
static void jsonPackFuel(void)
{
    char json[300];
    
    sprintf(json, "{1,%d,%d,%d,%d,%d,%f,%d,%d,%f,%f,%f,%f,%d,%d}",
            (int)g_motecData.frontSpeed,
            (int)g_motecData.throttlePosition,
            0,  /* bms_fault */
            (int)g_motecData.engineRPM,
            0,  /* rpm_right */
            0.0f,
            g_motecData.gear,
            g_motecData.gear,
            0.0f, 0.0f, 0.0f, 0.0f,
            0, 0);
    
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
    
    /* MQTT服务器连接 */
    char cmd[128];
    sprintf(cmd, "AT+QMTOPEN=0,\"%s\",%d", MQTT_SERVER, MQTT_PORT);
    BSP_EC200_SendAT(cmd);
    
    /* 等待连接成功 */
    HAL_Delay(2000);
    
    /* MQTT客户端连接 */
    sprintf(cmd, "AT+QMTCONN=0,\"%s\",\"%s\",\"%s\"", 
            MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD);
    BSP_EC200_SendAT(cmd);
    
    HAL_Delay(1000);
    
    mqttConnected = 1;
    g_mqttConnected = 1;
    
    printf("MQTT Connected\r\n");
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
    sprintf(json, "{%d,%d,%d,%d,%d,%d,%d,%d}",
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
