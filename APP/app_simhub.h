/**
 * @file app_simhub.h
 * @brief SimHub模拟器数据解析
 * @version 1.0.0
 * @date 2025-01-01
 */

#ifndef __APP_SIMHUB_H
#define __APP_SIMHUB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include "Variable.h"

/* Exported defines ----------------------------------------------------------*/
#define SIMHUB_BUFFER_SIZE  512

/* Exported types ------------------------------------------------------------*/
typedef struct {
    uint16_t rpm;
    uint16_t speed;
    uint8_t gear;
    uint8_t throttle;
    uint8_t brake;
    uint8_t fuel;
    uint16_t redLineRPM;
    char *bLapTime;
    char *cLapTime;
    uint8_t lap;
} SimHubData_t;

/* Exported functions --------------------------------------------------------*/
void APP_SimHub_Init(void);
void APP_SimHub_Process(void);
void APP_SimHub_ParseJSON(const char *json);
void APP_SimHub_UpdateVehicleData(void);

/* 回调函数 */
void APP_SimHub_DataReceivedCallback(VehicleData_t *data);

#ifdef __cplusplus
}
#endif

#endif /* __APP_SIMHUB_H */
