/**
 * @file app_input.c
 * @brief 输入处理应用实现（按键+拨片）
 * @version 1.0.0
 * @date 2025-01-01
 *
 * 按键功能分配（可按需修改keyHandlers中的处理函数）：
 *   KEY1: 显示模式切换    KEY5: 圈计时重置
 *   KEY2: 页面切换        KEY6: 标记圈
 *   KEY3: 亮度+           KEY7: 自定义1
 *   KEY4: 亮度-           KEY8: 自定义2
 *
 * 事件处理链路：
 *   短按 → keyHandlers[i]() → APP_Input_PressCallback(id)
 *   长按 → APP_Input_LongPressCallback(id)
 *   换挡 → APP_Input_ShiftCallback(up, down)
 */

#include "app_input.h"
#include "bsp_input.h"
#include "app_can.h"
#include "Variable.h"

/* Private types -------------------------------------------------------------*/
typedef void (*InputHandler_t)(void);

/* Private function prototypes -----------------------------------------------*/
static void key1_handler(void);  /* 显示模式切换 */
static void key2_handler(void);  /* 页面切换 */
static void key3_handler(void);  /* 亮度+ */
static void key4_handler(void);  /* 亮度- */
static void key5_handler(void);  /* 圈计时重置 */
static void key6_handler(void);  /* 标记圈 */
static void key7_handler(void);  /* 自定义1 */
static void key8_handler(void);  /* 自定义2 */

/**
 * 按键处理函数指针表
 * 索引对应KeyId_t枚举值（KEY_1=0, KEY_2=1, ...）
 * 短按时自动调用对应函数
 */
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

/* Private variables ---------------------------------------------------------*/
static uint8_t lastShiftUp = 0;   /* SW-M5修复: 上次拨片状态，用于边沿检测 */
static uint8_t lastShiftDown = 0;

/* Private functions ---------------------------------------------------------*/

/* KEY1: 显示模式切换（通过key_clicked通知LVGL任务） */
static void key1_handler(void)
{
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
 * @brief 扫描并处理输入事件（建议20ms周期调用）
 *
 * 处理流程：
 *   1. BSP_Input_Scan() — 扫描硬件，检测消抖/长按/双击
 *   2. 更新key_state位掩码 — 8个按键各占1位，用于CAN上报
 *   3. 遍历按键事件：
 *      短按 → keyHandlers函数指针 + APP_Input_PressCallback
 *      长按 → APP_Input_LongPressCallback
 *   4. 更新拨片状态 → APP_Input_ShiftCallback
 */
void APP_Input_Scan(void)
{
    /* 1. 扫描硬件 */
    BSP_Input_Scan();

    /* 2. 更新按键状态位掩码（bit0=KEY1, bit1=KEY2, ...） */
    g_vehicleData.key_state = 0;
    for (uint8_t i = 0; i < KEY_COUNT; i++)
    {
        if (BSP_Input_IsPressed((KeyId_t)i))
        {
            g_vehicleData.key_state |= (1 << i);
        }
    }

    /* 3. 处理按键事件 */
    for (uint8_t i = 0; i < KEY_COUNT; i++)
    {
        InputEvent_t event = BSP_Input_GetEvent((KeyId_t)i);

        if (event == INPUT_EVENT_PRESS)
        {
            /* 短按：调用对应按键的处理函数 + 回调 */
            if (keyHandlers[i] != NULL)
            {
                keyHandlers[i]();
            }
            APP_Input_PressCallback((KeyId_t)i);
            BSP_Input_ClearEvent((KeyId_t)i);
        }
        else if (event == INPUT_EVENT_LONG_PRESS)
        {
            /* 长按：调用长按回调 */
            APP_Input_LongPressCallback((KeyId_t)i);
            BSP_Input_ClearEvent((KeyId_t)i);
        }
    }

    /* 4. 更新换挡拨片状态（上升沿检测，避免持续触发） */
    uint8_t curShiftUp = BSP_Input_IsPressed(SHIFT_UP);
    uint8_t curShiftDown = BSP_Input_IsPressed(SHIFT_DOWN);
    g_vehicleData.shift_up = curShiftUp;
    g_vehicleData.shift_down = curShiftDown;

    /* 只在0→1跳变时触发换挡回调（按住不重复触发） */
    uint8_t risingUp = curShiftUp && !lastShiftUp;
    uint8_t risingDown = curShiftDown && !lastShiftDown;
    lastShiftUp = curShiftUp;
    lastShiftDown = curShiftDown;

    if (risingUp || risingDown)
    {
        APP_Input_ShiftCallback(risingUp, risingDown);
    }
}

/**
 * @brief 处理单个输入事件（外部API，可由其他模块调用）
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
 * @brief 发送按键状态CAN报文（ID 0x155）
 */
void APP_Input_SendCAN(void)
{
    APP_CAN_SendKeyState(g_vehicleData.key_state);
}

/**
 * @brief 按键短按回调（弱定义，外部重写实现自定义逻辑）
 * @note  在keyHandlers之后调用，此时key_clicked已更新
 */
__weak void APP_Input_PressCallback(KeyId_t id)
{
    (void)id;
}

/**
 * @brief 按键长按回调（弱定义，外部重写实现自定义逻辑）
 */
__weak void APP_Input_LongPressCallback(KeyId_t id)
{
    (void)id;
}

/**
 * @brief 换挡回调（弱定义，外部重写实现自定义逻辑）
 * @note  只在拨片按下时调用，释放后不再调用
 */
__weak void APP_Input_ShiftCallback(uint8_t up, uint8_t down)
{
    (void)up;
    (void)down;
}
