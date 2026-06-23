/**
 * @file bsp_input.c
 * @brief 输入设备驱动实现（按键+拨片）
 * @version 1.0.0
 * @date 2025-01-01
 */

#include "bsp_input.h"
#include "main.h"

/* Private defines -----------------------------------------------------------*/
#define DEBOUNCE_TIME       20      /* 消抖时间(ms) */
#define LONG_PRESS_TIME     1000    /* 长按时间(ms) */
#define DOUBLE_CLICK_TIME   300     /* 双击间隔(ms) */

/* Private variables ---------------------------------------------------------*/
static InputState_t inputState[KEY_COUNT + SHIFT_COUNT];
static InputEvent_t inputEvent[KEY_COUNT + SHIFT_COUNT];

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 读取按键GPIO状态
 * @param id: 按键ID
 * @return 0=释放, 1=按下
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
        default:        return 0;
    }
}

/* Exported functions --------------------------------------------------------*/

/**
 * @brief 初始化输入设备
 */
void BSP_Input_Init(void)
{
    /* 清零所有状态 */
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
 * @brief 扫描输入设备（需要定期调用，建议20ms周期）
 */
void BSP_Input_Scan(void)
{
    uint32_t now = HAL_GetTick();
    
    for (uint8_t i = 0; i < KEY_COUNT + SHIFT_COUNT; i++)
    {
        uint8_t currentState = BSP_Input_ReadGPIO((KeyId_t)i);
        InputState_t *s = &inputState[i];
        
        /* 消抖处理 */
        if (currentState != s->lastState)
        {
            s->lastState = currentState;
            continue;
        }
        
        /* 状态变化检测 */
        if (currentState != s->state)
        {
            s->state = currentState;
            
            if (currentState) /* 按下 */
            {
                s->pressTime = now;
                s->longPressTriggered = 0;
                inputEvent[i] = INPUT_EVENT_PRESS;
                
                /* 双击检测 */
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
            else /* 释放 */
            {
                inputEvent[i] = INPUT_EVENT_RELEASE;
            }
        }
        
        /* 长按检测 */
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
 * @brief 判断按键是否按下
 * @param id: 按键ID
 * @return 0=释放, 1=按下
 */
uint8_t BSP_Input_IsPressed(KeyId_t id)
{
    if (id >= KEY_COUNT + SHIFT_COUNT) return 0;
    return inputState[id].state;
}

/**
 * @brief 判断按键是否长按
 * @param id: 按键ID
 * @return 0=未长按, 1=已长按
 */
uint8_t BSP_Input_IsLongPressed(KeyId_t id)
{
    if (id >= KEY_COUNT + SHIFT_COUNT) return 0;
    return inputState[id].longPressTriggered;
}

/**
 * @brief 获取按键事件
 * @param id: 按键ID
 * @return 事件类型
 */
InputEvent_t BSP_Input_GetEvent(KeyId_t id)
{
    if (id >= KEY_COUNT + SHIFT_COUNT) return INPUT_EVENT_NONE;
    return inputEvent[id];
}

/**
 * @brief 清除按键事件
 * @param id: 按键ID
 */
void BSP_Input_ClearEvent(KeyId_t id)
{
    if (id >= KEY_COUNT + SHIFT_COUNT) return;
    inputEvent[id] = INPUT_EVENT_NONE;
}

/**
 * @brief 按键事件回调（弱定义，可重写）
 * @param id: 按键ID
 * @param event: 事件类型
 */
__weak void BSP_Input_EventCallback(KeyId_t id, InputEvent_t event)
{
    (void)id;
    (void)event;
}
