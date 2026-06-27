/**
 * @file bsp_ws2812b.c
 * @brief WS2812B RGB LED驱动实现
 * @version 1.0.0
 * @date 2025-01-01
 *
 * DMA缓冲区结构：
 *   [RESET_PULSE个0] [LED0 24bit] [LED1 24bit] ... [LED11 24bit]
 *   前230个uint16_t为0（低电平复位信号>50us）
 *   每个LED占24个uint16_t（8bit G + 8bit R + 8bit B）
 *
 * PWM时序（800kHz，Period=299，定时器时钟240MHz）：
 *   bit1: 高电平 ~830ns（要求580-1000ns）→ ONE_PULSE = (299+1)/3*2-1 = 199
 *   bit0: 高电平 ~413ns（要求220-380ns）→ ZERO_PULSE = (299+1)/3-1 = 99
 *   总周期: 1250ns（要求1250±60ns）
 */

#include "bsp_ws2812b.h"
#include "tim.h"

/* Private defines ----------------------------------------------------------*/
#define ONE_PULSE       ((htim1.Init.Period + 1) / 3 * 2 - 1)  /* bit1占空比 ~66% */
#define ZERO_PULSE      ((htim1.Init.Period + 1) / 3 - 1)      /* bit0占空比 ~33% */
#define RESET_PULSE     230     /* 复位信号长度（低电平>50us） */
#define LED_DATA_LEN    24      /* 每个LED数据长度（8bit×3色=24bit） */
#define HTIM            htim1   /* 使用的定时器句柄 */
#define CHANNEL         TIM_CHANNEL_1  /* PWM通道 */

/* Private variables --------------------------------------------------------*/
/*
 * DMA缓冲区：前RESET_PULSE个为复位信号（0），后面是LED数据
 * 每个元素是uint16_t，对应定时器的一个PWM周期
 * .bss默认放RAM(0x24000000=D1区)，DMA1/DMA2可直接访问
 */
static uint16_t rgbBuffer[RESET_PULSE + WS2812B_MAX_LED_NUM * LED_DATA_LEN];

/* Exported functions -------------------------------------------------------*/

/**
 * @brief 初始化WS2812B，清空DMA缓冲区
 */
void BSP_WS2812B_Init(void)
{
    /* 清空整个缓冲区（复位信号+LED数据全为0） */
    for (uint16_t i = 0; i < sizeof(rgbBuffer) / sizeof(rgbBuffer[0]); i++)
    {
        rgbBuffer[i] = 0;
    }
}

/**
 * @brief 设置单个LED颜色（写入DMA缓冲区）
 * @param index LED索引（0-11）
 * @param r     红色（0-255）
 * @param g     绿色（0-255）
 * @param b     蓝色（0-255）
 *
 * WS2812B数据格式：GRB顺序，MSB先发
 * 每个LED占24个PWM周期：bit[0-7]=G, bit[8-15]=R, bit[16-23]=B
 */
void BSP_WS2812B_SetPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if (index >= WS2812B_MAX_LED_NUM) return;

    /* 指向该LED在缓冲区中的起始位置（跳过复位信号区域） */
    uint16_t *p = rgbBuffer + RESET_PULSE + (index * LED_DATA_LEN);

    /* 逐位转换为PWM占空比，MSB先发（WS2812B协议要求） */
    for (uint16_t i = 0; i < 8; i++)
    {
        p[i]      = ((g << i) & 0x80) ? ONE_PULSE : ZERO_PULSE;  /* 绿色 bit7→bit0 */
        p[i + 8]  = ((r << i) & 0x80) ? ONE_PULSE : ZERO_PULSE;  /* 红色 bit7→bit0 */
        p[i + 16] = ((b << i) & 0x80) ? ONE_PULSE : ZERO_PULSE;  /* 蓝色 bit7→bit0 */
    }
}

/**
 * @brief 发送缓冲区数据到LED（触发DMA传输）
 * @note  DMA完成后自动触发HAL_TIM_PWM_PulseFinishedCallback回调停止PWM
 */
void BSP_WS2812B_Update(void)
{
    uint16_t dataLen = RESET_PULSE + WS2812B_MAX_LED_NUM * LED_DATA_LEN;
    HAL_TIM_PWM_Start_DMA(&HTIM, CHANNEL, (uint32_t *)rgbBuffer, dataLen);
}

/**
 * @brief 清空所有LED颜色（设为黑色）
 * @note  只修改缓冲区，需要调用BSP_WS2812B_Update()发送到LED才会生效
 */
void BSP_WS2812B_Clear(void)
{
    for (uint16_t i = 0; i < WS2812B_MAX_LED_NUM; i++)
    {
        BSP_WS2812B_SetPixel(i, 0, 0, 0);
    }
}

/**
 * @brief 设置全局亮度（兼容旧接口）
 * @note  当前为空函数，亮度由APP层在SetPixel前通过乘法缩放控制
 */
void WS2812B_SetBrightness(uint8_t brightness)
{
    (void)brightness;
}

/**
 * @brief DMA传输完成回调（ISR中调用）
 * @note  先将占空比清零确保PWM输出低电平，再停止DMA
 *        与MeterBox项目处理方式一致
 */
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1)
    {
        __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_1, 0);  /* 占空比清零，确保输出低电平 */
        HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);     /* 停止DMA和PWM */
    }
}
