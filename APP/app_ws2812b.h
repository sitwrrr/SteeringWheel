/**
 * @file app_ws2812b.h
 * @brief WS2812B RGB LED控制（APP层）
 * @version 1.0.0
 * @date 2025-01-01
 *
 * 功能说明：
 * - 5种LED模式：关闭、RPM、彩虹、呼吸、纯色
 * - 超转速闪烁警告（RPM达到红线时绿灯闪烁）
 * - 亮度控制（0-255）
 * - HSV转RGB颜色空间转换
 */

#ifndef __APP_WS2812B_H
#define __APP_WS2812B_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include "Variable.h"

/* Exported defines ----------------------------------------------------------*/
#define WS2812B_LED_COUNT   12      /* LED数量（与BSP层WS2812B_MAX_LED_NUM一致） */
#define WS2812B_BRIGHTNESS  50      /* 默认亮度（0-255） */

/* Exported types ------------------------------------------------------------*/
typedef enum {
    WS2812B_MODE_OFF = 0,       /* 关闭 */
    WS2812B_MODE_RPM,           /* RPM模式（转速指示+超转速闪烁） */
    WS2812B_MODE_RAINBOW,       /* 彩虹模式（循环渐变色） */
    WS2812B_MODE_BREATHING,     /* 呼吸模式（亮度渐变） */
    WS2812B_MODE_SOLID          /* 纯色模式（固定颜色） */
} WS2812B_Mode_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief 初始化WS2812B应用层
 * @note  调用BSP层初始化并关闭LED
 */
void APP_WS2812B_Init(void);

/**
 * @brief 更新WS2812B显示（需在任务中周期调用）
 * @param rpm  当前转速（RPM模式使用）
 * @param mode 当前工作模式（用于判断是否需要更新）
 * @note  不同模式的行为：
 *        OFF: 关闭所有LED
 *        RPM: 根据转速点亮LED，超转速时绿灯闪烁
 *        RAINBOW: 彩虹渐变动画
 *        BREATHING: 呼吸灯效果
 *        SOLID: 显示设定的纯色
 */
void APP_WS2812B_Update(uint16_t rpm, WorkMode_t mode);

/**
 * @brief 设置LED模式
 * @param mode 目标模式
 */
void APP_WS2812B_SetMode(WS2812B_Mode_t mode);

/**
 * @brief 设置纯色模式的颜色
 * @param r 红色（0-255）
 * @param g 绿色（0-255）
 * @param b 蓝色（0-255）
 * @note  仅在WS2812B_MODE_SOLID模式下生效
 */
void APP_WS2812B_SetColor(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief 设置全局亮度
 * @param b 亮度值（0-255）
 */
void APP_WS2812B_SetBrightness(uint8_t b);

/**
 * @brief 关闭所有LED
 */
void APP_WS2812B_Off(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_WS2812B_H */
