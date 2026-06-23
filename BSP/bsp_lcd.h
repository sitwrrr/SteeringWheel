/**
 * @file bsp_lcd.h
 * @brief LCD显示驱动
 * @version 1.0.0
 * @date 2025-01-01
 */

#ifndef __BSP_LCD_H
#define __BSP_LCD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

/* Exported defines ----------------------------------------------------------*/
#define LCD_FMC_NEX         1
#define LCD_FMC_AX          19

#define LCD_WIDTH           480
#define LCD_HEIGHT          320

/* Exported types ------------------------------------------------------------*/
typedef struct {
    volatile uint16_t LCD_REG;
    volatile uint16_t LCD_RAM;
} LCD_TypeDef;

typedef struct {
    uint16_t width;
    uint16_t height;
    uint16_t id;
    uint8_t dir;
    uint16_t wramcmd;
    uint16_t setxcmd;
    uint16_t setycmd;
} LCD_Device_t;

/* Exported variables --------------------------------------------------------*/
extern LCD_Device_t lcddev;
extern uint16_t POINT_COLOR;
extern uint16_t BACK_COLOR;

/* Exported functions --------------------------------------------------------*/
void BSP_LCD_Init(void);
void BSP_LCD_Clear(uint16_t color);
void BSP_LCD_SetCursor(uint16_t x, uint16_t y);
void BSP_LCD_WritePixel(uint16_t x, uint16_t y, uint16_t color);
uint16_t BSP_LCD_ReadPixel(uint16_t x, uint16_t y);
void BSP_LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void BSP_LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void BSP_LCD_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void BSP_LCD_DrawCircle(uint16_t x, uint16_t y, uint8_t r);
void BSP_LCD_SetBacklight(uint8_t state);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_LCD_H */

