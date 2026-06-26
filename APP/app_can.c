/**
 * @file app_can.c
 * @brief CAN数据解析实现
 *        ISR只存消息到队列，任务再解析，避免中断中做耗时操作
 * @version 1.2.0
 * @date 2025-01-01
 */

#include "app_can.h"
#include "app_motec.h"
#include "bsp_can.h"
#include "Variable.h"
#include "cmsis_os2.h"

/* Private defines ----------------------------------------------------------*/
#define CAN_QUEUE_SIZE  32  /* 消息队列深度 */

/* Private variables --------------------------------------------------------*/
static osMessageQueueId_t canQueue = NULL;

/* Exported functions -------------------------------------------------------*/

/**
 * @brief 初始化CAN应用
 */
void APP_CAN_Init(void)
{
    BSP_CAN_Init();
    APP_Motec_Init();
    
    /* 创建消息队列 */
    canQueue = osMessageQueueNew(CAN_QUEUE_SIZE, sizeof(CAN_Message_t), NULL);
}

/**
 * @brief CAN数据处理任务
 *        阻塞在消息队列上，有消息就取出解析，解析完通知MQTT上传
 */
void APP_CAN_TaskProcess(void)
{
    CAN_Message_t msg;
    
    /* 从队列取消息，阻塞等待 */
    if (osMessageQueueGet(canQueue, &msg, NULL, osWaitForever) == osOK)
    {
        if (msg.id < 0x800)
            APP_CAN_Decode(msg.id, msg.data);
        else
            APP_CAN_DecodePower(msg.id, msg.data);
        
        /* 非阻塞排空队列中剩余消息 */
        while (osMessageQueueGet(canQueue, &msg, NULL, 0) == osOK)
        {
            if (msg.id < 0x800)
                APP_CAN_Decode(msg.id, msg.data);
            else
                APP_CAN_DecodePower(msg.id, msg.data);
        }
    }
}

/**
 * @brief 解析FDCAN1数据: VCU汇总(0x211) + VCU↔MCU扩展ID
 */
void APP_CAN_Decode(uint32_t id, uint8_t *data)
{
    switch (id)
    {
        case 0x5F0:
            APP_Motec_ProcessFrame(data);
            break;

        case CAN_ID_VCU_SUMMARY:  /* 0x211 整车状态汇总 */
            g_vehicleData.speed = data[0];              /* 前轮车速 1Byte */
            g_vehicleData.throttle = data[1];           /* 油门踏板开度 1Byte */
            g_vehicleData.brake = data[2];              /* 刹车踏板开度 1Byte */
            g_vehicleData.car_travel = data[3];         /* 车辆跑动距离 1Byte */
            g_vehicleData.imd_safe = data[4] & 0x01;           /* bit0: IMD绝缘监测 1=正常 */
            g_vehicleData.bms_safe = (data[4] >> 1) & 0x01;    /* bit1: BMS电池安全 1=正常 */
            g_vehicleData.emerg_stop = (data[4] >> 2) & 0x01;  /* bit2: 急停开关 */
            g_vehicleData.inertia_switch = (data[4] >> 3) & 0x01;  /* bit3: 惯性开关 */
            g_vehicleData.brake_overtr_switch = (data[4] >> 4) & 0x01; /* bit4: 刹车过度开关 */
            g_vehicleData.hvd = (data[4] >> 5) & 0x01;         /* bit5: 高压断路 */
            g_vehicleData.lmcs = (data[4] >> 6) & 0x01;        /* bit6: 左MCU安全状态 */
            g_vehicleData.rmcs = (data[4] >> 7) & 0x01;        /* bit7: 右MCU安全状态 */
            g_vehicleData.bspd_safe = data[5] & 0x01;          /* bit0: BSPD制动安全 */
            g_vehicleData.gear = (data[5] >> 5) & 0x03;        /* bit5-6: 挡位 0:N,1:AC,2:SK */
            g_vehicleData.steering_angle = data[6] - 90;       /* 转向角 offset:-90 */
            break;

        case CAN_ID_VCU_TO_MCU1:  /* 0x08C1EF21 VCU→左MCU */
            g_vehicleData.rpm_target_left = (data[0] + data[1] * 256) / 2 - 10000;        /* 左电机目标转速 offset:-10000 分辨率:0.5 */
            g_vehicleData.torque_target_left = (data[2] + data[3] * 256);                 /* 左电机目标转矩 2Byte */
            g_vehicleData.l_target_controlmodeorder = data[4];                             /* 左电机目标控制模式指令 */
            g_vehicleData.l_gearstage = data[5] & 0x03;                                    /* 左电机挡位状态 */
            g_vehicleData.dccur = (data[6] + data[7] * 256) * 0.1;                       /* 直流母线电压 分辨率:0.1 */
            break;
        case CAN_ID_VCU_TO_MCU2:  /* 0x08B1EF21 VCU→右MCU */
            g_vehicleData.rpm_target_right = (data[0] + data[1] * 256) / 2 - 10000;       /* 右电机目标转速 offset:-10000 分辨率:0.5 */
            g_vehicleData.torque_target_right = (data[2] + data[3] * 256);                /* 右电机目标转矩 2Byte */
            g_vehicleData.r_target_controlmodeorder = data[4];                             /* 右电机目标控制模式指令 */
            g_vehicleData.r_gearstage = data[5] & 0x03;                                    /* 右电机挡位状态 */
            break;
        case CAN_ID_MCU1_TO_VCU:  /* 0x0CFFC6EF 左MCU→VCU */
            g_vehicleData.rpm_left = (data[0] + data[1] * 256) / 2 - 10000;               /* 左电机实际转速 offset:-10000 分辨率:0.5 */
            g_vehicleData.torque_left = (data[2] + data[3] * 256);                        /* 左电机实际转矩 2Byte */
            g_vehicleData.l_controlmodeorder = data[4];                             /* 左电机控制模式指令 */
            g_vehicleData.l_mcu_ready = data[5] & 0x01;                             /* bit0: 左MCU就绪 */
            g_vehicleData.l_mcu_precharge_state = (data[5] >> 1) & 0x01;           /* bit1: 左MCU预充电状态 */
            g_vehicleData.l_mcu_wrong_code = (data[5] >> 2) | ((data[6] & 0x03) << 6); /* 错误码（跨字节） */
            g_vehicleData.l_mcu_selftest_state = (data[6] >> 7) & 0x01;            /* bit7: 左MCU自检状态 */
            g_vehicleData.l_mcu_alert = data[7] & 0x03;                             /* bit0-1: 左MCU告警级别 */
            break;
        case CAN_ID_MCU2_TO_VCU:  /* 0x0CB221EF 右MCU→VCU */
            g_vehicleData.rpm_right = (data[0] + data[1] * 256) / 2 - 10000;              /* 右电机实际转速 offset:-10000 分辨率:0.5 */
            g_vehicleData.torque_right = (data[2] + data[3] * 256);                      /* 右电机实际转矩 2Byte */
            g_vehicleData.r_controlmodeorder = data[4];                             /* 右电机控制模式指令 */
            g_vehicleData.r_mcu_ready = data[5] & 0x01;                             /* bit0: 右MCU就绪 */
            g_vehicleData.r_mcu_precharge_state = (data[5] >> 1) & 0x01;           /* bit1: 右MCU预充电状态 */
            g_vehicleData.r_mcu_wrong_code = (data[5] >> 2) | ((data[6] & 0x03) << 6); /* 错误码（跨字节） */
            g_vehicleData.r_mcu_selftest_state = (data[6] >> 7) & 0x01;            /* bit7: 右MCU自检状态 */
            g_vehicleData.r_mcu_alert = data[7] & 0x03;                             /* bit0-1: 右MCU告警级别 */
            break;
        case CAN_ID_MCU1_TO_VCU2:  /* 0x0CFFC7EF 左MCU→VCU电气参数 */
            g_vehicleData.temp_controller_left = data[0] - 50;                      /* 左控制器温度 offset:-50 */
            g_vehicleData.temp_motor_left = data[1] - 50;                           /* 左电机温度 offset:-50 */
            g_vehicleData.lmcu_dcvol = (data[2] + data[3] * 256) * 0.1;           /* 左电机直流母线电压 分辨率:0.1 */
            g_vehicleData.lmcu_dccur = (data[4] + data[5] * 256) * 0.1 - 1600;    /* 左电机直流母线电流 offset:-1600 分辨率:0.1 */
            g_vehicleData.lmcu_accur = (data[6] + data[7] * 256) * 0.1 - 1600;    /* 左电机交流电流有效值 offset:-1600 分辨率:0.1 */
            break;
        case CAN_ID_MCU2_TO_VCU2:  /* 0x0CB321EF 右MCU→VCU电气参数 */
            g_vehicleData.temp_controller_right = data[0] - 50;                     /* 右控制器温度 offset:-50 */
            g_vehicleData.temp_motor_right = data[1] - 50;                          /* 右电机温度 offset:-50 */
            g_vehicleData.rmcu_dcvol = (data[2] + data[3] * 256) * 0.1;           /* 右电机直流母线电压 分辨率:0.1 */
            g_vehicleData.rmcu_dccur = (data[4] + data[5] * 256) * 0.1 - 1600;    /* 右电机直流母线电流 offset:-1600 分辨率:0.1 */
            g_vehicleData.rmcu_accur = (data[6] + data[7] * 256) * 0.1 - 1600;    /* 右电机交流电流有效值 offset:-1600 分辨率:0.1 */
            break;
    }
}

/**
 * @brief 解析FDCAN2数据: IMU(0x050) + BMS扩展ID
 */
void APP_CAN_DecodePower(uint32_t id, uint8_t *data)
{
    switch (id)
    {
        case CAN_ID_IMU:  /* 0x50 IMU惯性测量单元 */
            {
                uint8_t sensor_diff = data[1];
                if (sensor_diff == 0x51)  /* 加速度数据 */
                {
                    g_vehicleData.accel_x = ((int16_t)(data[2] | data[3] << 8)) / 32768.0f * 16.0f;  /* X轴加速度 ±16g */
                    g_vehicleData.accel_y = ((int16_t)(data[4] | data[5] << 8)) / 32768.0f * 16.0f;  /* Y轴加速度 ±16g */
                    g_vehicleData.accel_z = ((int16_t)(data[6] | data[7] << 8)) / 32768.0f * 16.0f;  /* Z轴加速度 ±16g */
                }
                else if (sensor_diff == 0x53)  /* 姿态角数据 */
                {
                    g_vehicleData.roll = ((int16_t)(data[2] | data[3] << 8)) / 32768.0f * 180.0f;   /* 横滚角 ±180° */
                    g_vehicleData.pitch = ((int16_t)(data[4] | data[5] << 8)) / 32768.0f * 180.0f;  /* 俯仰角 ±180° */
                    g_vehicleData.yaw = ((int16_t)(data[6] | data[7] << 8)) / 32768.0f * 180.0f;    /* 航向角 ±180° */
                }
            }
            break;

        case CAN_ID_BMS_STATUS:  /* 0x186040F3 BMS电池状态 */
            g_vehicleData.voltage = (data[1] + data[0] * 256) * 0.1;          /* 动力电池总电压 分辨率:0.1V */
            g_vehicleData.current = (data[3] + data[2] * 256) * 0.1 - 1000;  /* 动力电池总电流 offset:-1000 分辨率:0.1A */
            g_vehicleData.soc = data[4];                                       /* 电池SOC 0-100% */
            g_vehicleData.soh = data[5];                                       /* 电池SOH */
            g_vehicleData.bat_state = data[6] >> 4;                            /* 高4位: 电池状态 */
            g_vehicleData.bat_alarm_level = data[6] & 0x0F;                   /* 低4位: 告警级别 */
            g_vehicleData.bat_life = data[7];                                  /* 通信生命信息 */
            break;

        case CAN_ID_BMS_CELL_VOLT:  /* 0x186140F3 BMS单体电压 */
            g_vehicleData.cell_voltage_max = data[0] * 256 + data[1];  /* 最高单体电压 */
            g_vehicleData.cell_voltage_min = data[2] * 256 + data[3];  /* 最低单体电压 */
            break;
    }
}

/**
 * @brief FDCAN1接收回调（ISR中调用，只存消息+设标志，不做解析）
 */
void BSP_CAN1_RxCallback(CAN_Message_t *msg)
{
    osMessageQueuePut(canQueue, msg, 0, 0);
}

/**
 * @brief FDCAN2接收回调（ISR中调用，只存消息+设标志，不做解析）
 */
void BSP_CAN2_RxCallback(CAN_Message_t *msg)
{
    osMessageQueuePut(canQueue, msg, 0, 0);
}

/**
 * @brief 发送按键状态CAN报文
 */
void APP_CAN_SendKeyState(uint8_t keyState)
{
    uint8_t data[8] = {0};
    data[0] = keyState;
    
    BSP_CAN1_Send(0x155, data);
}
