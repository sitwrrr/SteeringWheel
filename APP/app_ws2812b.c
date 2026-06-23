/**
 * @file app_ws2812b.c
 * @brief WS2812B RGB LED控制实现
 * @version 1.0.0
 * @date 2025-01-01
 */

#include "app_ws2812b.h"
#include "bsp_ws2812b.h"

/* Private variables ---------------------------------------------------------*/
static WS2812B_Mode_t currentMode = WS2812B_MODE_OFF;
static uint8_t currentR = 0, currentG = 0, currentB = 0;
static uint8_t brightness = WS2812B_BRIGHTNESS;
static uint32_t animationCounter = 0;

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 限制数值范围
 */
static uint8_t clamp(uint8_t value, uint8_t min, uint8_t max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

/**
 * @brief 计算RPM LED数量
 */
static uint8_t calcRpmLedCount(uint16_t rpm, uint16_t redLine)
{
    uint8_t count = (uint8_t)((uint32_t)rpm * WS2812B_LED_COUNT / redLine);
    return clamp(count, 0, WS2812B_LED_COUNT);
}

/**
 * @brief 获取RPM颜色
 */
static void getRpmColor(uint8_t index, uint8_t *r, uint8_t *g, uint8_t *b)
{
    /* 绿色 -> 黄色 -> 红色 */
    if (index < WS2812B_LED_COUNT / 3)
    {
        *r = 0;
        *g = 255;
        *b = 0;
    }
    else if (index < WS2812B_LED_COUNT * 2 / 3)
    {
        *r = 255;
        *g = 255;
        *b = 0;
    }
    else
    {
        *r = 255;
        *g = 0;
        *b = 0;
    }
}

/**
 * @brief HSV转RGB
 */
static void hsvToRgb(uint16_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b)
{
    uint8_t region = h / 43;
    uint8_t remainder = (h - (region * 43)) * 6;
    
    uint8_t p = (v * (255 - s)) >> 8;
    uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;
    
    switch (region)
    {
        case 0:  *r = v; *g = t; *b = p; break;
        case 1:  *r = q; *g = v; *b = p; break;
        case 2:  *r = p; *g = v; *b = t; break;
        case 3:  *r = p; *g = q; *b = v; break;
        case 4:  *r = t; *g = p; *b = v; break;
        default: *r = v; *g = p; *b = q; break;
    }
}

/* Exported functions --------------------------------------------------------*/

/**
 * @brief 初始化WS2812B
 */
void APP_WS2812B_Init(void)
{
    BSP_WS2812B_Init();
    APP_WS2812B_Off();
}

/**
 * @brief 更新WS2812B显示
 * @param rpm: 当前转速
 * @param mode: 工作模式
 */
void APP_WS2812B_Update(uint16_t rpm, WorkMode_t mode)
{
    animationCounter++;
    
    switch (currentMode)
    {
        case WS2812B_MODE_OFF:
            BSP_WS2812B_Clear();
            break;
            
        case WS2812B_MODE_RPM:
        {
            uint8_t ledCount = calcRpmLedCount(rpm, 8000);
            for (uint8_t i = 0; i < WS2812B_LED_COUNT; i++)
            {
                if (i < ledCount)
                {
                    uint8_t r, g, b;
                    getRpmColor(i, &r, &g, &b);
                    r = (uint16_t)r * brightness / 255;
                    g = (uint16_t)g * brightness / 255;
                    b = (uint16_t)b * brightness / 255;
                    BSP_WS2812B_SetPixel(i, r, g, b);
                }
                else
                {
                    BSP_WS2812B_SetPixel(i, 0, 0, 0);
                }
            }
            BSP_WS2812B_Update();
            break;
        }
            
        case WS2812B_MODE_RAINBOW:
        {
            uint16_t hue = animationCounter * 2;
            for (uint8_t i = 0; i < WS2812B_LED_COUNT; i++)
            {
                uint8_t r, g, b;
                hsvToRgb((hue + i * 256 / WS2812B_LED_COUNT) % 256, 255, brightness, &r, &g, &b);
                BSP_WS2812B_SetPixel(i, r, g, b);
            }
            BSP_WS2812B_Update();
            break;
        }
            
        case WS2812B_MODE_BREATHING:
        {
            uint8_t breath = (animationCounter % 100 < 50) ? 
                            (animationCounter % 100) * 255 / 50 :
                            (100 - animationCounter % 100) * 255 / 50;
            uint8_t val = (uint16_t)breath * brightness / 255;
            for (uint8_t i = 0; i < WS2812B_LED_COUNT; i++)
            {
                BSP_WS2812B_SetPixel(i, val, val, val);
            }
            BSP_WS2812B_Update();
            break;
        }
            
        case WS2812B_MODE_SOLID:
        {
            uint8_t r = (uint16_t)currentR * brightness / 255;
            uint8_t g = (uint16_t)currentG * brightness / 255;
            uint8_t b = (uint16_t)currentB * brightness / 255;
            for (uint8_t i = 0; i < WS2812B_LED_COUNT; i++)
            {
                BSP_WS2812B_SetPixel(i, r, g, b);
            }
            BSP_WS2812B_Update();
            break;
        }
    }
}

/**
 * @brief 设置WS2812B模式
 */
void APP_WS2812B_SetMode(WS2812B_Mode_t mode)
{
    currentMode = mode;
    animationCounter = 0;
}

/**
 * @brief 设置颜色（纯色模式使用）
 */
void APP_WS2812B_SetColor(uint8_t r, uint8_t g, uint8_t b)
{
    currentR = r;
    currentG = g;
    currentB = b;
}

/**
 * @brief 设置亮度
 */
void APP_WS2812B_SetBrightness(uint8_t b)
{
    brightness = clamp(b, 0, 255);
}

/**
 * @brief 关闭所有LED
 */
void APP_WS2812B_Off(void)
{
    currentMode = WS2812B_MODE_OFF;
    BSP_WS2812B_Clear();
    BSP_WS2812B_Update();
}
