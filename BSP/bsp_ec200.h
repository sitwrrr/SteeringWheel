/**
 * @file bsp_ec200.h
 * @brief EC200 4G模块驱动
 * @version 1.0.0
 * @date 2025-01-01
 */

#ifndef __BSP_EC200_H
#define __BSP_EC200_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include "usart.h"
#include <string.h>

/* Exported defines ----------------------------------------------------------*/
#define EC200_RX_BUFFER_SIZE    512

/* Exported functions --------------------------------------------------------*/
uint8_t BSP_EC200_Init(void);
void BSP_EC200_SendAT(const char *cmd);
uint8_t BSP_EC200_WaitResponse(char *response, uint16_t len, uint32_t timeout);
void BSP_EC200_Process(void);
uint8_t BSP_EC200_IsMQTTReady(void);
uint8_t BSP_EC200_IsReady(void);
uint8_t BSP_EC200_GetMQTTOpenFlag(void);
uint8_t BSP_EC200_GetMQTTConnFlag(void);
void BSP_EC200_ClearMQTTFlags(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_EC200_H */
