/**
 * @file bsp_ws2812b.h
 * @brief WS2812B RGB LED驱动
 * @version 1.0.0
 * @date 2025-01-01
 */

#ifndef __BSP_WS2812B_H
#define __BSP_WS2812B_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

/* Exported defines ----------------------------------------------------------*/
#define WS2812B_MAX_LED_NUM     12

/* Exported functions --------------------------------------------------------*/
void BSP_WS2812B_Init(void);
void BSP_WS2812B_SetPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b);
void BSP_WS2812B_Update(void);
void BSP_WS2812B_Clear(void);
void WS2812B_SetBrightness(uint8_t brightness);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_WS2812B_H */

