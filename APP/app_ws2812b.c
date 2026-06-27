/**
 * @file app_ws2812b.c
 * @brief WS2812B RGB LED控制实现（APP层）
 * @version 1.0.0
 * @date 2025-01-01
 *
 * 模式说明：
 *   OFF       - 关闭所有LED
 *   RPM       - 根据转速点亮LED（绿→黄→红），超转速时绿灯闪烁警告
 *   RAINBOW   - 彩虹渐变动画（HSV色彩空间旋转）
 *   BREATHING - 呼吸灯效果（亮度渐变）
 *   SOLID     - 纯色模式（固定颜色+亮度）
 *
 * 使用方式：
 *   1. 调用APP_WS2812B_Init()初始化
 *   2. 在FreeRTOS任务中周期调用APP_WS2812B_Update(rpm, mode)
 *   3. 通过APP_WS2812B_SetMode()切换模式
 */

#include "app_ws2812b.h"
#include "app_simhub.h"  /* SW-L6修复: 获取红线转速 */
#include "bsp_ws2812b.h"

/* Private variables ---------------------------------------------------------*/
static WS2812B_Mode_t currentMode = WS2812B_MODE_OFF;  /* 当前LED模式 */
static uint8_t currentR = 0, currentG = 0, currentB = 0;  /* 纯色模式的颜色值 */
static uint8_t brightness = WS2812B_BRIGHTNESS;  /* 全局亮度（0-255） */
static uint32_t animationCounter = 0;  /* 动画帧计数器，每次Update递增 */

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 限制数值范围
 * @param value 输入值
 * @param min   最小值
 * @param max   最大值
 * @return 限制后的值
 */
static uint8_t clamp(uint8_t value, uint8_t min, uint8_t max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

/**
 * @brief 根据转速计算应点亮的LED数量
 * @param rpm     当前转速
 * @param redLine 红线转速（所有LED亮起的转速阈值）
 * @return 点亮的LED数量（0 ~ WS2812B_LED_COUNT）
 *
 * 计算公式：ledCount = rpm * LED总数 / 红线转速
 * 例：rpm=4000, redLine=8000 → ledCount=6（点亮一半LED）
 */
static uint8_t calcRpmLedCount(uint16_t rpm, uint16_t redLine)
{
    uint8_t count = (uint8_t)((uint32_t)rpm * WS2812B_LED_COUNT / redLine);
    return clamp(count, 0, WS2812B_LED_COUNT);
}

/**
 * @brief 根据LED索引获取RPM指示颜色
 * @param index LED索引（0-11）
 * @param r     输出：红色分量
 * @param g     输出：绿色分量
 * @param b     输出：蓝色分量
 *
 * 颜色分区（12个LED）：
 *   索引 0-3   → 绿色（低转速安全区）
 *   索引 4-7   → 黄色（中转速警告区）
 *   索引 8-11  → 红色（高转速危险区）
 */
static void getRpmColor(uint8_t index, uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (index < WS2812B_LED_COUNT / 3)         /* 前1/3：绿色 */
    {
        *r = 0;
        *g = 255;
        *b = 0;
    }
    else if (index < WS2812B_LED_COUNT * 2 / 3)  /* 中1/3：黄色 */
    {
        *r = 255;
        *g = 255;
        *b = 0;
    }
    else                                          /* 后1/3：红色 */
    {
        *r = 255;
        *g = 0;
        *b = 0;
    }
}

/**
 * @brief HSV颜色空间转RGB
 * @param h 色相（0-255，对应0°-360°）
 * @param s 饱和度（0-255）
 * @param v 明度（0-255）
 * @param r 输出：红色分量
 * @param g 输出：绿色分量
 * @param b 输出：蓝色分量
 *
 * 将色相分为6个区域（每区域43个值），通过线性插值计算RGB
 */
static void hsvToRgb(uint16_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b)
{
    uint8_t region = h / 43;           /* 色相区域（0-5） */
    uint8_t remainder = (h - (region * 43)) * 6;  /* 区域内偏移 */

    /* 插值计算中间变量 */
    uint8_t p = (v * (255 - s)) >> 8;
    uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    /* 根据区域分配RGB值 */
    switch (region)
    {
        case 0:  *r = v; *g = t; *b = p; break;  /* 红→黄 */
        case 1:  *r = q; *g = v; *b = p; break;  /* 黄→绿 */
        case 2:  *r = p; *g = v; *b = t; break;  /* 绿→青 */
        case 3:  *r = p; *g = q; *b = v; break;  /* 青→蓝 */
        case 4:  *r = t; *g = p; *b = v; break;  /* 蓝→紫 */
        default: *r = v; *g = p; *b = q; break;  /* 紫→红 */
    }
}

/* Exported functions --------------------------------------------------------*/

/**
 * @brief 初始化WS2812B应用层
 */
void APP_WS2812B_Init(void)
{
    BSP_WS2812B_Init();     /* 初始化BSP层（清空DMA缓冲区） */
    APP_WS2812B_Off();      /* 默认关闭所有LED */
}

/**
 * @brief 更新WS2812B显示（需在任务中周期调用）
 * @param rpm  当前转速（RPM模式使用）
 * @param mode 当前工作模式
 *
 * 注意：mode参数当前未使用，实际模式由currentMode控制
 * 通过APP_WS2812B_SetMode()切换模式
 */
void APP_WS2812B_Update(uint16_t rpm, WorkMode_t mode)
{
    animationCounter++;

    switch (currentMode)
    {
        /* ===== 关闭模式 ===== */
        case WS2812B_MODE_OFF:
            BSP_WS2812B_Clear();
            BSP_WS2812B_Update();
            break;

        /* ===== RPM转速指示模式 ===== */
        case WS2812B_MODE_RPM:
        {
            uint16_t redLine = APP_SimHub_GetRedLineRPM();  /* SW-L6修复: 从SimHub获取红线转速 */
            uint8_t ledCount = calcRpmLedCount(rpm, redLine);

            /* 超转速闪烁警告：RPM达到红线时闪烁 */
            if (ledCount >= WS2812B_LED_COUNT)
            {
                static uint8_t blinkState = 0;
                blinkState = !blinkState;
                if (blinkState)
                {
                    /* 亮：实车模式绿色，模拟器模式红色 */
                    uint8_t r = (mode == MODE_SIMULATOR) ? brightness : 0;
                    uint8_t g = (mode == MODE_SIMULATOR) ? 0 : brightness;
                    for (uint8_t i = 0; i < WS2812B_LED_COUNT; i++)
                    {
                        BSP_WS2812B_SetPixel(i, r, g, 0);
                    }
                }
                else
                {
                    /* 灭：清空所有LED */
                    BSP_WS2812B_Clear();
                }
                BSP_WS2812B_Update();
                break;
            }

            /* 正常转速：按转速比例点亮LED，颜色从绿→黄→红渐变 */
            for (uint8_t i = 0; i < WS2812B_MAX_LED_NUM; i++)
            {
                if (i < ledCount)
                {
                    uint8_t r, g, b;
                    getRpmColor(i, &r, &g, &b);
                    /* 应用亮度缩放 */
                    r = (uint16_t)r * brightness / 255;
                    g = (uint16_t)g * brightness / 255;
                    b = (uint16_t)b * brightness / 255;
                    BSP_WS2812B_SetPixel(i, r, g, b);
                }
                else
                {
                    BSP_WS2812B_SetPixel(i, 0, 0, 0);  /* 未点亮的LED */
                }
            }
            BSP_WS2812B_Update();
            break;
        }

        /* ===== 彩虹渐变模式 ===== */
        case WS2812B_MODE_RAINBOW:
        {
            uint16_t hue = animationCounter * 2;  /* 色相随时间旋转 */
            for (uint8_t i = 0; i < WS2812B_LED_COUNT; i++)
            {
                uint8_t r, g, b;
                /* 每个LED色相偏移，形成彩虹效果 */
                hsvToRgb((hue + i * 256 / WS2812B_LED_COUNT) % 256, 255, brightness, &r, &g, &b);
                BSP_WS2812B_SetPixel(i, r, g, b);
            }
            BSP_WS2812B_Update();
            break;
        }

        /* ===== 呼吸灯模式 ===== */
        case WS2812B_MODE_BREATHING:
        {
            /* 呼吸周期100帧：0→50渐亮，50→100渐暗 */
            uint8_t breath = (animationCounter % 100 < 50) ?
                            (animationCounter % 100) * 255 / 50 :
                            (100 - animationCounter % 100) * 255 / 50;
            uint8_t val = (uint16_t)breath * brightness / 255;  /* 应用亮度限制 */
            for (uint8_t i = 0; i < WS2812B_LED_COUNT; i++)
            {
                BSP_WS2812B_SetPixel(i, val, val, val);  /* 白色呼吸 */
            }
            BSP_WS2812B_Update();
            break;
        }

        /* ===== 纯色模式 ===== */
        case WS2812B_MODE_SOLID:
        {
            /* 应用亮度缩放到设定颜色 */
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
 * @brief 设置LED模式
 * @param mode 目标模式
 * @note  切换模式时重置动画计数器
 */
void APP_WS2812B_SetMode(WS2812B_Mode_t mode)
{
    currentMode = mode;
    animationCounter = 0;  /* 重置动画，避免切换时跳变 */
}

/**
 * @brief 设置纯色模式的颜色
 * @param r 红色（0-255）
 * @param g 绿色（0-255）
 * @param b 蓝色（0-255）
 * @note  仅存储颜色值，需要在SOLID模式下Update才会显示
 */
void APP_WS2812B_SetColor(uint8_t r, uint8_t g, uint8_t b)
{
    currentR = r;
    currentG = g;
    currentB = b;
}

/**
 * @brief 设置全局亮度
 * @param b 亮度值（0-255）
 * @note  所有模式的颜色都会乘以brightness/255进行缩放
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
