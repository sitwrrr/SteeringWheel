/**
 * @file app_ec200.h
 * @brief EC200 MQTT应用头文件
 * @version 1.0.0
 * @date 2025-01-01
 */

#ifndef __APP_EC200_H
#define __APP_EC200_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include "Variable.h"

/* Exported defines ----------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
uint8_t APP_EC200_Init(void);
void APP_EC200_Upload(void);
void APP_EC200_UploadData(VehicleData_t *data);
uint8_t APP_EC200_IsConnected(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_EC200_H */
