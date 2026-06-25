/**
 * @file bsp_lcd.c
 * @brief LCD显示驱动实现
 * @version 1.0.0
 * @date 2025-01-01
 */

#include "bsp_lcd.h"
#include "fmc.h"
#include "gpio.h"
#include <stdlib.h>

/* Private defines ----------------------------------------------------------*/
#define LCD_BASE    (uint32_t)((0x60000000 + (0x4000000 * (LCD_FMC_NEX - 1))) | (((1 << LCD_FMC_AX) * 2) - 2))
#define LCD         ((LCD_TypeDef *)LCD_BASE)

#define LCD_WRITE_REG(value)    (LCD->LCD_REG = value)
#define LCD_WRITE_DATA(value)   (LCD->LCD_RAM = value)
#define LCD_READ_DATA()         (LCD->LCD_RAM)

/* Exported variables -------------------------------------------------------*/
LCD_Device_t lcddev;
uint16_t POINT_COLOR = 0xF800;  /* 默认红色 */
uint16_t BACK_COLOR = 0xFFFF;   /* 默认白色 */

/* Private functions --------------------------------------------------------*/

/**
 * @brief 写寄存器
 */
void LCD_WriteReg(uint16_t reg, uint16_t value)
{
    LCD_WRITE_REG(reg);
    LCD_WRITE_DATA(value);
}

/**
 * @brief 读寄存器
 */
uint16_t LCD_ReadReg(uint16_t reg)
{
    LCD_WRITE_REG(reg);
    return LCD_READ_DATA();
}

/**
 * @brief 准备写GRAM
 */
void LCD_WriteRAM_Prepare(void)
{
    LCD_WRITE_REG(0x2C);
}

/**
 * @brief 写GRAM
 */
void LCD_WriteRAM(uint16_t color)
{
    LCD_WRITE_DATA(color);
}

/**
 * @brief BGR转RGB
 */
uint16_t LCD_BGR2RGB(uint16_t color)
{
    uint16_t r, g, b, rgb;
    b = (color >> 0) & 0x1F;
    g = (color >> 5) & 0x3F;
    r = (color >> 11) & 0x1F;
    rgb = (b << 11) + (g << 5) + (r << 0);
    return rgb;
}

/* Exported functions -------------------------------------------------------*/

/**
 * @brief 初始化LCD
 */
void BSP_LCD_Init(void)
{
    HAL_Delay(100);
    
    /* LCD初始化序列（ST7796S/ILI9486通用） */
    LCD_WRITE_REG(0xF0);
    LCD_WRITE_DATA(0xC3);
    
    LCD_WRITE_REG(0xF0);
    LCD_WRITE_DATA(0x96);
    
    LCD_WRITE_REG(0x36);  /* Memory Access Control */
    LCD_WRITE_DATA(0x48); /* 竖屏，RGB顺序 */
    
    LCD_WRITE_REG(0x3A);  /* Interface Pixel Format */
    LCD_WRITE_DATA(0x55); /* 16bit/pixel (RGB565) */
    
    LCD_WRITE_REG(0xB4);
    LCD_WRITE_DATA(0x01);
    
    LCD_WRITE_REG(0xB1);
    LCD_WRITE_DATA(0x80);
    LCD_WRITE_DATA(0x10);
    
    LCD_WRITE_REG(0xC0);
    LCD_WRITE_DATA(0x80);
    LCD_WRITE_DATA(0x64);
    
    LCD_WRITE_REG(0xC1);
    LCD_WRITE_DATA(0x13);
    
    LCD_WRITE_REG(0xC2);
    LCD_WRITE_DATA(0xA7);
    
    LCD_WRITE_REG(0xC5);
    LCD_WRITE_DATA(0x09);
    
    LCD_WRITE_REG(0xE8);
    LCD_WRITE_DATA(0x40);
    LCD_WRITE_DATA(0x8A);
    LCD_WRITE_DATA(0x00);
    LCD_WRITE_DATA(0x00);
    LCD_WRITE_DATA(0x29);
    LCD_WRITE_DATA(0x19);
    LCD_WRITE_DATA(0xA5);
    LCD_WRITE_DATA(0x33);
    
    LCD_WRITE_REG(0xE0);
    LCD_WRITE_DATA(0xF0);
    LCD_WRITE_DATA(0x06);
    LCD_WRITE_DATA(0x0B);
    LCD_WRITE_DATA(0x07);
    LCD_WRITE_DATA(0x06);
    LCD_WRITE_DATA(0x05);
    LCD_WRITE_DATA(0x2E);
    LCD_WRITE_DATA(0x33);
    LCD_WRITE_DATA(0x47);
    LCD_WRITE_DATA(0x3A);
    LCD_WRITE_DATA(0x17);
    LCD_WRITE_DATA(0x16);
    LCD_WRITE_DATA(0x2E);
    LCD_WRITE_DATA(0x31);
    
    LCD_WRITE_REG(0xE1);
    LCD_WRITE_DATA(0xF0);
    LCD_WRITE_DATA(0x09);
    LCD_WRITE_DATA(0x0D);
    LCD_WRITE_DATA(0x09);
    LCD_WRITE_DATA(0x08);
    LCD_WRITE_DATA(0x23);
    LCD_WRITE_DATA(0x2E);
    LCD_WRITE_DATA(0x33);
    LCD_WRITE_DATA(0x46);
    LCD_WRITE_DATA(0x38);
    LCD_WRITE_DATA(0x13);
    LCD_WRITE_DATA(0x13);
    LCD_WRITE_DATA(0x2C);
    LCD_WRITE_DATA(0x32);
    
    LCD_WRITE_REG(0xF0);
    LCD_WRITE_DATA(0x3C);
    
    LCD_WRITE_REG(0xF0);
    LCD_WRITE_DATA(0x69);
    
    LCD_WRITE_REG(0x21);  /* Enter_invert_mode */
    
    LCD_WRITE_REG(0x11);  /* Exit Sleep */
    HAL_Delay(120);
    
    LCD_WRITE_REG(0x29);  /* Display ON */
    HAL_Delay(50);
    
    LCD_WRITE_REG(0x2C);  /* Write memory start */
    
    BSP_LCD_Clear(BACK_COLOR);
}

/**
 * @brief 清屏
 */
void BSP_LCD_Clear(uint16_t color)
{
    uint32_t total = LCD_WIDTH * LCD_HEIGHT;
    
    BSP_LCD_SetCursor(0, 0);
    LCD_WriteRAM_Prepare();
    
    for (uint32_t i = 0; i < total; i++)
    {
        LCD_WRITE_DATA(color);
    }
}

/**
 * @brief 设置光标位置
 */
void BSP_LCD_SetCursor(uint16_t x, uint16_t y)
{
    LCD_WRITE_REG(0x2A);
    LCD_WRITE_DATA(x >> 8);
    LCD_WRITE_DATA(x & 0xFF);
    
    LCD_WRITE_REG(0x2B);
    LCD_WRITE_DATA(y >> 8);
    LCD_WRITE_DATA(y & 0xFF);
}

/**
 * @brief 写像素
 */
void BSP_LCD_WritePixel(uint16_t x, uint16_t y, uint16_t color)
{
    BSP_LCD_SetCursor(x, y);
    LCD_WriteRAM_Prepare();
    LCD_WRITE_DATA(color);
}

/**
 * @brief 读像素
 */
uint16_t BSP_LCD_ReadPixel(uint16_t x, uint16_t y)
{
    BSP_LCD_SetCursor(x, y);
    LCD_WRITE_REG(0x2E);
    LCD_READ_DATA();  /* dummy read */
    return LCD_READ_DATA();
}

/**
 * @brief 填充矩形
 */
void BSP_LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    for (uint16_t i = y; i < y + h; i++)
    {
        BSP_LCD_SetCursor(x, i);
        LCD_WriteRAM_Prepare();
        for (uint16_t j = x; j < x + w; j++)
        {
            LCD_WRITE_DATA(color);
        }
    }
}

/**
 * @brief 画线
 */
void BSP_LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    int16_t dx = abs(x2 - x1);
    int16_t dy = abs(y2 - y1);
    int16_t sx = (x1 < x2) ? 1 : -1;
    int16_t sy = (y1 < y2) ? 1 : -1;
    int16_t err = dx - dy;
    
    while (1)
    {
        BSP_LCD_WritePixel(x1, y1, POINT_COLOR);
        
        if (x1 == x2 && y1 == y2) break;
        
        int16_t e2 = 2 * err;
        if (e2 > -dy)
        {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            y1 += sy;
        }
    }
}

/**
 * @brief 画矩形
 */
void BSP_LCD_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    BSP_LCD_DrawLine(x, y, x + w, y);
    BSP_LCD_DrawLine(x + w, y, x + w, y + h);
    BSP_LCD_DrawLine(x + w, y + h, x, y + h);
    BSP_LCD_DrawLine(x, y + h, x, y);
}

/**
 * @brief 画圆
 */
void BSP_LCD_DrawCircle(uint16_t x, uint16_t y, uint8_t r)
{
    int16_t a = 0, b = r;
    int16_t d = 3 - 2 * r;
    
    while (a <= b)
    {
        BSP_LCD_WritePixel(x + a, y - b, POINT_COLOR);
        BSP_LCD_WritePixel(x + b, y - a, POINT_COLOR);
        BSP_LCD_WritePixel(x + b, y + a, POINT_COLOR);
        BSP_LCD_WritePixel(x + a, y + b, POINT_COLOR);
        BSP_LCD_WritePixel(x - a, y + b, POINT_COLOR);
        BSP_LCD_WritePixel(x - b, y + a, POINT_COLOR);
        BSP_LCD_WritePixel(x - b, y - a, POINT_COLOR);
        BSP_LCD_WritePixel(x - a, y - b, POINT_COLOR);
        
        if (d < 0)
        {
            d += 4 * a + 6;
        }
        else
        {
            d += 10 + 4 * (a - b);
            b--;
        }
        a++;
    }
}

/**
 * @brief 设置背光
 */
void BSP_LCD_SetBacklight(uint8_t state)
{
    if (state)
    {
        HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin, GPIO_PIN_RESET);
    }
}
