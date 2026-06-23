/**
 * @file app_ws2812b.h
 * @brief WS2812B RGB LED控制
 * @version 1.0.0
 * @date 2025-01-01
 */

#ifndef __APP_WS2812B_H
#define __APP_WS2812B_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include "Variable.h"

/* Exported defines ----------------------------------------------------------*/
#define WS2812B_LED_COUNT   12
#define WS2812B_BRIGHTNESS  50  /* 0-255 */

/* Exported types ------------------------------------------------------------*/
typedef enum {
    WS2812B_MODE_OFF = 0,       /* 关闭 */
    WS2812B_MODE_RPM,           /* RPM模式 */
    WS2812B_MODE_RAINBOW,       /* 彩虹模式 */
    WS2812B_MODE_BREATHING,     /* 呼吸模式 */
    WS2812B_MODE_SOLID          /* 纯色模式 */
} WS2812B_Mode_t;

/* Exported functions --------------------------------------------------------*/
void APP_WS2812B_Init(void);
void APP_WS2812B_Update(uint16_t rpm, WorkMode_t mode);
void APP_WS2812B_SetMode(WS2812B_Mode_t mode);
void APP_WS2812B_SetColor(uint8_t r, uint8_t g, uint8_t b);
void APP_WS2812B_SetBrightness(uint8_t brightness);
void APP_WS2812B_Off(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_WS2812B_H */
