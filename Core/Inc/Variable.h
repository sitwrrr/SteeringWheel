#ifndef __VARIABLE_H
#define __VARIABLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include "cmsis_os2.h"

/* 车辆数据结构体 */
typedef struct {
    /* 车速 */
    uint16_t speed;
    
    /* 电机数据（目标值：VCU→MCU） */
    int16_t rpm_target_left;
    int16_t rpm_target_right;
    int16_t torque_target_left;
    int16_t torque_target_right;

    /* 电机数据（实际值：MCU→VCU） */
    int16_t rpm_left;
    int16_t rpm_right;
    int16_t torque_left;
    int16_t torque_right;
    uint8_t temp_motor_left;
    uint8_t temp_motor_right;
    uint8_t temp_controller_left;
    uint8_t temp_controller_right;
    
    /* 电池数据 */
    uint16_t voltage;
    int16_t current;
    uint8_t soc;
    uint8_t soh;
    uint16_t temp_battery;
    uint16_t cell_voltage_max;
    uint16_t cell_voltage_min;
    uint8_t bat_state;
    uint8_t bat_alarm_level;
    uint8_t bat_life;
    
    /* 车辆状态 */
    uint8_t gear;           /* 0:N, 1:AC, 2:SK, 3:AU, 4:EF */
    uint8_t brake;
    uint8_t throttle;
    int8_t steering_angle;  /* SW-H2修复: 改为int8_t，支持负值 */
    uint8_t car_travel;     /* 行驶里程 */

    /* VCU→MCU控制指令 */
    uint8_t l_target_controlmodeorder;
    uint8_t l_gearstage;
    uint8_t r_target_controlmodeorder;
    uint8_t r_gearstage;
    float dccur;            /* 直流母线电压 */

    /* MCU→VCU状态 */
    uint8_t l_controlmodeorder;
    uint8_t r_controlmodeorder;
    uint8_t l_mcu_ready;
    uint8_t l_mcu_precharge_state;
    uint8_t l_mcu_wrong_code;
    uint8_t l_mcu_selftest_state;
    uint8_t l_mcu_alert;
    uint8_t r_mcu_ready;
    uint8_t r_mcu_precharge_state;
    uint8_t r_mcu_wrong_code;
    uint8_t r_mcu_selftest_state;
    uint8_t r_mcu_alert;

    /* MCU电气参数 */
    float lmcu_dcvol;
    float lmcu_dccur;
    float lmcu_accur;
    float rmcu_dcvol;
    float rmcu_dccur;
    float rmcu_accur;

    /* 加速度 */
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t yaw_rate;

    /* 姿态角 */
    float roll;
    float pitch;
    float yaw;

    /* 安全信号 */
    uint8_t imd_safe;
    uint8_t bms_safe;
    uint8_t emerg_stop;
    uint8_t inertia_switch;
    uint8_t brake_overtr_switch;
    uint8_t hvd;
    uint8_t lmcs;
    uint8_t rmcs;
    uint8_t bspd_safe;
    
    /* 输入状态 */
    uint8_t key_state;          /* 按键位掩码 */
    uint8_t key_clicked;        /* 最后点击的按键编号 */
    uint8_t shift_up;           /* 升挡状态 */
    uint8_t shift_down;         /* 降挡状态 */
    
    /* 时间戳 */
    uint32_t timestamp;
} VehicleData_t;

/* 工作模式枚举 */
typedef enum {
    MODE_STANDBY = 0,       /* 待机模式 */
    MODE_VEHICLE,           /* 实车模式 */
    MODE_SIMULATOR,         /* 模拟器模式 */
    MODE_STORAGE            /* 存储模式 */
} WorkMode_t;

/* 挡位枚举 */
typedef enum {
    GEAR_N = 0,             /* 空挡 */
    GEAR_AC = 1,            /* 加速 */
    GEAR_SK = 2,            /* 蛇形 */
    GEAR_AU = 3,            /* 自动 */
    GEAR_EF = 4             /* 紧急 */
} GearType_t;

/* 全局变量声明 */
extern VehicleData_t g_vehicleData;
extern WorkMode_t g_workMode;
extern osMutexId_t g_dataMutex;
extern osMutexId_t g_lvglMutex;
extern osEventFlagsId_t canEventHandle;

/* CAN接收标志 */
extern volatile uint8_t g_can1Received;
extern volatile uint8_t g_can2Received;

/* USB接收缓冲区 */
extern uint8_t g_usbRxBuffer[256];
extern volatile uint16_t g_usbRxLength;

/* MQTT状态 */
extern volatile uint8_t g_mqttConnected;
extern volatile uint8_t g_mqttUploading;

/* Flash存储状态 */
extern volatile uint8_t g_storageActive;
extern uint32_t g_storageCount;

/* 函数声明 */
void Variable_Init(void);
void VehicleData_Reset(void);

#ifdef __cplusplus
}
#endif

#endif /* __VARIABLE_H */
