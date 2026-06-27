/**
 * @file app_can.c
 * @brief CAN数据解析实现
 *        ISR只存消息到队列，任务再解析，避免中断中做耗时操作
 * @version 1.3.0
 */

#include "app_can.h"
#include "app_motec.h"
#include "bsp_can.h"
#include "Variable.h"
#include "cmsis_os2.h"

/* Private defines ----------------------------------------------------------*/
#define CAN_QUEUE_SIZE  32

/* Private variables --------------------------------------------------------*/
static osMessageQueueId_t canQueue = NULL;

/**
 * @brief 初始化CAN应用
 */
void APP_CAN_Init(void)
{
    BSP_CAN_Init();
    APP_Motec_Init();
    canQueue = osMessageQueueNew(CAN_QUEUE_SIZE, sizeof(CAN_Message_t), NULL);
}

/**
 * @brief CAN数据处理任务
 *        阻塞在消息队列上，有消息就取出解析，解析完通知MQTT上传
 */
void APP_CAN_TaskProcess(void)
{
    CAN_Message_t msg;

    if (osMessageQueueGet(canQueue, &msg, NULL, osWaitForever) == osOK)
    {
        if (msg.channel == 1)
            APP_CAN_Decode(msg.id, msg.data);
        else
            APP_CAN_DecodePower(msg.id, msg.data);

        while (osMessageQueueGet(canQueue, &msg, NULL, 0) == osOK)
        {
            if (msg.channel == 1)
                APP_CAN_Decode(msg.id, msg.data);
            else
                APP_CAN_DecodePower(msg.id, msg.data);
        }
    }
}

/**
 * @brief 解析FDCAN1数据: VCU汇总(0x211) + VCU↔MCU扩展ID + MoTeC(0x5F0)
 */
void APP_CAN_Decode(uint32_t id, uint8_t *data)
{
    osMutexAcquire(g_dataMutex, osWaitForever);  /* SW-H5修复: 保护g_vehicleData */
    switch (id)
    {
        case 0x5F0:  /* MoTeC油车ECU 18帧数据 */
            APP_Motec_ProcessFrame(data);
            break;

        case CAN_ID_VCU_SUMMARY:  /* 0x211 整车状态汇总 */
            g_motecData.carType = 0;  /* 收到VCU数据，标记为电车 */
            g_vehicleData.speed = data[0];              /* [0] 前轮车速 */
            g_vehicleData.throttle = data[1];           /* [1] 油门踏板开度 */
            g_vehicleData.brake = data[2];              /* [2] 刹车踏板开度 */
            g_vehicleData.car_travel = data[3];         /* [3] 行驶里程 */
            g_vehicleData.imd_safe = data[4] & 0x01;           /* [4] bit0: IMD绝缘监测 */
            g_vehicleData.bms_safe = (data[4] >> 1) & 0x01;    /* [4] bit1: BMS电池安全 */
            g_vehicleData.emerg_stop = (data[4] >> 2) & 0x01;  /* [4] bit2: 急停开关 */
            g_vehicleData.inertia_switch = (data[4] >> 3) & 0x01;  /* [4] bit3: 惯性开关 */
            g_vehicleData.brake_overtr_switch = (data[4] >> 4) & 0x01; /* [4] bit4: 刹车过度 */
            g_vehicleData.hvd = (data[4] >> 5) & 0x01;         /* [4] bit5: 高压断路 */
            g_vehicleData.lmcs = (data[4] >> 6) & 0x01;        /* [4] bit6: 左MCU安全 */
            g_vehicleData.rmcs = (data[4] >> 7) & 0x01;        /* [4] bit7: 右MCU安全 */
            g_vehicleData.bspd_safe = data[5] & 0x01;          /* [5] bit0: BSPD制动安全 */
            g_vehicleData.gear = (data[5] >> 5) & 0x03;        /* [5] bit5-6: 挡位 */
            g_vehicleData.steering_angle = (int8_t)(data[6] - 90);  /* [6] 转向角 offset:-90, 范围±90° */
            break;

        case CAN_ID_VCU_TO_MCU1:  /* 0x08C1EF21 VCU→左MCU */
            g_vehicleData.rpm_target_left = (int16_t)((data[0] + data[1] * 256) / 2 - 10000);  /* [0:1] 目标转速 */
            g_vehicleData.torque_target_left = (int16_t)(data[2] | (data[3] << 8));             /* [2:3] 目标转矩 */
            g_vehicleData.l_target_controlmodeorder = data[4];                                   /* [4] 控制模式 */
            g_vehicleData.l_gearstage = data[5] & 0x03;                                          /* [5] 挡位状态 */
            g_vehicleData.dccur = (data[6] + data[7] * 256) * 0.1;                              /* [6:7] 直流母线电压 */
            break;

        case CAN_ID_VCU_TO_MCU2:  /* 0x08B1EF21 VCU→右MCU */
            g_vehicleData.rpm_target_right = (int16_t)((data[0] + data[1] * 256) / 2 - 10000); /* [0:1] 目标转速 */
            g_vehicleData.torque_target_right = (int16_t)(data[2] | (data[3] << 8));            /* [2:3] 目标转矩 */
            g_vehicleData.r_target_controlmodeorder = data[4];                                    /* [4] 控制模式 */
            g_vehicleData.r_gearstage = data[5] & 0x03;                                           /* [5] 挡位状态 */
            break;

        case CAN_ID_MCU1_TO_VCU:  /* 0x0CFFC6EF 左MCU→VCU 状态 */
            g_vehicleData.rpm_left = (int16_t)((data[0] + data[1] * 256) / 2 - 10000);  /* [0:1] 实际转速 */
            g_vehicleData.torque_left = (int16_t)(data[2] | (data[3] << 8));              /* [2:3] 实际转矩 */
            g_vehicleData.l_controlmodeorder = data[4];                        /* [4] 控制模式 */
            g_vehicleData.l_mcu_ready = data[5] & 0x01;                       /* [5] bit0: MCU就绪 */
            g_vehicleData.l_mcu_precharge_state = (data[5] >> 1) & 0x01;      /* [5] bit1: 预充状态 */
            g_vehicleData.l_mcu_wrong_code = (data[5] >> 2) | ((data[6] & 0x03) << 6); /* [5:6] 故障码 */
            g_vehicleData.l_mcu_selftest_state = (data[6] >> 7) & 0x01;       /* [6] bit7: 自检状态 */
            g_vehicleData.l_mcu_alert = data[7] & 0x03;                        /* [7] bit0-1: 告警级别 */
            break;

        case CAN_ID_MCU2_TO_VCU:  /* 0x0CB221EF 右MCU→VCU 状态 */
            g_vehicleData.rpm_right = (int16_t)((data[0] + data[1] * 256) / 2 - 10000); /* [0:1] 实际转速 */
            g_vehicleData.torque_right = (int16_t)(data[2] | (data[3] << 8));            /* [2:3] 实际转矩 */
            g_vehicleData.r_controlmodeorder = data[4];                        /* [4] 控制模式 */
            g_vehicleData.r_mcu_ready = data[5] & 0x01;                       /* [5] bit0: MCU就绪 */
            g_vehicleData.r_mcu_precharge_state = (data[5] >> 1) & 0x01;      /* [5] bit1: 预充状态 */
            g_vehicleData.r_mcu_wrong_code = (data[5] >> 2) | ((data[6] & 0x03) << 6); /* [5:6] 错误码 */
            g_vehicleData.r_mcu_selftest_state = (data[6] >> 7) & 0x01;        /* [6] bit7: 自检状态 */
            g_vehicleData.r_mcu_alert = data[7] & 0x03;                         /* [7] 告警级别 */
            break;

        case CAN_ID_MCU1_TO_VCU2:  /* 0x0CFFC7EF 左MCU→VCU 电气参数 */
            g_vehicleData.temp_controller_left = data[0] - 50;           /* [0] 控制器温度 offset:-50 */
            g_vehicleData.temp_motor_left = data[1] - 50;                /* [1] 电机温度 offset:-50 */
            g_vehicleData.lmcu_dcvol = (data[2] + data[3] * 256) * 0.1; /* [2:3] 直流母线电压 */
            g_vehicleData.lmcu_dccur = (data[4] + data[5] * 256) * 0.1 - 1600; /* [4:5] 直流母线电流 */
            g_vehicleData.lmcu_accur = (data[6] + data[7] * 256) * 0.1 - 1600; /* [6:7] 交流电流 */
            break;

        case CAN_ID_MCU2_TO_VCU2:  /* 0x0CB321EF 右MCU→VCU 电气参数 */
            g_vehicleData.temp_controller_right = data[0] - 50;          /* [0] 控制器温度 offset:-50 */
            g_vehicleData.temp_motor_right = data[1] - 50;               /* [1] 电机温度 offset:-50 */
            g_vehicleData.rmcu_dcvol = (data[2] + data[3] * 256) * 0.1; /* [2:3] 直流母线电压 */
            g_vehicleData.rmcu_dccur = (data[4] + data[5] * 256) * 0.1 - 1600; /* [4:5] 直流母线电流 */
            g_vehicleData.rmcu_accur = (data[6] + data[7] * 256) * 0.1 - 1600; /* [6:7] 交流电流 */
            break;

        default:
            break;
    }
    osMutexRelease(g_dataMutex);  /* SW-H5修复 */
}

/**
 * @brief 解析FDCAN2数据: IMU(0x050) + BMS扩展ID
 */
void APP_CAN_DecodePower(uint32_t id, uint8_t *data)
{
    osMutexAcquire(g_dataMutex, osWaitForever);  /* SW-H5修复: 保护g_vehicleData */
    switch (id)
    {
        case CAN_ID_IMU:  /* 0x050 IMU惯性测量单元 */
        {
            uint8_t sensor_diff = data[1];  /* [1] 数据类型标识 */
            if (sensor_diff == 0x51)        /* 加速度数据 */
            {
                g_vehicleData.accel_x = (int16_t)(((int16_t)(data[2] | data[3] << 8)) / 32768.0f * 1600.0f);  /* [2:3] X轴 单位0.01g */
                g_vehicleData.accel_y = (int16_t)(((int16_t)(data[4] | data[5] << 8)) / 32768.0f * 1600.0f);  /* [4:5] Y轴 单位0.01g */
                g_vehicleData.accel_z = (int16_t)(((int16_t)(data[6] | data[7] << 8)) / 32768.0f * 1600.0f);  /* [6:7] Z轴 单位0.01g */
            }
            else if (sensor_diff == 0x53)   /* 姿态角数据 */
            {
                g_vehicleData.roll = ((int16_t)(data[2] | data[3] << 8)) / 32768.0f * 180.0f;   /* [2:3] 横滚角 ±180° */
                g_vehicleData.pitch = ((int16_t)(data[4] | data[5] << 8)) / 32768.0f * 180.0f;  /* [4:5] 俯仰角 ±180° */
                g_vehicleData.yaw = ((int16_t)(data[6] | data[7] << 8)) / 32768.0f * 180.0f;    /* [6:7] 航向角 ±180° */
            }
        }
            break;

        case CAN_ID_BMS_STATUS:  /* 0x186040F3 BMS电池状态 大端序 */
            g_vehicleData.voltage = (data[1] + data[0] * 256) * 0.1;          /* [0:1] 总电压 0.1V */
            g_vehicleData.current = (data[3] + data[2] * 256) * 0.1 - 1000;  /* [2:3] 总电流 offset:-1000 */
            g_vehicleData.soc = data[4];                                       /* [4] SOC 0-100% */
            g_vehicleData.soh = data[5];                                       /* [5] SOH 健康度 */
            g_vehicleData.bat_state = data[6] >> 4;                            /* [6] 高4位: 电池状态 */
            g_vehicleData.bat_alarm_level = data[6] & 0x0F;                   /* [6] 低4位: 告警级别 */
            g_vehicleData.bat_life = data[7];                                  /* [7] 通信生命计数 */
            break;

        case CAN_ID_BMS_CELL_VOLT:  /* 0x186140F3 BMS单体电压 大端序 */
            g_vehicleData.cell_voltage_max = data[0] * 256 + data[1];  /* [0:1] 最高单体电压 */
            g_vehicleData.cell_voltage_min = data[2] * 256 + data[3];  /* [2:3] 最低单体电压 */
            break;

        default:
            break;
    }
    osMutexRelease(g_dataMutex);  /* SW-H5修复 */
}

/**
 * @brief FDCAN接收回调（ISR中调用，只存消息到队列）
 */
void BSP_CAN1_RxCallback(CAN_Message_t *msg)
{
    osMessageQueuePut(canQueue, msg, 0, 0);
}

void BSP_CAN2_RxCallback(CAN_Message_t *msg)
{
    osMessageQueuePut(canQueue, msg, 0, 0);
}

/**
 * @brief 发送按键状态CAN报文
 * @param keyState 按键位掩码（8个按键各占1位）
 */
void APP_CAN_SendKeyState(uint8_t keyState)
{
    uint8_t data[8] = {0};
    data[0] = keyState;         /* [0] 按键状态位掩码 */
    data[1] = 0;                /* [1] 保留 */
    data[2] = 0;                /* [2] 保留 */
    data[3] = 0;                /* [3] 保留 */
    data[4] = 0;                /* [4] 保留 */
    data[5] = 0;                /* [5] 保留 */
    data[6] = 0;                /* [6] 保留 */
    data[7] = 0;                /* [7] 保留 */
    BSP_CAN1_Send(0x155, data); /* 发送ID 0x155 */
}
