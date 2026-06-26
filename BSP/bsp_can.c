/**
 * @file bsp_can.c
 * @brief CAN总线驱动实现（中断回调方式）
 * @version 1.1.0
 * @date 2025-01-01
 */

#include "bsp_can.h"
#include <string.h>

/* Private variables --------------------------------------------------------*/
static FDCAN_TxHeaderTypeDef txHeader1;
static FDCAN_TxHeaderTypeDef txHeader2;

/* Exported functions -------------------------------------------------------*/

/**
 * @brief 初始化CAN总线
 */
void BSP_CAN_Init(void)
{
    FDCAN_FilterTypeDef filterConfig;
    
    /* FDCAN1过滤器 */
    filterConfig.IdType = FDCAN_EXTENDED_ID;
    filterConfig.FilterIndex = 1;
    filterConfig.FilterType = FDCAN_FILTER_MASK;
    filterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    filterConfig.FilterID1 = 0;
    filterConfig.FilterID2 = 0;
    HAL_FDCAN_ConfigFilter(&hfdcan1, &filterConfig);
    
    /* FDCAN2过滤器 */
    HAL_FDCAN_ConfigFilter(&hfdcan2, &filterConfig);
    
    /* 全局过滤器 */
    HAL_FDCAN_ConfigGlobalFilter(&hfdcan1, FDCAN_ACCEPT_IN_RX_FIFO0, 
                                  FDCAN_ACCEPT_IN_RX_FIFO0, 
                                  FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);
    HAL_FDCAN_ConfigGlobalFilter(&hfdcan2, FDCAN_ACCEPT_IN_RX_FIFO0, 
                                  FDCAN_ACCEPT_IN_RX_FIFO0, 
                                  FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);
    
    /* 启动FDCAN */
    HAL_FDCAN_Start(&hfdcan1);
    HAL_FDCAN_Start(&hfdcan2);
    
    /* 使能接收中断 */
    HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
    HAL_FDCAN_ActivateNotification(&hfdcan2, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
}

/**
 * @brief FDCAN1发送
 */
void BSP_CAN1_Send(uint32_t id, uint8_t *data)
{
    txHeader1.Identifier = id;
    txHeader1.IdType = FDCAN_EXTENDED_ID;
    txHeader1.TxFrameType = FDCAN_DATA_FRAME;
    txHeader1.DataLength = FDCAN_DLC_BYTES_8;
    txHeader1.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    txHeader1.BitRateSwitch = FDCAN_BRS_OFF;
    txHeader1.FDFormat = FDCAN_CLASSIC_CAN;
    txHeader1.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    txHeader1.MessageMarker = 0;
    
    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &txHeader1, data);
}

/**
 * @brief FDCAN2发送
 */
void BSP_CAN2_Send(uint32_t id, uint8_t *data)
{
    txHeader2.Identifier = id;
    txHeader2.IdType = FDCAN_EXTENDED_ID;
    txHeader2.TxFrameType = FDCAN_DATA_FRAME;
    txHeader2.DataLength = FDCAN_DLC_BYTES_8;
    txHeader2.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    txHeader2.BitRateSwitch = FDCAN_BRS_OFF;
    txHeader2.FDFormat = FDCAN_CLASSIC_CAN;
    txHeader2.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    txHeader2.MessageMarker = 1;
    
    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &txHeader2, data);
}

/**
 * @brief FDCAN接收中断回调（HAL库自动调用）
 */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    CAN_Message_t msg;
    FDCAN_RxHeaderTypeDef rxHeader;
    uint8_t data[8];
    
    if (hfdcan == &hfdcan1)
    {
        if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rxHeader, data) == HAL_OK)
        {
            msg.id = rxHeader.Identifier;
            memcpy(msg.data, data, 8);

            BSP_CAN1_RxCallback(&msg);
        }
    }
    else if (hfdcan == &hfdcan2)
    {
        if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rxHeader, data) == HAL_OK)
        {
            msg.id = rxHeader.Identifier;
            memcpy(msg.data, data, 8);

            BSP_CAN2_RxCallback(&msg);
        }
    }
}
