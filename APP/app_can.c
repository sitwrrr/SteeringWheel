/**
 * @file app_can.c
 * @brief CAN数据解析实现
 * @version 1.0.0
 * @date 2025-01-01
 */

#include "app_can.h"
#include "bsp_can.h"
#include "Variable.h"

/* Exported functions -------------------------------------------------------*/

/**
 * @brief 初始化CAN应用
 */
void APP_CAN_Init(void)
{
    BSP_CAN_Init();
}

/**
 * @brief 处理CAN数据（在freertos.c中调用）
 */
void APP_CAN_Process(void)
{
    CAN_Message_t msg;
    
    /* 处理CAN1数据 */
    while (BSP_CAN1_Receive(&msg))
    {
        APP_CAN_Decode(msg.id, msg.data);
    }
    
    /* 处理CAN2数据 */
    while (BSP_CAN2_Receive(&msg))
    {
        APP_CAN_DecodePower(msg.id, msg.data);
    }
}

/**
 * @brief 解析CAN数据（电机数据）
 * @param id: CAN ID
 * @param data: 数据指针
 */
void APP_CAN_Decode(uint32_t id, uint8_t *data)
{
    switch (id)
    {
        case 0x08C1EF21:  /* VCUtoMCU1 */
            g_vehicleData.rpm_left = (data[0] + data[1] * 256) / 2 - 10000;
            g_vehicleData.torque_left = (data[2] + data[3] * 256);
            break;
            
        case 0x08B1EF21:  /* VCUtoMCU2 */
            g_vehicleData.rpm_right = (data[0] + data[1] * 256) / 2 - 10000;
            g_vehicleData.torque_right = (data[2] + data[3] * 256);
            break;
            
        case 0x0CFFC6EF:  /* MCU1toVCU1 */
            g_vehicleData.rpm_left = (data[0] + data[1] * 256) / 2 - 10000;
            g_vehicleData.torque_left = (data[2] + data[3] * 256);
            break;
            
        case 0x0CB221EF:  /* MCU2toVCU1 */
            g_vehicleData.rpm_right = (data[0] + data[1] * 256) / 2 - 10000;
            g_vehicleData.torque_right = (data[2] + data[3] * 256);
            break;
            
        case 0x0CFFC7EF:  /* MCU1toVCU2 */
            g_vehicleData.temp_controller_left = data[0] - 50;
            g_vehicleData.temp_motor_left = data[1] - 50;
            break;
            
        case 0x0CB321EF:  /* MCU2toVCU2 */
            g_vehicleData.temp_controller_right = data[0] - 50;
            g_vehicleData.temp_motor_right = data[1] - 50;
            break;
    }
}

/**
 * @brief 解析电源数据
 * @param id: CAN ID
 * @param data: 数据指针
 */
void APP_CAN_DecodePower(uint32_t id, uint8_t *data)
{
    switch (id)
    {
        case 0x211:  /* 车速、踏板、安全信号 */
            g_vehicleData.speed = data[0];
            g_vehicleData.throttle = data[1];
            g_vehicleData.brake = data[2];
            
            /* 安全信号 */
            g_vehicleData.safety_loop = (data[4] & 0x01) ? 0 : 1;
            g_vehicleData.bms_fault = (data[4] >> 1) & 0x01;
            g_vehicleData.mcu_fault = (data[4] >> 2) & 0x01;
            break;
            
        case 0x212:  /* 电池电压、电流、SOC */
            g_vehicleData.voltage = (data[0] + data[1] * 256) * 0.1;
            g_vehicleData.current = (data[2] + data[3] * 256) * 0.1 - 1000;
            g_vehicleData.soc = data[4];
            break;
            
        case 0x213:  /* 温度 */
            g_vehicleData.temp_battery = data[0] - 50;
            break;
            
        case 0x214:  /* 加速度 */
            g_vehicleData.accel_x = (int16_t)(data[0] | data[1] << 8);
            g_vehicleData.accel_y = (int16_t)(data[2] | data[3] << 8);
            g_vehicleData.accel_z = (int16_t)(data[4] | data[5] << 8);
            g_vehicleData.yaw_rate = (int16_t)(data[6] | data[7] << 8);
            break;
    }
    
    g_vehicleData.timestamp = HAL_GetTick();
}

/**
 * @brief 发送按键状态CAN报文
 * @param keyState: 按键状态位掩码
 */
void APP_CAN_SendKeyState(uint8_t keyState)
{
    uint8_t data[8] = {0};
    data[0] = keyState;
    
    BSP_CAN2_Send(0x050, data, 8);
}
