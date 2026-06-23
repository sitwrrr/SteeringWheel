/**
 * @file app_simhub.c
 * @brief SimHub模拟器数据解析实现
 * @version 1.0.0
 * @date 2025-01-01
 */

#include "app_simhub.h"
#include "bsp_usb.h"
#include "Variable.h"
#include <string.h>
#include <stdlib.h>

/* Private variables ---------------------------------------------------------*/
static SimHubData_t simhubData;
static uint8_t simhubBuffer[SIMHUB_BUFFER_SIZE];
static uint16_t simhubIndex = 0;
static uint8_t simhubReady = 0;

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 简单JSON解析（提取整数值）
 */
static int json_get_int(const char *json, const char *key)
{
    char *pos = strstr(json, key);
    if (pos == NULL) return 0;
    
    pos = strchr(pos, ':');
    if (pos == NULL) return 0;
    pos++;
    
    while (*pos == ' ') pos++;
    
    return atoi(pos);
}

/**
 * @brief 简单JSON解析（提取字符串值）
 */
static const char *json_get_string(const char *json, const char *key, char *buf, int bufSize)
{
    char *pos = strstr(json, key);
    if (pos == NULL) return NULL;
    
    pos = strchr(pos, ':');
    if (pos == NULL) return NULL;
    pos++;
    
    while (*pos == ' ') pos++;
    if (*pos != '"') return NULL;
    pos++;
    
    int i = 0;
    while (*pos != '"' && *pos != '\0' && i < bufSize - 1)
    {
        buf[i++] = *pos++;
    }
    buf[i] = '\0';
    
    return buf;
}

/* Exported functions --------------------------------------------------------*/

/**
 * @brief 初始化SimHub
 */
void APP_SimHub_Init(void)
{
    memset(&simhubData, 0, sizeof(SimHubData_t));
    memset(simhubBuffer, 0, SIMHUB_BUFFER_SIZE);
    simhubIndex = 0;
    simhubReady = 0;
}

/**
 * @brief 处理SimHub数据
 */
void APP_SimHub_Process(void)
{
    if (simhubReady)
    {
        APP_SimHub_ParseJSON((const char *)simhubBuffer);
        APP_SimHub_UpdateVehicleData();
        simhubReady = 0;
        simhubIndex = 0;
    }
}

/**
 * @brief 接收USB数据
 * @param data: 数据指针
 * @param len: 数据长度
 */
void APP_SimHub_ReceiveData(uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        if (data[i] == '{')
        {
            simhubIndex = 0;
            simhubBuffer[simhubIndex++] = data[i];
        }
        else if (data[i] == '}' && simhubIndex > 0)
        {
            simhubBuffer[simhubIndex++] = data[i];
            simhubBuffer[simhubIndex] = '\0';
            simhubReady = 1;
        }
        else if (simhubIndex > 0 && simhubIndex < SIMHUB_BUFFER_SIZE - 1)
        {
            simhubBuffer[simhubIndex++] = data[i];
        }
    }
}

/**
 * @brief 解析JSON数据
 * @param json: JSON字符串
 */
void APP_SimHub_ParseJSON(const char *json)
{
    char buf[32];
    
    simhubData.rpm = json_get_int(json, "rpm");
    simhubData.speed = json_get_int(json, "speed");
    simhubData.throttle = json_get_int(json, "throttle");
    simhubData.brake = json_get_int(json, "brake");
    simhubData.fuel = json_get_int(json, "fuel");
    simhubData.redLineRPM = json_get_int(json, "redLineRPM");
    simhubData.lap = json_get_int(json, "lap");
    
    const char *gearStr = json_get_string(json, "gear", buf, sizeof(buf));
    if (gearStr != NULL)
    {
        if (strcmp(gearStr, "N") == 0) simhubData.gear = 0;
        else if (strcmp(gearStr, "1") == 0) simhubData.gear = 1;
        else if (strcmp(gearStr, "2") == 0) simhubData.gear = 2;
        else if (strcmp(gearStr, "3") == 0) simhubData.gear = 3;
        else if (strcmp(gearStr, "4") == 0) simhubData.gear = 4;
        else if (strcmp(gearStr, "5") == 0) simhubData.gear = 5;
        else if (strcmp(gearStr, "6") == 0) simhubData.gear = 6;
        else simhubData.gear = 0;
    }
}

/**
 * @brief 更新车辆数据
 */
void APP_SimHub_UpdateVehicleData(void)
{
    g_vehicleData.speed = simhubData.speed;
    g_vehicleData.rpm_left = simhubData.rpm;
    g_vehicleData.rpm_right = simhubData.rpm;
    g_vehicleData.gear = simhubData.gear;
    g_vehicleData.throttle = simhubData.throttle;
    g_vehicleData.brake = simhubData.brake;
    g_vehicleData.soc = simhubData.fuel;
    g_vehicleData.timestamp = HAL_GetTick();
    
    APP_SimHub_DataReceivedCallback(&g_vehicleData);
}

/**
 * @brief 数据接收回调（弱定义，可重写）
 */
__weak void APP_SimHub_DataReceivedCallback(VehicleData_t *data)
{
    (void)data;
}
