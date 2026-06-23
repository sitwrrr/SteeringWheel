/**
 * @file app_input.h
 * @brief 输入处理应用（按键+拨片）
 * @version 1.0.0
 * @date 2025-01-01
 */

#ifndef __APP_INPUT_H
#define __APP_INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include "bsp_input.h"
#include "Variable.h"

/* Exported functions --------------------------------------------------------*/
void APP_Input_Init(void);
void APP_Input_Scan(void);
void APP_Input_ProcessEvent(KeyId_t id, InputEvent_t event);
void APP_Input_SendCAN(void);

/* 按键回调 */
void APP_Input_PressCallback(KeyId_t id);
void APP_Input_LongPressCallback(KeyId_t id);
void APP_Input_ShiftCallback(uint8_t up, uint8_t down);

#ifdef __cplusplus
}
#endif

#endif /* __APP_INPUT_H */
