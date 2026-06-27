/**
 * @file app_input.h
 * @brief 输入处理应用（按键+拨片）
 * @version 1.0.0
 * @date 2025-01-01
 *
 * 功能说明：
 * - 调用BSP层扫描，处理按键事件
 * - 短按事件通过函数指针表分发到各按键处理函数
 * - 长按/短按/换挡事件通过__weak回调暴露，外部可重写
 * - 按键状态存入g_vehicleData.key_state位掩码（CAN上报用）
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

/**
 * @brief 初始化输入应用
 */
void APP_Input_Init(void);

/**
 * @brief 扫描并处理输入事件（需定期调用，建议20ms周期）
 * @note  处理流程：
 *        1. 调用BSP_Input_Scan()扫描硬件
 *        2. 更新g_vehicleData.key_state位掩码
 *        3. 遍历按键事件：短按→keyHandlers函数指针+PressCallback，长按→LongPressCallback
 *        4. 更新拨片状态：shift_up/shift_down
 *        5. 有拨片输入→ShiftCallback
 */
void APP_Input_Scan(void);

/**
 * @brief 处理单个输入事件（外部可调用的API）
 * @param id 按键ID
 * @param event 事件类型
 */
void APP_Input_ProcessEvent(KeyId_t id, InputEvent_t event);

/**
 * @brief 发送按键状态CAN报文
 */
void APP_Input_SendCAN(void);

/**
 * @brief 按键短按回调（弱定义，外部重写实现自定义逻辑）
 * @param id 按键ID
 */
void APP_Input_PressCallback(KeyId_t id);

/**
 * @brief 按键长按回调（弱定义，外部重写实现自定义逻辑）
 * @param id 按键ID
 */
void APP_Input_LongPressCallback(KeyId_t id);

/**
 * @brief 换挡回调（弱定义，外部重写实现自定义逻辑）
 * @param up 升挡状态：0=未按下，1=按下
 * @param down 降挡状态：0=未按下，1=按下
 */
void APP_Input_ShiftCallback(uint8_t up, uint8_t down);

#ifdef __cplusplus
}
#endif

#endif /* __APP_INPUT_H */
