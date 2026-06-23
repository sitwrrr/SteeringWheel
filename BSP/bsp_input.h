/**
 * @file bsp_input.h
 * @brief 输入设备驱动（按键+拨片）
 * @version 1.0.0
 * @date 2025-01-01
 */

#ifndef __BSP_INPUT_H
#define __BSP_INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

/* Exported defines ----------------------------------------------------------*/
#define KEY_COUNT       8
#define SHIFT_COUNT     2

/* Exported types ------------------------------------------------------------*/
typedef enum {
    KEY_1 = 0,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    SHIFT_UP,
    SHIFT_DOWN
} KeyId_t;

typedef enum {
    INPUT_EVENT_NONE = 0,
    INPUT_EVENT_PRESS,
    INPUT_EVENT_RELEASE,
    INPUT_EVENT_LONG_PRESS,
    INPUT_EVENT_DOUBLE_PRESS
} InputEvent_t;

typedef struct {
    uint8_t state;
    uint8_t lastState;
    uint32_t pressTime;
    uint8_t longPressTriggered;
    uint8_t clickCount;
    uint32_t lastClickTime;
} InputState_t;

/* Exported functions --------------------------------------------------------*/
void BSP_Input_Init(void);
void BSP_Input_Scan(void);
uint8_t BSP_Input_IsPressed(KeyId_t id);
uint8_t BSP_Input_IsLongPressed(KeyId_t id);
InputEvent_t BSP_Input_GetEvent(KeyId_t id);
void BSP_Input_ClearEvent(KeyId_t id);

/* Callback functions */
void BSP_Input_EventCallback(KeyId_t id, InputEvent_t event);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_INPUT_H */
