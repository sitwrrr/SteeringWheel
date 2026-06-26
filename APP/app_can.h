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

#include "Variable.h"

/* Exported defines ----------------------------------------------------------*/
/* FDCAN1 标准ID */
#define CAN_ID_VCU_SUMMARY     0x211    /* VCU汇总: 车速/踏板/安全信号 */

/* FDCAN1 扩展ID (VCU↔MCU通信，仪表旁听) */
#define CAN_ID_VCU_TO_MCU1     0x08C1EF21  /* VCU→左电机 */
#define CAN_ID_VCU_TO_MCU2     0x08B1EF21  /* VCU→右电机 */
#define CAN_ID_MCU1_TO_VCU     0x0CFFC6EF  /* 左电机→VCU (状态) */
#define CAN_ID_MCU2_TO_VCU     0x0CB221EF  /* 右电机→VCU (状态) */
#define CAN_ID_MCU1_TO_VCU2    0x0CFFC7EF  /* 左电机→VCU (温度/电压/电流) */
#define CAN_ID_MCU2_TO_VCU2    0x0CB321EF  /* 右电机→VCU (温度/电压/电流) */

/* FDCAN2 标准ID */
#define CAN_ID_IMU             0x050    /* IMU传感器: 加速度/姿态角 */

/* FDCAN2 扩展ID (BMS) */
#define CAN_ID_BMS_STATUS      0x186040F3  /* BMS: 电压/电流/SOC/状态 */
#define CAN_ID_BMS_CELL_VOLT   0x186140F3  /* BMS: 最高/最低单体电压 */

/* Exported functions --------------------------------------------------------*/
void APP_CAN_Init(void);
void APP_CAN_TaskProcess(void);
void APP_CAN_Decode(uint32_t id, uint8_t *data);
void APP_CAN_DecodePower(uint32_t id, uint8_t *data);
void APP_CAN_SendKeyState(uint8_t keyState);

#ifdef __cplusplus
}
#endif

#endif /* __APP_CAN_H */
