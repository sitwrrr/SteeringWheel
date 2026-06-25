/**
 * @file app_can.c
 * @brief CAN数据解析实现
 * @version 1.0.0
 * @date 2025-01-01
 */

#include "app_can.h"
#include "app_motec.h"
#include "bsp_can.h"
#include "Variable.h"

/* Exported functions -------------------------------------------------------*/

/**
 * @brief 初始化CAN应用
 */
void APP_CAN_Init(void)
{
    BSP_CAN_Init();
    APP_Motec_Init();
}

/**
 * @brief 处理CAN数据（在freertos.c中调用）
 *        FDCAN1 → APP_CAN_Decode():      VCU汇总(0x211) + VCU↔MCU扩展ID
 *        FDCAN2 → APP_CAN_DecodePower():  IMU(0x050) + BMS扩展ID
 */
void APP_CAN_Process(void)
{
    CAN_Message_t msg;
    
    /* 处理FDCAN1数据: VCU汇总 + VCU↔MCU通信 */
    while (BSP_CAN1_Receive(&msg))
    {
        APP_CAN_Decode(msg.id, msg.data);
    }
    
    /* 处理FDCAN2数据: IMU + BMS */
    while (BSP_CAN2_Receive(&msg))
    {
        APP_CAN_DecodePower(msg.id, msg.data);
    }
}

/**
 * @brief 解析FDCAN1数据: VCU汇总(0x211) + VCU↔MCU扩展ID
 *        参照Steer_2025原项目 decode() 函数
 */
void APP_CAN_Decode(uint32_t id, uint8_t *data)
{
    switch (id)
    {
        /* ========== MoTeC ECU (油车协议) ========== */
        case 0x5F0:
            APP_Motec_ProcessFrame(data);
            break;

        /* ========== VCU汇总数据 (标准ID) ========== */
        case CAN_ID_VCU_SUMMARY:  /* 0x211 */
            g_vehicleData.speed = data[0];
            g_vehicleData.throttle = data[1];
            g_vehicleData.brake = data[2];
            g_vehicleData.car_travel = data[3];
            
            /* Byte4: 安全电路位域 (1=正常, 0=故障，与原项目一致) */
            g_vehicleData.safety_loop = data[4] & 0x01;
            g_vehicleData.bms_fault = (data[4] >> 1) & 0x01;
            g_vehicleData.mcu_fault = (data[4] >> 2) & 0x01;
            
            /* Byte5: BSPD + 档位 */
            g_vehicleData.gear = (data[5] >> 5) & 0x03;
            
            /* Byte6: 转向角 */
            g_vehicleData.steering_angle = data[6] - 90;
            break;

        /* ========== VCU↔MCU通信 (扩展ID，仪表旁听) ========== */
        case CAN_ID_VCU_TO_MCU1:  /* 0x08C1EF21 VCU→左电机 */
            g_vehicleData.rpm_left = (data[0] + data[1] * 256) / 2 - 10000;
            g_vehicleData.torque_left = (data[2] + data[3] * 256);
            break;
            
        case CAN_ID_VCU_TO_MCU2:  /* 0x08B1EF21 VCU→右电机 */
            g_vehicleData.rpm_right = (data[0] + data[1] * 256) / 2 - 10000;
            g_vehicleData.torque_right = (data[2] + data[3] * 256);
            break;
            
        case CAN_ID_MCU1_TO_VCU:  /* 0x0CFFC6EF 左电机→VCU (状态) */
            g_vehicleData.rpm_left = (data[0] + data[1] * 256) / 2 - 10000;
            g_vehicleData.torque_left = (data[2] + data[3] * 256);
            /* Byte4: 控制模式 */
            /* Byte5[0]: 就绪, [1]: 预充, [2]+Byte6[1:0]: 故障码, Byte6[7]: 自检 */
            /* Byte7[1:0]: 告警级别 */
            break;
            
        case CAN_ID_MCU2_TO_VCU:  /* 0x0CB221EF 右电机→VCU (状态) */
            g_vehicleData.rpm_right = (data[0] + data[1] * 256) / 2 - 10000;
            g_vehicleData.torque_right = (data[2] + data[3] * 256);
            break;
            
        case CAN_ID_MCU1_TO_VCU2:  /* 0x0CFFC7EF 左电机→VCU (温度/电压/电流) */
            g_vehicleData.temp_controller_left = data[0] - 50;
            g_vehicleData.temp_motor_left = data[1] - 50;
            /* Byte2-3: 直流电压 (raw*0.1 V) */
            /* Byte4-5: 直流电流 (raw*0.1-1600 A) */
            /* Byte6-7: 交流电流 (raw*0.1-1600 A) */
            break;
            
        case CAN_ID_MCU2_TO_VCU2:  /* 0x0CB321EF 右电机→VCU (温度/电压/电流) */
            g_vehicleData.temp_controller_right = data[0] - 50;
            g_vehicleData.temp_motor_right = data[1] - 50;
            break;
    }
    
    g_vehicleData.timestamp = HAL_GetTick();
}

/**
 * @brief 解析FDCAN2数据: IMU(0x050) + BMS扩展ID
 *        参照Steer_2025原项目 decode_power() 函数
 */
void APP_CAN_DecodePower(uint32_t id, uint8_t *data)
{
    switch (id)
    {
        /* ========== IMU传感器 (标准ID) ========== */
        case CAN_ID_IMU:  /* 0x050 */
            {
                uint8_t sensor_diff = data[1];
                if (sensor_diff == 0x51)  /* 加速度 */
                {
                    g_vehicleData.accel_x = ((int16_t)(data[2] | data[3] << 8)) / 32768.0f * 16.0f;
                    g_vehicleData.accel_y = ((int16_t)(data[4] | data[5] << 8)) / 32768.0f * 16.0f;
                    g_vehicleData.accel_z = ((int16_t)(data[6] | data[7] << 8)) / 32768.0f * 16.0f;
                }
                else if (sensor_diff == 0x53)  /* 姿态角 */
                {
                    g_vehicleData.roll = ((int16_t)(data[2] | data[3] << 8)) / 32768.0f * 180.0f;
                    g_vehicleData.pitch = ((int16_t)(data[4] | data[5] << 8)) / 32768.0f * 180.0f;
                    g_vehicleData.yaw = ((int16_t)(data[6] | data[7] << 8)) / 32768.0f * 180.0f;
                }
            }
            break;

        /* ========== BMS状态 (扩展ID，大端序) ========== */
        case CAN_ID_BMS_STATUS:  /* 0x186040F3 */
            g_vehicleData.voltage = (data[1] + data[0] * 256) * 0.1;
            g_vehicleData.current = (data[3] + data[2] * 256) * 0.1 - 1000;
            g_vehicleData.soc = data[4];
            g_vehicleData.soh = data[5];
            g_vehicleData.bat_state = data[6] >> 4;
            g_vehicleData.bat_alarm_level = data[6] & 0x0F;
            g_vehicleData.bat_life = data[7];
            break;

        case CAN_ID_BMS_CELL_VOLT:  /* 0x186140F3 */
            g_vehicleData.cell_voltage_max = data[0] * 256 + data[1];
            g_vehicleData.cell_voltage_min = data[2] * 256 + data[3];
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
    
    BSP_CAN1_Send(0x155, data, 8);  /* 按键状态发送到FDCAN1 */
}
