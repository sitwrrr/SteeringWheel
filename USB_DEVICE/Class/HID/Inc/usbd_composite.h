/**
 * @file usbd_composite.h
 * @brief USB CDC+HID复合设备
 * @version 1.0.0
 * @date 2025-01-01
 *
 * CDC: Interface 0（通信）+ Interface 1（数据），EP1 IN/OUT
 * HID: Interface 2（16键手柄），EP2 IN
 */

#ifndef __USBD_COMPOSITE_H
#define __USBD_COMPOSITE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usbd_ioreq.h"

#define COMPOSITE_CDC_COMM_ITF          0x00
#define COMPOSITE_CDC_DATA_ITF          0x01
#define COMPOSITE_HID_ITF               0x02
#define COMPOSITE_HID_EPIN              0x82
#define COMPOSITE_HID_EPIN_SIZE         0x04
#define COMPOSITE_HID_REPORT_DESC_SIZE  29
#define COMPOSITE_CDC_DESC_LEN          67      /* 配置描述符(9)+CDC通信接口(9)+功能描述符(19)+通知端点(7)+数据接口(9)+数据端点(14) */
#define COMPOSITE_HID_DESC_LEN          25      /* HID接口(9)+HID描述符(9)+HID端点(7) */
#define COMPOSITE_CFG_DESC_LEN          (COMPOSITE_CDC_DESC_LEN + COMPOSITE_HID_DESC_LEN)

extern USBD_ClassTypeDef USBD_COMPOSITE;

uint8_t USBD_Composite_SendHIDReport(USBD_HandleTypeDef *pdev, uint8_t *report, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* __USBD_COMPOSITE_H */
