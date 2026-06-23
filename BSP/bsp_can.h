/**
 * @file bsp_can.h
 * @brief CAN总线驱动
 * @version 1.0.0
 * @date 2025-01-01
 */

#ifndef __BSP_CAN_H
#define __BSP_CAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include "fdcan.h"

/* Exported defines ----------------------------------------------------------*/
#define CAN_TX_BUFFER_SIZE  16
#define CAN_RX_BUFFER_SIZE  16

/* Exported types ------------------------------------------------------------*/
typedef struct {
    uint32_t id;
    uint8_t data[8];
    uint8_t len;
} CAN_Message_t;

/* Exported functions --------------------------------------------------------*/
void BSP_CAN_Init(void);
void BSP_CAN1_Send(uint32_t id, uint8_t *data, uint8_t len);
void BSP_CAN2_Send(uint32_t id, uint8_t *data, uint8_t len);
void BSP_CAN_FilterConfig(void);
uint8_t BSP_CAN1_Receive(CAN_Message_t *msg);
uint8_t BSP_CAN2_Receive(CAN_Message_t *msg);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_CAN_H */
