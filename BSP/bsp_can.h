/**
 * @file bsp_can.h
 * @brief CAN总线驱动
 * @version 1.1.0
 * @date 2025-01-01
 */

#ifndef __BSP_CAN_H
#define __BSP_CAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

/* Exported types ------------------------------------------------------------*/
typedef struct {
    uint32_t id;
    uint8_t channel;  /* 1=FDCAN1, 2=FDCAN2 */
    uint8_t data[8];
} CAN_Message_t;

/* Exported functions --------------------------------------------------------*/
void BSP_CAN_Init(void);
void BSP_CAN1_Send(uint32_t id, uint8_t *data);
void BSP_CAN2_Send(uint32_t id, uint8_t *data);

/* 回调函数（在app_can.c中实现） */
void BSP_CAN1_RxCallback(CAN_Message_t *msg);
void BSP_CAN2_RxCallback(CAN_Message_t *msg);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_CAN_H */
