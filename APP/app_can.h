/**
 * @file app_can.h
 * @brief CAN数据解析应用
 * @version 1.0.0
 * @date 2025-01-01
 */

#ifndef __APP_CAN_H
#define __APP_CAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include "Variable.h"

/* Exported defines ----------------------------------------------------------*/
#define CAN_ID_MOTOR_DATA      0x211
#define CAN_ID_BATTERY_DATA    0x212
#define CAN_ID_TEMP_DATA       0x213
#define CAN_ID_ACCEL_DATA      0x214
#define CAN_ID_SAFETY_DATA     0x050

/* Exported functions --------------------------------------------------------*/
void APP_CAN_Init(void);
void APP_CAN_Process(void);
void APP_CAN_Decode(uint32_t id, uint8_t *data);
void APP_CAN_DecodePower(uint32_t id, uint8_t *data);
void APP_CAN_SendKeyState(uint8_t keyState);

#ifdef __cplusplus
}
#endif

#endif /* __APP_CAN_H */
