/**
 * @file bsp_ws2812b.c
 * @brief WS2812B RGB LED驱动实现
 * @version 1.0.0
 * @date 2025-01-01
 */

#include "bsp_ws2812b.h"
#include "tim.h"

/* Private defines ----------------------------------------------------------*/
#define ONE_PULSE       ((htim1.Init.Period + 1) / 3 * 2 - 1)
#define ZERO_PULSE      ((htim1.Init.Period + 1) / 3 - 1)
#define RESET_PULSE     230
#define LED_DATA_LEN    24
#define HTIM            htim1
#define CHANNEL         TIM_CHANNEL_1

/* Private variables --------------------------------------------------------*/
static uint16_t rgbBuffer[RESET_PULSE + WS2812B_MAX_LED_NUM * LED_DATA_LEN] 
    __attribute__((section(".ARM.__at_0x24000000")));

/* Exported functions -------------------------------------------------------*/

/**
 * @brief 初始化WS2812B
 */
void BSP_WS2812B_Init(void)
{
    /* 清空缓冲区 */
    for (uint16_t i = 0; i < sizeof(rgbBuffer) / sizeof(rgbBuffer[0]); i++)
    {
        rgbBuffer[i] = 0;
    }
}

/**
 * @brief 设置单个LED颜色
 * @param index: LED索引（0-11）
 * @param r: 红色（0-255）
 * @param g: 绿色（0-255）
 * @param b: 蓝色（0-255）
 */
void BSP_WS2812B_SetPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if (index >= WS2812B_MAX_LED_NUM) return;
    
    uint16_t *p = rgbBuffer + RESET_PULSE + (index * LED_DATA_LEN);
    
    for (uint16_t i = 0; i < 8; i++)
    {
        p[i]      = ((g << i) & 0x80) ? ONE_PULSE : ZERO_PULSE;
        p[i + 8]  = ((r << i) & 0x80) ? ONE_PULSE : ZERO_PULSE;
        p[i + 16] = ((b << i) & 0x80) ? ONE_PULSE : ZERO_PULSE;
    }
}

/**
 * @brief 更新LED显示（发送数据）
 */
void BSP_WS2812B_Update(void)
{
    uint16_t dataLen = RESET_PULSE + WS2812B_MAX_LED_NUM * LED_DATA_LEN;
    HAL_TIM_PWM_Start_DMA(&HTIM, CHANNEL, (uint32_t *)rgbBuffer, dataLen);
}

/**
 * @brief 清空所有LED
 */
void BSP_WS2812B_Clear(void)
{
    for (uint16_t i = 0; i < WS2812B_MAX_LED_NUM; i++)
    {
        BSP_WS2812B_SetPixel(i, 0, 0, 0);
    }
}

/**
 * @brief 设置全局亮度（简化版本，直接调用SetPixel）
 * @param brightness: 亮度值（0-255）
 */
void WS2812B_SetBrightness(uint8_t brightness)
{
    /* 此函数为兼容旧接口，实际亮度由APP层控制 */
    (void)brightness;
}

/**
 * @brief DMA完成回调
 */
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1)
    {
        HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
    }
}
