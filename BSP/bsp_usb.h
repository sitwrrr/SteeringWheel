/**
 * @file bsp_usb.h
 * @brief USB复合设备驱动
 * @version 1.0.0
 * @date 2025-01-01
 */

#ifndef __BSP_USB_H
#define __BSP_USB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include "usb_device.h"
#include <stdio.h>
#include <stdarg.h>

/* Exported defines ----------------------------------------------------------*/
#define USB_TX_BUFFER_SIZE  256
#define USB_RX_BUFFER_SIZE  256

/* Exported functions --------------------------------------------------------*/
void BSP_USB_Init(void);
void BSP_USB_SendData(uint8_t *data, uint16_t len);
void BSP_USB_Printf(const char *format, ...);
uint8_t BSP_USB_IsConnected(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_USB_H */
