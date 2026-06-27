/**
 * @file usbd_composite.c
 * @brief USB CDC+HID复合设备实现
 * @version 1.0.0
 * @date 2025-01-01
 *
 * 实现思路：
 * - 配置描述符 = CDC描述符 + HID描述符（手动拼接）
 * - Setup/DataIn/DataOut按接口号路由到CDC或HID处理
 * - CDC用EP1 IN/OUT，HID用EP2 IN
 */

#include "usbd_composite.h"
#include "usbd_ctlreq.h"
#include "usbd_cdc.h"

/* HID状态 */
typedef enum { HID_IDLE = 0, HID_BUSY } HID_StateTypeDef;

typedef struct {
    uint32_t Protocol;
    uint32_t IdleState;
    uint32_t AltSetting;
    HID_StateTypeDef state;
} USBD_HID_HandleTypeDef;

/* 前向声明 */
static uint8_t USBD_COMPOSITE_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_COMPOSITE_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_COMPOSITE_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t USBD_COMPOSITE_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_COMPOSITE_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t *USBD_COMPOSITE_GetFSCfgDesc(uint16_t *length);
static uint8_t *USBD_COMPOSITE_GetDeviceQualifierDesc(uint16_t *length);

/* HID实例（静态分配） */
static USBD_HID_HandleTypeDef hhid;

/* HID Report描述符：16按键手柄 */
__ALIGN_BEGIN static uint8_t HID_ReportDesc[COMPOSITE_HID_REPORT_DESC_SIZE] __ALIGN_END = {
    0x05, 0x01,        /* USAGE_PAGE (Generic Desktop) */
    0x15, 0x00,        /* LOGICAL_MINIMUM (0) */
    0x09, 0x04,        /* USAGE (Joystick) */
    0xa1, 0x01,        /* COLLECTION (Application) */
    0x05, 0x09,        /*   USAGE_PAGE (Button) */
    0x19, 0x01,        /*   USAGE_MINIMUM (Button 1) */
    0x29, 0x0A,        /*   USAGE_MAXIMUM (Button 10) */
    0x15, 0x00,        /*   LOGICAL_MINIMUM (0) */
    0x25, 0x01,        /*   LOGICAL_MAXIMUM (1) */
    0x75, 0x01,        /*   REPORT_SIZE (1) */
    0x95, 0x10,        /*   REPORT_COUNT (16) */
    0x55, 0x00,        /*   UNIT_EXPONENT (0) */
    0x65, 0x00,        /*   UNIT (None) */
    0x81, 0x02,        /*   INPUT (Data,Var,Abs) */
    0xc0               /* END_COLLECTION */
};

/* ===== 复合配置描述符（CDC + HID） ===== */
__ALIGN_BEGIN static uint8_t USBD_COMPOSITE_CfgDesc[COMPOSITE_CFG_DESC_LEN] __ALIGN_END = {

    /* ---- CDC部分（67字节，来自usbd_cdc.c标准描述符） ---- */
    /* 配置描述符 */
    0x09,                       /* bLength */
    USB_DESC_TYPE_CONFIGURATION,/* bDescriptorType */
    COMPOSITE_CFG_DESC_LEN, 0x00, /* wTotalLength */
    0x03,                       /* bNumInterfaces: CDC(2) + HID(1) = 3 */
    0x01,                       /* bConfigurationValue */
    0x00,                       /* iConfiguration */
    0xC0,                       /* bmAttributes: 自供电 */
    0x32,                       /* MaxPower: 100mA */

    /* CDC接口0：通信接口 */
    0x09, USB_DESC_TYPE_INTERFACE, COMPOSITE_CDC_COMM_ITF, 0x00, 0x01,
    0x02, 0x02, 0x01, 0x00,     /* CDC ACM, 1端点 */

    /* CDC功能描述符 */
    0x05, 0x24, 0x00, 0x10, 0x01, /* Header */
    0x05, 0x24, 0x01, 0x00, 0x01, /* Call Management */
    0x04, 0x24, 0x02, 0x02,       /* ACM */
    0x05, 0x24, 0x06, 0x00, 0x01, /* Union */

    /* CDC通知端点 */
    0x07, USB_DESC_TYPE_ENDPOINT, 0x81, 0x03, 0x08, 0x00, 0x10,

    /* CDC接口1：数据接口 */
    0x09, USB_DESC_TYPE_INTERFACE, COMPOSITE_CDC_DATA_ITF, 0x00, 0x02,
    0x0A, 0x00, 0x00, 0x00,     /* CDC Data, 2端点 */

    /* CDC数据端点 OUT */
    0x07, USB_DESC_TYPE_ENDPOINT, 0x02, 0x02, 0x40, 0x00, 0x00,
    /* CDC数据端点 IN */
    0x07, USB_DESC_TYPE_ENDPOINT, 0x83, 0x02, 0x40, 0x00, 0x00,

    /* ---- HID部分（34字节） ---- */
    /* HID接口 */
    0x09, USB_DESC_TYPE_INTERFACE, COMPOSITE_HID_ITF, 0x00, 0x01,
    0x03, 0x00, 0x00, 0x00,     /* HID, 1端点 */

    /* HID描述符 */
    0x09, 0x21, 0x11, 0x01, 0x00, 0x01, 0x22,
    COMPOSITE_HID_REPORT_DESC_SIZE, 0x00,

    /* HID端点 IN */
    0x07, USB_DESC_TYPE_ENDPOINT, COMPOSITE_HID_EPIN, 0x03,
    COMPOSITE_HID_EPIN_SIZE, 0x00, 0x0A,   /* 10ms轮询 */
};

/* USB设备限定符描述符 */
__ALIGN_BEGIN static uint8_t USBD_COMPOSITE_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END = {
    USB_LEN_DEV_QUALIFIER_DESC, USB_DESC_TYPE_DEVICE_QUALIFIER,
    0x00, 0x02, 0x00, 0x00, 0x00, 0x40, 0x01, 0x00,
};

/* 复合设备类定义 */
USBD_ClassTypeDef USBD_COMPOSITE = {
    USBD_COMPOSITE_Init,
    USBD_COMPOSITE_DeInit,
    USBD_COMPOSITE_Setup,
    NULL,                       /* EP0_TxSent */
    NULL,                       /* EP0_RxReady */
    USBD_COMPOSITE_DataIn,
    USBD_COMPOSITE_DataOut,
    NULL,                       /* SOF */
    NULL, NULL,
    USBD_COMPOSITE_GetFSCfgDesc,
    USBD_COMPOSITE_GetFSCfgDesc,
    USBD_COMPOSITE_GetFSCfgDesc,
    USBD_COMPOSITE_GetDeviceQualifierDesc,
};

/* ===== 初始化 ===== */
static uint8_t USBD_COMPOSITE_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    (void)cfgidx;
    /* CDC初始化：分配CDC句柄内存 */
    USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef *)USBD_malloc(sizeof(USBD_CDC_HandleTypeDef));
    if (hcdc == NULL) return USBD_EMEM;
    USBD_memset(hcdc, 0, sizeof(USBD_CDC_HandleTypeDef));
    pdev->pClassData = hcdc;

    /* CDC端点：EP1 IN(0x81)通知 + EP2 OUT(0x02)数据接收 + EP3 IN(0x83)数据发送 */
    pdev->ep_in[0x81 & 0xFU].bInterval = 0;
    USBD_LL_OpenEP(pdev, 0x81, USBD_EP_TYPE_INTR, 0x08);
    pdev->ep_in[0x81 & 0xFU].is_used = 1U;
    USBD_LL_OpenEP(pdev, 0x02, USBD_EP_TYPE_BULK, 0x40);
    pdev->ep_out[0x02 & 0xFU].is_used = 1U;
    USBD_LL_OpenEP(pdev, 0x83, USBD_EP_TYPE_BULK, 0x40);
    pdev->ep_in[0x83 & 0xFU].is_used = 1U;
    hcdc->TxState = 0;
    hcdc->RxState = 0;
    USBD_LL_PrepareReceive(pdev, 0x02, hcdc->RxBuffer, 0x40);

    /* HID初始化：EP2 IN(0x82) */
    hhid.state = HID_IDLE;
    USBD_LL_OpenEP(pdev, COMPOSITE_HID_EPIN, USBD_EP_TYPE_INTR, COMPOSITE_HID_EPIN_SIZE);
    pdev->ep_in[COMPOSITE_HID_EPIN & 0xFU].is_used = 1U;
    pdev->ep_in[COMPOSITE_HID_EPIN & 0xFU].bInterval = 0x0A;
    pdev->pClassData_HID_Mouse = &hhid;

    return USBD_OK;
}

/* ===== 反初始化 ===== */
static uint8_t USBD_COMPOSITE_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    (void)cfgidx;
    /* 关闭CDC端点 */
    USBD_LL_CloseEP(pdev, 0x81);
    pdev->ep_in[0x81 & 0xFU].is_used = 0U;
    USBD_LL_CloseEP(pdev, 0x02);
    pdev->ep_out[0x02 & 0xFU].is_used = 0U;
    USBD_LL_CloseEP(pdev, 0x83);
    pdev->ep_in[0x83 & 0xFU].is_used = 0U;

    /* 关闭HID端点 */
    USBD_LL_CloseEP(pdev, COMPOSITE_HID_EPIN);
    pdev->ep_in[COMPOSITE_HID_EPIN & 0xFU].is_used = 0U;

    if (pdev->pClassData != NULL) {
        USBD_free(pdev->pClassData);
        pdev->pClassData = NULL;
    }
    pdev->pClassData_HID_Mouse = NULL;

    return USBD_OK;
}

/* ===== 请求处理（按接口号路由） ===== */
static uint8_t USBD_COMPOSITE_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    uint16_t len = 0;
    uint8_t *pbuf = NULL;
    uint8_t itf = LOBYTE(req->wIndex);

    /* HID接口的请求 */
    if (itf == COMPOSITE_HID_ITF) {
        USBD_HID_HandleTypeDef *hh = (USBD_HID_HandleTypeDef *)pdev->pClassData_HID_Mouse;
        switch (req->bmRequest & USB_REQ_TYPE_MASK) {
        case USB_REQ_TYPE_CLASS:
            switch (req->bRequest) {
            case 0x0A: /* SET_IDLE */
                hh->IdleState = (uint8_t)(req->wValue >> 8);
                break;
            case 0x01: /* GET_REPORT */
                /* 不处理，返回空 */
                break;
            default:
                USBD_CtlError(pdev, req);
                return USBD_FAIL;
            }
            break;
        case USB_REQ_TYPE_STANDARD:
            switch (req->bRequest) {
            case USB_REQ_GET_DESCRIPTOR:
                if ((req->wValue >> 8) == 0x22) { /* HID Report Descriptor */
                    pbuf = HID_ReportDesc;
                    len = MIN(COMPOSITE_HID_REPORT_DESC_SIZE, req->wLength);
                }
                USBD_CtlSendData(pdev, pbuf, len);
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
        return USBD_OK;
    }

    /* CDC接口的请求：转发给CDC类处理 */
    if (pdev->pClassData != NULL) {
        /* 调用CDC的Setup */
        extern USBD_ClassTypeDef USBD_CDC;
        return USBD_CDC.Setup(pdev, req);
    }

    return USBD_OK;
}

/* ===== 数据IN完成 ===== */
static uint8_t USBD_COMPOSITE_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    /* EP1 IN (0x81) + EP3 IN (0x83): CDC */
    if ((epnum & 0x0F) == (0x81 & 0x0F) || (epnum & 0x0F) == (0x83 & 0x0F)) {
        if (pdev->pClassData != NULL) {
            extern USBD_ClassTypeDef USBD_CDC;
            USBD_CDC.DataIn(pdev, epnum);
        }
    }

    /* EP2 IN (0x82): HID */
    if ((epnum & 0x0F) == (COMPOSITE_HID_EPIN & 0x0F)) {
        USBD_HID_HandleTypeDef *hh = (USBD_HID_HandleTypeDef *)pdev->pClassData_HID_Mouse;
        if (hh != NULL) {
            hh->state = HID_IDLE;
        }
    }

    return USBD_OK;
}

/* ===== 数据OUT ===== */
static uint8_t USBD_COMPOSITE_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    /* EP2 OUT (0x02): CDC数据接收 */
    if ((epnum & 0x0F) == 0x02) {
        USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef *)pdev->pClassData;
        if (hcdc != NULL) {
            hcdc->RxLength = USBD_LL_GetRxDataSize(pdev, epnum);
            extern USBD_ClassTypeDef USBD_CDC;
            USBD_CDC.DataOut(pdev, epnum);
        }
    }
    return USBD_OK;
}

/* ===== 描述符 ===== */
static uint8_t *USBD_COMPOSITE_GetFSCfgDesc(uint16_t *length)
{
    *length = (uint16_t)sizeof(USBD_COMPOSITE_CfgDesc);
    return USBD_COMPOSITE_CfgDesc;
}

static uint8_t *USBD_COMPOSITE_GetDeviceQualifierDesc(uint16_t *length)
{
    *length = (uint16_t)sizeof(USBD_COMPOSITE_DeviceQualifierDesc);
    return USBD_COMPOSITE_DeviceQualifierDesc;
}

/* ===== 发送HID报告 ===== */
uint8_t USBD_Composite_SendHIDReport(USBD_HandleTypeDef *pdev, uint8_t *report, uint16_t len)
{
    USBD_HID_HandleTypeDef *hh = (USBD_HID_HandleTypeDef *)pdev->pClassData_HID_Mouse;
    if (hh == NULL) return USBD_FAIL;
    if (pdev->dev_state != USBD_STATE_CONFIGURED) return USBD_FAIL;
    if (hh->state != HID_IDLE) return USBD_BUSY;

    hh->state = HID_BUSY;
    USBD_LL_Transmit(pdev, COMPOSITE_HID_EPIN, report, len);
    return USBD_OK;
}
