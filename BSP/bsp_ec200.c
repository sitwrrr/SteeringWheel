/**
 * @file bsp_ec200.c
 * @brief EC200 4G模块驱动实现
 * @version 1.0.0
 * @date 2025-01-01
 */

#include "bsp_ec200.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>

/* Private defines ----------------------------------------------------------*/
#define EC200_RX_BUFFER_SIZE    512
#define EC200_TIMEOUT           5000

/* Private variables --------------------------------------------------------*/
static uint8_t rxBuffer[EC200_RX_BUFFER_SIZE];
static uint16_t rxIndex = 0;
static uint8_t rxFlag = 0;
static uint8_t readyFlag = 0;
static uint8_t mqttOpenFlag = 0;
static uint8_t mqttConnFlag = 0;
static uint8_t mqttPubFlag = 0;

/* Private functions --------------------------------------------------------*/

/**
 * @brief 发送AT命令
 */
static void EC200_SendAT(const char *cmd)
{
    char buf[256];
    sprintf(buf, "%s\r\n", cmd);
    HAL_UART_Transmit(&huart3, (uint8_t *)buf, strlen(buf), HAL_MAX_DELAY);
    printf("[EC200 TX] %s\r\n", cmd);  /* 将发送的AT命令打印到串口1，方便调试 */
}

/**
 * @brief 等待响应
 */
static uint8_t EC200_WaitResponse(const char *expected, uint32_t timeout)
{
    uint32_t start = HAL_GetTick();
    
    while ((HAL_GetTick() - start) < timeout)
    {
        if (strstr((char *)rxBuffer, expected) != NULL)
        {
            memset(rxBuffer, 0, EC200_RX_BUFFER_SIZE);
            rxIndex = 0;
            return 1;
        }
        HAL_Delay(10);
    }
    
    return 0;
}

/* Exported functions -------------------------------------------------------*/

/**
 * @brief 初始化EC200
 * @return 1=成功, 0=失败
 */
uint8_t BSP_EC200_Init(void)
{
    /* 复位EC200 */
    HAL_GPIO_WritePin(PWR_4G_GPIO_Port, PWR_4G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(RST_4G_GPIO_Port, RST_4G_Pin, GPIO_PIN_SET);
    HAL_Delay(3000);
    HAL_GPIO_WritePin(PWR_4G_GPIO_Port, PWR_4G_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RST_4G_GPIO_Port, RST_4G_Pin, GPIO_PIN_RESET);
    
    /* 启动串口接收 */
    HAL_UART_Receive_IT(&huart3, rxBuffer, 1);
    
    /* 等待模块就绪 */
    if (!EC200_WaitResponse("RDY", EC200_TIMEOUT))
    {
        return 0;
    }
    
    /* 关闭回显 */
    EC200_SendAT("ATE0");
    HAL_Delay(500);
    
    /* 检查SIM卡 */
    EC200_SendAT("AT+CIMI");
    if (!EC200_WaitResponse("460", EC200_TIMEOUT))
    {
        return 0;
    }
    
    /* 检查信号 */
    EC200_SendAT("AT+CSQ");
    if (!EC200_WaitResponse("CSQ", EC200_TIMEOUT))
    {
        return 0;
    }
    
    /* 检查网络注册 */
    EC200_SendAT("AT+CGATT?");
    if (!EC200_WaitResponse("+CGATT: 1", EC200_TIMEOUT))
    {
        return 0;
    }
    
    readyFlag = 1;
    return 1;
}

/**
 * @brief 发送AT命令（外部接口）
 */
void BSP_EC200_SendAT(const char *cmd)
{
    EC200_SendAT(cmd);
}

/**
 * @brief 等待响应（外部接口）
 */
uint8_t BSP_EC200_WaitResponse(char *response, uint16_t len, uint32_t timeout)
{
    return EC200_WaitResponse(response, timeout);
}

/**
 * @brief 处理EC200数据
 */
void BSP_EC200_Process(void)
{
    /* 在中断回调中处理 */
}

/**
 * @brief 获取MQTT状态
 */
uint8_t BSP_EC200_IsMQTTReady(void)
{
    return readyFlag && mqttOpenFlag && mqttConnFlag;
}

/**
 * @brief 获取就绪状态
 */
uint8_t BSP_EC200_IsReady(void)
{
    return readyFlag;
}

/**
 * @brief 获取MQTT Open标志
 */
uint8_t BSP_EC200_GetMQTTOpenFlag(void)
{
    return mqttOpenFlag;
}

/**
 * @brief 获取MQTT Connect标志
 */
uint8_t BSP_EC200_GetMQTTConnFlag(void)
{
    return mqttConnFlag;
}

/**
 * @brief 清除MQTT标志
 */
void BSP_EC200_ClearMQTTFlags(void)
{
    mqttOpenFlag = 0;
    mqttConnFlag = 0;
    mqttPubFlag = 0;
}

/* UART接收回调 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3)
    {
        if (rxIndex >= EC200_RX_BUFFER_SIZE - 1)
        {
            rxIndex = 0;
            memset(rxBuffer, 0, EC200_RX_BUFFER_SIZE);
        }
        
        rxBuffer[rxIndex++] = rxBuffer[0];
        
        /* 检查是否收到完整响应 */
        if (rxIndex >= 2 && rxBuffer[rxIndex - 2] == 0x0D && rxBuffer[rxIndex - 1] == 0x0A)
        {
            rxBuffer[rxIndex] = '\0';
            printf("[EC200 RX] %s", rxBuffer);  /* 将EC200回复内容打印到串口1，方便调试 */

            /* 检查特定响应 */
            if (strstr((char *)rxBuffer, "RDY") != NULL)
            {
                readyFlag = 1;
            }
            else if (strstr((char *)rxBuffer, "OPEN") != NULL)
            {
                mqttOpenFlag = 1;
            }
            else if (strstr((char *)rxBuffer, "CONN") != NULL)
            {
                mqttConnFlag = 1;
            }
            else if (strstr((char *)rxBuffer, "PUBEX") != NULL)
            {
                mqttPubFlag = 1;
            }
            else if (strstr((char *)rxBuffer, "ERROR") != NULL)
            {
                readyFlag = 0;
                mqttOpenFlag = 0;
                mqttConnFlag = 0;
                mqttPubFlag = 0;
            }
            
            rxIndex = 0;
            memset(rxBuffer, 0, EC200_RX_BUFFER_SIZE);
        }
        
        /* 继续接收 */
        HAL_UART_Receive_IT(&huart3, rxBuffer, 1);
    }
}
