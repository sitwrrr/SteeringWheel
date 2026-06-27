/**
 * @file bsp_ws2812b.h
 * @brief WS2812B RGB LED驱动
 * @version 1.0.0
 * @date 2025-01-01
 *
 * 硬件说明：
 * - 12个WS2812B RGB LED串联，单线归零码通信
 * - 数据格式：GRB顺序（Green-Red-Blue），每色8位，共24bit/LED
 * - 使用TIM1 CH1 + DMA方式产生800kHz PWM信号
 * - GPIO: PA8 -> TIM1_CH1 -> WS2812B DIN
 */

#ifndef __BSP_WS2812B_H
#define __BSP_WS2812B_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

/* Exported defines ----------------------------------------------------------*/
#define WS2812B_MAX_LED_NUM     12      /* LED串联数量 */

/* Exported functions --------------------------------------------------------*/

/**
 * @brief 初始化WS2812B，清空DMA缓冲区
 * @note  必须在其他BSP_WS2812B函数之前调用
 */
void BSP_WS2812B_Init(void);

/**
 * @brief 设置单个LED颜色（写入DMA缓冲区，不立即发送）
 * @param index LED索引（0 ~ WS2812B_MAX_LED_NUM-1）
 * @param r     红色分量（0-255）
 * @param g     绿色分量（0-255）
 * @param b     蓝色分量（0-255）
 * @note  超出范围的index会被忽略
 */
void BSP_WS2812B_SetPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief 发送缓冲区数据到LED（触发DMA传输）
 * @note  调用后LED才会更新显示，DMA完成后自动停止PWM
 */
void BSP_WS2812B_Update(void);

/**
 * @brief 清空所有LED颜色（设为黑色）
 * @note  只修改缓冲区，需要调用BSP_WS2812B_Update()才会生效
 */
void BSP_WS2812B_Clear(void);

/**
 * @brief 设置全局亮度（兼容旧接口，当前由APP层控制亮度）
 * @param brightness 亮度值（0-255），当前未使用
 */
void WS2812B_SetBrightness(uint8_t brightness);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_WS2812B_H */
