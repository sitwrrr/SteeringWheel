/**
 * @file bsp_input.h
 * @brief 输入设备驱动（按键+拨片）
 * @version 1.0.0
 * @date 2025-01-01
 *
 * 硬件说明：
 * - 8个独立按键（KEY1-KEY8），低电平有效，内部上拉
 * - 2个换挡拨片（SHIFT_UP/SHIFT_DOWN），低电平有效
 * - 所有输入通过GPIO轮询方式读取
 *
 * 功能特性：
 * - 消抖20ms
 * - 长按检测1000ms
 * - 双击检测300ms
 * - 事件驱动架构（PRESS/RELEASE/LONG_PRESS/DOUBLE_PRESS）
 */

#ifndef __BSP_INPUT_H
#define __BSP_INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

/* Exported defines ----------------------------------------------------------*/
#define KEY_COUNT       8       /* 独立按键数量 */
#define SHIFT_COUNT     2       /* 换挡拨片数量 */

/* Exported types ------------------------------------------------------------*/

/**
 * @brief 输入设备ID枚举
 * @note  KEY_1~KEY_8为按键，SHIFT_UP/SHIFT_DOWN为拨片
 */
typedef enum {
    KEY_1 = 0,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    SHIFT_UP,       /* 升挡拨片 */
    SHIFT_DOWN      /* 降挡拨片 */
} KeyId_t;

/**
 * @brief 输入事件类型
 */
typedef enum {
    INPUT_EVENT_NONE = 0,           /* 无事件 */
    INPUT_EVENT_PRESS,              /* 短按（按下后释放，<1000ms） */
    INPUT_EVENT_RELEASE,            /* 释放 */
    INPUT_EVENT_LONG_PRESS,         /* 长按（持续按住>1000ms） */
    INPUT_EVENT_DOUBLE_PRESS        /* 双击（300ms内连续按两次） */
} InputEvent_t;

/**
 * @brief 输入设备状态结构体
 */
typedef struct {
    uint8_t state;                  /* 当前状态：0=释放，1=按下 */
    uint8_t lastState;              /* 上次状态（用于消抖） */
    uint32_t pressTime;             /* 按下时刻（ms） */
    uint8_t longPressTriggered;     /* 长按已触发标志 */
    uint8_t clickCount;             /* 连续点击计数（双击检测用） */
    uint32_t lastClickTime;         /* 上次点击时刻（双击间隔判断用） */
} InputState_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief 初始化输入设备，清零所有状态
 */
void BSP_Input_Init(void);

/**
 * @brief 扫描所有输入设备（需定期调用，建议20ms周期）
 * @note  非阻塞，用HAL_GetTick()时间戳判断，不会卡住FreeRTOS任务
 *        每次调用：读GPIO→消抖→检测状态变化→设置事件标志
 */
void BSP_Input_Scan(void);

/**
 * @brief 查询按键是否按下
 * @param id 按键ID
 * @return 0=释放，1=按下
 */
uint8_t BSP_Input_IsPressed(KeyId_t id);

/**
 * @brief 查询按键是否处于长按状态
 * @param id 按键ID
 * @return 0=未长按，1=已长按（持续按住超过1000ms）
 */
uint8_t BSP_Input_IsLongPressed(KeyId_t id);

/**
 * @brief 获取按键事件
 * @param id 按键ID
 * @return 事件类型（INPUT_EVENT_*）
 * @note  事件读取后需调用BSP_Input_ClearEvent()清除
 */
InputEvent_t BSP_Input_GetEvent(KeyId_t id);

/**
 * @brief 清除按键事件
 * @param id 按键ID
 */
void BSP_Input_ClearEvent(KeyId_t id);

/**
 * @brief 按键事件回调（弱定义，可重写）
 * @param id 按键ID
 * @param event 事件类型
 */
void BSP_Input_EventCallback(KeyId_t id, InputEvent_t event);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_INPUT_H */
