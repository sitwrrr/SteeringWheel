/**
 * @file bsp_usb.c
 * @brief USB复合设备驱动实现
 * @version 1.0.0
 * @date 2025-01-01
 */

#include "bsp_usb.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"
#include "usbd_composite.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* Private defines ----------------------------------------------------------*/
#define USB_TX_BUFFER_SIZE  256

/* Private variables --------------------------------------------------------*/
extern USBD_HandleTypeDef hUsbDevice;

static uint8_t txBuffer[USB_TX_BUFFER_SIZE];
static uint8_t connected = 0;

/* Private types ------------------------------------------------------------*/
typedef struct {
    uint8_t button_group0;
    uint8_t button_group1;
} joyStick_HID_t;

static joyStick_HID_t joyStick_HID;

/* Exported functions -------------------------------------------------------*/

/**
 * @brief 初始化USB
 */
void BSP_USB_Init(void)
{
    connected = 0;
    memset(&joyStick_HID, 0, sizeof(joyStick_HID_t));
}

/**
 * @brief USB发送数据
 * @param data: 数据指针
 * @param len: 数据长度
 */
void BSP_USB_SendData(uint8_t *data, uint16_t len)
{
    CDC_Transmit_FS(data, len);
}

/**
 * @brief USB printf
 * @param format: 格式化字符串
 */
void BSP_USB_Printf(const char *format, ...)
{
    va_list args;
    uint32_t length;
    
    va_start(args, format);
    length = vsnprintf((char *)txBuffer, USB_TX_BUFFER_SIZE, format, args);
    va_end(args);
    
    CDC_Transmit_FS(txBuffer, length);
}

/**
 * @brief 检查USB是否连接
 * @return 1=已连接, 0=未连接
 */
uint8_t BSP_USB_IsConnected(void)
{
    return connected;
}

/**
 * @brief USB HID发送按键报告
 * @param buttons: 按键位掩码（bit0-bit9对应10个按键）
 */
void BSP_USB_SendHIDReport(uint8_t buttons)
{
    joyStick_HID.button_group0 = buttons & 0xFF;
    joyStick_HID.button_group1 = (buttons >> 8) & 0xFF;

    extern USBD_HandleTypeDef hUsbDeviceFS;
    USBD_Composite_SendHIDReport(&hUsbDeviceFS, (uint8_t *)&joyStick_HID, sizeof(joyStick_HID_t));
}

/**
 * @brief 设置USB连接状态
 * @param state: 1=已连接, 0=断开
 */
void BSP_USB_SetConnected(uint8_t state)
{
    connected = state;
}
