/**
 * @file bsp_lcd.c
 * @brief LCD显示驱动实现（NV3041A 480x272）
 * @version 2.0.0
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

static void LCD_WriteReg(uint16_t reg, uint16_t value)
{
    LCD_WRITE_REG(reg);
    LCD_WRITE_DATA(value);
}

static uint16_t LCD_ReadReg(uint16_t reg)
{
    LCD_WRITE_REG(reg);
    return LCD_READ_DATA();
}

static void LCD_WriteRAM_Prepare(void)
{
    LCD_WRITE_REG(0x2C);
}

static void LCD_WriteRAM(uint16_t color)
{
    LCD_WRITE_DATA(color);
}

/* Exported functions -------------------------------------------------------*/

/**
 * @brief 初始化LCD（NV3041A 480x272 8080接口）
 */
void BSP_LCD_Init(void)
{
    /* 硬件复位后等待 */
    HAL_Delay(100);

    /* Sleep Out */
    LCD_WRITE_REG(0x11);
    HAL_Delay(120);

    /* Memory Data Access Control: 0x00=正常, 0x60=90°, 0xC0=180°, 0xA0=270° */
    /* bit7:MY 行地址顺序, bit6:MX 列地址顺序, bit5:MV 行列交换 */
    /* bit4:BGR RGB顺序, bit3:MH 刷新方向 */
    LCD_WRITE_REG(0x36);
    LCD_WRITE_DATA(0x00);   /* 正常方向，RGB顺序 */

    /* Interface Pixel Format: 0x55=RGB565(16bit) */
    LCD_WRITE_REG(0x3A);
    LCD_WRITE_DATA(0x55);

    /* Column Address Set: 0x2A, start=0, end=479(0x01DF) */
    LCD_WRITE_REG(0x2A);
    LCD_WRITE_DATA(0x00);
    LCD_WRITE_DATA(0x00);
    LCD_WRITE_DATA(0x01);
    LCD_WRITE_DATA(0xDF);

    /* Row Address Set: 0x2B, start=0, end=271(0x010F) */
    LCD_WRITE_REG(0x2B);
    LCD_WRITE_DATA(0x00);
    LCD_WRITE_DATA(0x00);
    LCD_WRITE_DATA(0x01);
    LCD_WRITE_DATA(0x0F);

    /* Normal Display Mode On */
    LCD_WRITE_REG(0x13);

    /* Display Inversion On (NV3041A通常需要反色) */
    LCD_WRITE_REG(0x21);

    /* Display ON */
    LCD_WRITE_REG(0x29);
    HAL_Delay(50);

    /* Write Memory Start */
    LCD_WRITE_REG(0x2C);

    /* 清屏为白色 */
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
 * @brief 画线（Bresenham算法）
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
        HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin, GPIO_PIN_SET);
    else
        HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin, GPIO_PIN_RESET);
}
