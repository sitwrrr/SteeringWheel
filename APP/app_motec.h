/**
 * @file app_motec.h
 * @brief MoTeC ECU CAN协议解析（燃油车）
 * @version 1.0.0
 * @date 2025-01-01
 */

#ifndef __APP_MOTEC_H
#define __APP_MOTEC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

/* Exported defines ----------------------------------------------------------*/
#define MOTEC_CAN_ID            0x5F0       /* MoTeC ECU CAN ID */
#define MOTEC_FRAME_COUNT       18          /* 18帧为一组 */
#define MOTEC_FRAME_SIZE        8           /* 每帧8字节 */

/* 帧头标识 */
#define MOTEC_HEADER_BYTE4      0xFC
#define MOTEC_HEADER_BYTE5      0xFB
#define MOTEC_HEADER_BYTE6      0xFA

/* Exported types ------------------------------------------------------------*/
typedef struct {
    uint16_t engineRPM;         /* 发动机转速 RPM */
    uint16_t throttlePosition;  /* 节气门开度 % (raw/10) */
    uint16_t engineTemp;        /* 发动机温度 ℃ (raw/10) */
    uint16_t lambda1;           /* Lambda值 (raw/1000) */
    uint16_t oilTemp;           /* 机油温度 ℃ (raw/10) */
    uint16_t oilPressure;       /* 机油压力 (raw/10) */
    uint16_t lowBatVol;         /* 低压电池电压 (raw/10) */
    uint16_t ecuTemp;           /* ECU温度 ℃ (raw/10) */
    uint16_t gear;              /* 档位 */
    uint8_t carType;            /* 车型: 0=电车, 1=油车 */
    float    frontSpeed;        /* 车速 km/h */
} MotecData_t;

/* Exported variables --------------------------------------------------------*/
extern MotecData_t g_motecData;

/* Exported functions --------------------------------------------------------*/
void APP_Motec_Init(void);
void APP_Motec_ProcessFrame(uint8_t *data);
uint8_t APP_Motec_GetCarType(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_MOTEC_H */
