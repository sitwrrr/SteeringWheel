/**
 * @file bsp_can.c
 * @brief CAN总线驱动实现（中断回调方式）
 * @version 1.1.0
 * @date 2025-01-01
 */

#include "bsp_can.h"
#include "fdcan.h"
#include <string.h>

/* Private variables --------------------------------------------------------*/
static FDCAN_TxHeaderTypeDef txHeader1;
static FDCAN_TxHeaderTypeDef txHeader2;

/* Exported functions -------------------------------------------------------*/

/**
 * @brief 初始化CAN总线
 *        配置FDCAN1/FDCAN2过滤器、全局过滤、启动、使能中断
 */
void BSP_CAN_Init(void)
{
    FDCAN_FilterTypeDef filterConfig;
    
    /* FDCAN1过滤器配置 */
    filterConfig.IdType = FDCAN_EXTENDED_ID;        /* 扩展ID模式（29位） */
    filterConfig.FilterIndex = 1;                    /* 过滤器索引 */
    filterConfig.FilterType = FDCAN_FILTER_MASK;     /* 掩码模式 */
    filterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0; /* 匹配的报文存入RXFIFO0 */
    filterConfig.FilterID1 = 0;                      /* 过滤器ID1（接受所有） */
    filterConfig.FilterID2 = 0;                      /* 掩码（0=不比较任何位） */
    HAL_FDCAN_ConfigFilter(&hfdcan1, &filterConfig);
    
    /* FDCAN2过滤器配置（与FDCAN1相同） */
    HAL_FDCAN_ConfigFilter(&hfdcan2, &filterConfig);
    
    /* 全局过滤器：标准帧和扩展帧都接受，远程帧过滤 */
    HAL_FDCAN_ConfigGlobalFilter(&hfdcan1, FDCAN_ACCEPT_IN_RX_FIFO0, 
                                  FDCAN_ACCEPT_IN_RX_FIFO0, 
                                  FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);
    HAL_FDCAN_ConfigGlobalFilter(&hfdcan2, FDCAN_ACCEPT_IN_RX_FIFO0, 
                                  FDCAN_ACCEPT_IN_RX_FIFO0, 
                                  FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);
    
    /* 启动FDCAN外设 */
    HAL_FDCAN_Start(&hfdcan1);
    HAL_FDCAN_Start(&hfdcan2);
    
    /* 使能RXFIFO0新消息中断 */
    HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
    HAL_FDCAN_ActivateNotification(&hfdcan2, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
}

/**
 * @brief FDCAN1发送
 * @param id   CAN报文ID
 * @param data 8字节数据指针
 */
void BSP_CAN1_Send(uint32_t id, uint8_t *data)
{
    txHeader1.Identifier = id;                /* 报文ID */
    txHeader1.IdType = FDCAN_EXTENDED_ID;     /* 扩展ID（29位） */
    txHeader1.TxFrameType = FDCAN_DATA_FRAME; /* 数据帧 */
    txHeader1.DataLength = FDCAN_DLC_BYTES_8; /* 数据长度固定8字节 */
    txHeader1.ErrorStateIndicator = FDCAN_ESI_ACTIVE; /* 错误状态：主动 */
    txHeader1.BitRateSwitch = FDCAN_BRS_OFF;  /* 关闭可变波特率 */
    txHeader1.FDFormat = FDCAN_CLASSIC_CAN;    /* 经典CAN格式 */
    txHeader1.TxEventFifoControl = FDCAN_NO_TX_EVENTS; /* 不使用发送事件FIFO */
    txHeader1.MessageMarker = 0;              /* 消息标记（用于区分发送源） */

    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &txHeader1, data);
}

/**
 * @brief FDCAN2发送
 * @param id   CAN报文ID
 * @param data 8字节数据指针
 */
void BSP_CAN2_Send(uint32_t id, uint8_t *data)
{
    txHeader2.Identifier = id;                /* 报文ID */
    txHeader2.IdType = FDCAN_EXTENDED_ID;     /* 扩展ID（29位） */
    txHeader2.TxFrameType = FDCAN_DATA_FRAME; /* 数据帧 */
    txHeader2.DataLength = FDCAN_DLC_BYTES_8; /* 数据长度固定8字节 */
    txHeader2.ErrorStateIndicator = FDCAN_ESI_ACTIVE; /* 错误状态：主动 */
    txHeader2.BitRateSwitch = FDCAN_BRS_OFF;  /* 关闭可变波特率 */
    txHeader2.FDFormat = FDCAN_CLASSIC_CAN;    /* 经典CAN格式 */
    txHeader2.TxEventFifoControl = FDCAN_NO_TX_EVENTS; /* 不使用发送事件FIFO */
    txHeader2.MessageMarker = 1;              /* 消息标记（与FDCAN1区分） */

    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &txHeader2, data);
}

/**
 * @brief FDCAN接收中断回调（HAL库自动调用）
 *        ISR只做最轻量工作：读FIFO、打包消息、存队列，不做数据解析
 */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    CAN_Message_t msg;
    FDCAN_RxHeaderTypeDef rxHeader;
    uint8_t data[8];
    
    if (hfdcan == &hfdcan1)  /* FDCAN1收到消息 */
    {
        if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rxHeader, data) == HAL_OK)
        {
            msg.id = rxHeader.Identifier;  /* 提取CAN ID */
            msg.channel = 1;               /* 标记来源：FDCAN1 */
            memcpy(msg.data, data, 8);     /* 复制8字节数据 */
            BSP_CAN1_RxCallback(&msg);     /* 存入消息队列 */
        }
    }
    else if (hfdcan == &hfdcan2)  /* FDCAN2收到消息 */
    {
        if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rxHeader, data) == HAL_OK)
        {
            msg.id = rxHeader.Identifier;  /* 提取CAN ID */
            msg.channel = 2;               /* 标记来源：FDCAN2 */
            memcpy(msg.data, data, 8);     /* 复制8字节数据 */
            BSP_CAN2_RxCallback(&msg);     /* 存入消息队列 */
        }
    }
}
