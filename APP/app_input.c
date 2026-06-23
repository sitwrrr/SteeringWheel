/**
 * @file app_input.c
 * @brief 输入处理应用实现（按键+拨片）
 * @version 1.0.0
 * @date 2025-01-01
 */

#include "app_input.h"
#include "bsp_input.h"
#include "app_can.h"
#include "Variable.h"

/* Private defines -----------------------------------------------------------*/

/* Private types -------------------------------------------------------------*/
typedef void (*InputHandler_t)(void);

/* Private function prototypes -----------------------------------------------*/
static void key1_handler(void);
static void key2_handler(void);
static void key3_handler(void);
static void key4_handler(void);
static void key5_handler(void);
static void key6_handler(void);
static void key7_handler(void);
static void key8_handler(void);

/* Private variables ---------------------------------------------------------*/
static const InputHandler_t keyHandlers[KEY_COUNT] = {
    key1_handler,
    key2_handler,
    key3_handler,
    key4_handler,
    key5_handler,
    key6_handler,
    key7_handler,
    key8_handler
};

/* Private functions ---------------------------------------------------------*/

/* KEY1: 显示模式切换 */
static void key1_handler(void)
{
    /* 切换显示模式 */
    g_vehicleData.key_clicked = 1;
}

/* KEY2: 切换显示页面 */
static void key2_handler(void)
{
    g_vehicleData.key_clicked = 2;
}

/* KEY3: 增加亮度 */
static void key3_handler(void)
{
    g_vehicleData.key_clicked = 3;
}

/* KEY4: 减少亮度 */
static void key4_handler(void)
{
    g_vehicleData.key_clicked = 4;
}

/* KEY5: 复位圈计时 */
static void key5_handler(void)
{
    g_vehicleData.key_clicked = 5;
}

/* KEY6: 标记圈 */
static void key6_handler(void)
{
    g_vehicleData.key_clicked = 6;
}

/* KEY7: 自定义功能1 */
static void key7_handler(void)
{
    g_vehicleData.key_clicked = 7;
}

/* KEY8: 自定义功能2 */
static void key8_handler(void)
{
    g_vehicleData.key_clicked = 8;
}

/* Exported functions --------------------------------------------------------*/

/**
 * @brief 初始化输入应用
 */
void APP_Input_Init(void)
{
    BSP_Input_Init();
}

/**
 * @brief 扫描输入（需要定期调用，建议20ms周期）
 */
void APP_Input_Scan(void)
{
    BSP_Input_Scan();
    
    /* 更新按键状态到结构体 */
    g_vehicleData.key_state = 0;
    for (uint8_t i = 0; i < KEY_COUNT; i++)
    {
        if (BSP_Input_IsPressed((KeyId_t)i))
        {
            g_vehicleData.key_state |= (1 << i);
        }
    }
    
    /* 检查按键事件 */
    for (uint8_t i = 0; i < KEY_COUNT; i++)
    {
        InputEvent_t event = BSP_Input_GetEvent((KeyId_t)i);
        
        if (event == INPUT_EVENT_PRESS)
        {
            /* 短按处理 */
            if (keyHandlers[i] != NULL)
            {
                keyHandlers[i]();
            }
            BSP_Input_ClearEvent((KeyId_t)i);
        }
        else if (event == INPUT_EVENT_LONG_PRESS)
        {
            /* 长按处理（可扩展） */
            BSP_Input_ClearEvent((KeyId_t)i);
        }
    }
    
    /* 更新换挡拨片状态 */
    g_vehicleData.shift_up = BSP_Input_IsPressed(SHIFT_UP);
    g_vehicleData.shift_down = BSP_Input_IsPressed(SHIFT_DOWN);
    
    /* 换挡处理 */
    if (g_vehicleData.shift_up || g_vehicleData.shift_down)
    {
        APP_Input_ShiftCallback(g_vehicleData.shift_up, g_vehicleData.shift_down);
    }
}

/**
 * @brief 处理输入事件
 * @param id: 按键ID
 * @param event: 事件类型
 */
void APP_Input_ProcessEvent(KeyId_t id, InputEvent_t event)
{
    if (id < KEY_COUNT)
    {
        if (event == INPUT_EVENT_PRESS && keyHandlers[id] != NULL)
        {
            keyHandlers[id]();
        }
    }
}

/**
 * @brief 发送按键状态到CAN
 */
void APP_Input_SendCAN(void)
{
    APP_CAN_SendKeyState(g_vehicleData.key_state);
}

/**
 * @brief 按键短按回调（弱定义，可重写）
 * @param id: 按键ID
 */
__weak void APP_Input_PressCallback(KeyId_t id)
{
    (void)id;
}

/**
 * @brief 按键长按回调（弱定义，可重写）
 * @param id: 按键ID
 */
__weak void APP_Input_LongPressCallback(KeyId_t id)
{
    (void)id;
}

/**
 * @brief 换挡回调（弱定义，可重写）
 * @param up: 升挡状态
 * @param down: 降挡状态
 */
__weak void APP_Input_ShiftCallback(uint8_t up, uint8_t down)
{
    (void)up;
    (void)down;
}
