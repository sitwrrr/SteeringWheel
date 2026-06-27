/**
 * @file bsp_input.c
 * @brief 输入设备驱动实现（按键+拨片）
 * @version 1.0.0
 * @date 2025-01-01
 *
 * 扫描逻辑（非阻塞）：
 *   1. 读取GPIO电平（低电平=按下）
 *   2. 消抖：连续两次读到相同状态才确认变化
 *   3. 按下时记录时间戳，检测双击
 *   4. 持续按住时检查是否超过长按阈值
 *   5. 设置事件标志，由APP层读取处理
 */

#include "bsp_input.h"
#include "main.h"

/* Private defines ----------------------------------------------------------*/
#define DEBOUNCE_TIME       20      /* 消抖时间(ms) */
#define LONG_PRESS_TIME     1000    /* 长按阈值(ms) */
#define DOUBLE_CLICK_TIME   300     /* 双击间隔阈值(ms) */

/* Private variables ---------------------------------------------------------*/
static InputState_t inputState[KEY_COUNT + SHIFT_COUNT];   /* 各输入设备状态 */
static InputEvent_t inputEvent[KEY_COUNT + SHIFT_COUNT];   /* 各输入设备事件 */

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 读取单个输入设备的GPIO电平
 * @param id 输入设备ID
 * @return 0=释放（高电平），1=按下（低电平）
 *
 * 引脚映射：
 *   KEY1=PB0, KEY2=PB1, KEY3=PB2, KEY4=PC3
 *   KEY5=PC4, KEY6=PC1, KEY7=PC2, KEY8=PB14
 *   SHIFT_UP=PB15, SHIFT_DOWN=PC0
 */
static uint8_t BSP_Input_ReadGPIO(KeyId_t id)
{
    switch (id)
    {
        case KEY_1:     return HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET ? 1 : 0;
        case KEY_2:     return HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET ? 1 : 0;
        case KEY_3:     return HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin) == GPIO_PIN_RESET ? 1 : 0;
        case KEY_4:     return HAL_GPIO_ReadPin(KEY4_GPIO_Port, KEY4_Pin) == GPIO_PIN_RESET ? 1 : 0;
        case KEY_5:     return HAL_GPIO_ReadPin(KEY5_GPIO_Port, KEY5_Pin) == GPIO_PIN_RESET ? 1 : 0;
        case KEY_6:     return HAL_GPIO_ReadPin(KEY6_GPIO_Port, KEY6_Pin) == GPIO_PIN_RESET ? 1 : 0;
        case KEY_7:     return HAL_GPIO_ReadPin(KEY7_GPIO_Port, KEY7_Pin) == GPIO_PIN_RESET ? 1 : 0;
        case KEY_8:     return HAL_GPIO_ReadPin(KEY8_GPIO_Port, KEY8_Pin) == GPIO_PIN_RESET ? 1 : 0;
        case SHIFT_UP:  return HAL_GPIO_ReadPin(SHIFT_UP_GPIO_Port, SHIFT_UP_Pin) == GPIO_PIN_RESET ? 1 : 0;
        case SHIFT_DOWN:return HAL_GPIO_ReadPin(SHIFT_DOWN_GPIO_Port, SHIFT_DOWN_Pin) == GPIO_PIN_RESET ? 1 : 0;
    }
    return 0;
}

/* Exported functions --------------------------------------------------------*/

/**
 * @brief 初始化输入设备
 */
void BSP_Input_Init(void)
{
    for (uint8_t i = 0; i < KEY_COUNT + SHIFT_COUNT; i++)
    {
        inputState[i].state = 0;
        inputState[i].lastState = 0;
        inputState[i].pressTime = 0;
        inputState[i].longPressTriggered = 0;
        inputState[i].clickCount = 0;
        inputState[i].lastClickTime = 0;
        inputEvent[i] = INPUT_EVENT_NONE;
    }
}

/**
 * @brief 扫描所有输入设备（非阻塞，建议20ms调用一次）
 *
 * 处理流程：
 *   1. 读GPIO → 与上次比较，不同则更新lastState并跳过（消抖）
 *   2. 连续两次相同 → 确认状态变化
 *   3. 按下：记录时间戳，检查双击（300ms内连按两次）
 *   4. 释放：设置RELEASE事件
 *   5. 持续按住：检查是否超过1000ms长按阈值
 */
void BSP_Input_Scan(void)
{
    uint32_t now = HAL_GetTick();

    for (uint8_t i = 0; i < KEY_COUNT + SHIFT_COUNT; i++)
    {
        uint8_t currentState = BSP_Input_ReadGPIO((KeyId_t)i);
        InputState_t *s = &inputState[i];

        /* 消抖：本次读数与上次不同，更新lastState但不触发事件 */
        if (currentState != s->lastState)
        {
            s->lastState = currentState;
            continue;  /* 跳过，等下次扫描确认 */
        }

        /* 状态变化检测：确认后的读数与已记录状态不同 */
        if (currentState != s->state)
        {
            s->state = currentState;

            if (currentState) /* 0→1：按下 */
            {
                s->pressTime = now;
                s->longPressTriggered = 0;
                inputEvent[i] = INPUT_EVENT_PRESS;

                /* 双击检测：300ms内再次按下 */
                if ((now - s->lastClickTime) < DOUBLE_CLICK_TIME)
                {
                    s->clickCount++;
                    if (s->clickCount >= 2)
                    {
                        inputEvent[i] = INPUT_EVENT_DOUBLE_PRESS;
                        s->clickCount = 0;
                    }
                }
                else
                {
                    s->clickCount = 1;
                }
                s->lastClickTime = now;
            }
            else /* 1→0：释放 */
            {
                inputEvent[i] = INPUT_EVENT_RELEASE;
            }
        }

        /* 长按检测：持续按下且未触发过，检查是否超过阈值 */
        if (s->state && !s->longPressTriggered)
        {
            if ((now - s->pressTime) >= LONG_PRESS_TIME)
            {
                s->longPressTriggered = 1;
                inputEvent[i] = INPUT_EVENT_LONG_PRESS;
            }
        }
    }
}

/**
 * @brief 查询按键是否按下
 */
uint8_t BSP_Input_IsPressed(KeyId_t id)
{
    if (id >= KEY_COUNT + SHIFT_COUNT) return 0;
    return inputState[id].state;
}

/**
 * @brief 查询按键是否处于长按状态
 */
uint8_t BSP_Input_IsLongPressed(KeyId_t id)
{
    if (id >= KEY_COUNT + SHIFT_COUNT) return 0;
    return inputState[id].longPressTriggered;
}

/**
 * @brief 获取按键事件
 */
InputEvent_t BSP_Input_GetEvent(KeyId_t id)
{
    if (id >= KEY_COUNT + SHIFT_COUNT) return INPUT_EVENT_NONE;
    return inputEvent[id];
}

/**
 * @brief 清除按键事件
 */
void BSP_Input_ClearEvent(KeyId_t id)
{
    if (id >= KEY_COUNT + SHIFT_COUNT) return;
    inputEvent[id] = INPUT_EVENT_NONE;
}

/**
 * @brief 按键事件回调（弱定义，可重写）
 */
__weak void BSP_Input_EventCallback(KeyId_t id, InputEvent_t event)
{
    (void)id;
    (void)event;
}
