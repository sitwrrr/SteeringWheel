/**
 * @file bsp_can.c
 * @brief CAN总线驱动实现
 * @version 1.0.0
 * @date 2025-01-01
 */

#include "bsp_can.h"
#include "fdcan.h"

/* Private variables --------------------------------------------------------*/
static FDCAN_TxHeaderTypeDef txHeader1;
static FDCAN_TxHeaderTypeDef txHeader2;
static FDCAN_RxHeaderTypeDef rxHeader1;
static FDCAN_RxHeaderTypeDef rxHeader2;

/* Private functions --------------------------------------------------------*/

/**
 * @brief 配置FDCAN1过滤器
 */
static void CAN1_FilterConfig(void)
{
    FDCAN_FilterTypeDef filterConfig;
    
    filterConfig.IdType = FDCAN_EXTENDED_ID;
    filterConfig.FilterIndex = 1;
    filterConfig.FilterType = FDCAN_FILTER_MASK;
    filterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    filterConfig.FilterID1 = 0;
    filterConfig.FilterID2 = 0;
    
    if (HAL_FDCAN_ConfigFilter(&hfdcan1, &filterConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief 配置FDCAN2过滤器
 */
static void CAN2_FilterConfig(void)
{
    FDCAN_FilterTypeDef filterConfig;
    
    filterConfig.IdType = FDCAN_EXTENDED_ID;
    filterConfig.FilterIndex = 1;
    filterConfig.FilterType = FDCAN_FILTER_MASK;
    filterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    filterConfig.FilterID1 = 0;
    filterConfig.FilterID2 = 0;
    
    if (HAL_FDCAN_ConfigFilter(&hfdcan2, &filterConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

/* Exported functions -------------------------------------------------------*/

/**
 * @brief 初始化CAN总线
 */
void BSP_CAN_Init(void)
{
    CAN1_FilterConfig();
    CAN2_FilterConfig();
    
    HAL_FDCAN_ConfigGlobalFilter(&hfdcan1, FDCAN_ACCEPT_IN_RX_FIFO0, 
                                  FDCAN_ACCEPT_IN_RX_FIFO0, 
                                  FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);
    HAL_FDCAN_ConfigGlobalFilter(&hfdcan2, FDCAN_ACCEPT_IN_RX_FIFO0, 
                                  FDCAN_ACCEPT_IN_RX_FIFO0, 
                                  FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);
    
    if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK) Error_Handler();
    if (HAL_FDCAN_Start(&hfdcan2) != HAL_OK) Error_Handler();
    
    if (HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK)
        Error_Handler();
    if (HAL_FDCAN_ActivateNotification(&hfdcan2, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK)
        Error_Handler();
}

/**
 * @brief FDCAN1发送数据
 * @param id: CAN ID
 * @param data: 数据指针
 * @param len: 数据长度
 */
void BSP_CAN1_Send(uint32_t id, uint8_t *data, uint8_t len)
{
    txHeader1.Identifier = id;
    txHeader1.IdType = FDCAN_EXTENDED_ID;
    txHeader1.TxFrameType = FDCAN_DATA_FRAME;
    txHeader1.DataLength = len << 16;
    txHeader1.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    txHeader1.BitRateSwitch = FDCAN_BRS_OFF;
    txHeader1.FDFormat = FDCAN_CLASSIC_CAN;
    txHeader1.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    txHeader1.MessageMarker = 0;
    
    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &txHeader1, data);
}

/**
 * @brief FDCAN2发送数据
 * @param id: CAN ID
 * @param data: 数据指针
 * @param len: 数据长度
 */
void BSP_CAN2_Send(uint32_t id, uint8_t *data, uint8_t len)
{
    txHeader2.Identifier = id;
    txHeader2.IdType = FDCAN_EXTENDED_ID;
    txHeader2.TxFrameType = FDCAN_DATA_FRAME;
    txHeader2.DataLength = len << 16;
    txHeader2.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    txHeader2.BitRateSwitch = FDCAN_BRS_OFF;
    txHeader2.FDFormat = FDCAN_CLASSIC_CAN;
    txHeader2.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    txHeader2.MessageMarker = 1;
    
    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &txHeader2, data);
}

/**
 * @brief 配置CAN过滤器（兼容旧接口）
 */
void BSP_CAN_FilterConfig(void)
{
    BSP_CAN_Init();
}

/**
 * @brief FDCAN1接收数据
 * @param msg: 消息结构体
 * @return 1=接收到数据, 0=无数据
 */
uint8_t BSP_CAN1_Receive(CAN_Message_t *msg)
{
    if (HAL_FDCAN_GetRxFifoFillLevel(&hfdcan1, FDCAN_RX_FIFO0) == 0)
        return 0;
    
    uint8_t data[8];
    if (HAL_FDCAN_GetRxMessage(&hfdcan1, FDCAN_RX_FIFO0, &rxHeader1, data) != HAL_OK)
        return 0;
    
    msg->id = rxHeader1.Identifier;
    msg->len = rxHeader1.DataLength >> 16;
    for (uint8_t i = 0; i < msg->len; i++)
        msg->data[i] = data[i];
    
    return 1;
}

/**
 * @brief FDCAN2接收数据
 * @param msg: 消息结构体
 * @return 1=接收到数据, 0=无数据
 */
uint8_t BSP_CAN2_Receive(CAN_Message_t *msg)
{
    if (HAL_FDCAN_GetRxFifoFillLevel(&hfdcan2, FDCAN_RX_FIFO0) == 0)
        return 0;
    
    uint8_t data[8];
    if (HAL_FDCAN_GetRxMessage(&hfdcan2, FDCAN_RX_FIFO0, &rxHeader2, data) != HAL_OK)
        return 0;
    
    msg->id = rxHeader2.Identifier;
    msg->len = rxHeader2.DataLength >> 16;
    for (uint8_t i = 0; i < msg->len; i++)
        msg->data[i] = data[i];
    
    return 1;
}
