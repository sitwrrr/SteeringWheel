/**
 * @file app_motec.c
 * @brief MoTeC ECU CAN协议解析实现（燃油车）
 *
 * MoTeC M800 ECU通过CAN ID 0x5F0发送144字节数据（18帧×8字节）。
 * 帧头标识: byte[4]=0xFC, byte[5]=0xFB, byte[6]=0xFA
 *
 * @version 1.0.0
 * @date 2025-01-01
 */

#include "app_motec.h"
#include <string.h>

/* Private defines ----------------------------------------------------------*/
#define MOTEC_FRAME_INDEX_RPM           1   /* 帧1: 发动机转速 + 节气门 */
#define MOTEC_FRAME_INDEX_ENG_TEMP      2   /* 帧2: 发动机温度 + Lambda */
#define MOTEC_FRAME_INDEX_OIL_TEMP      3   /* 帧3: 机油温度 */
#define MOTEC_FRAME_INDEX_OIL_PRESS     4   /* 帧4: 机油压力 */
#define MOTEC_FRAME_INDEX_BAT_VOL       6   /* 帧6: 低压电池电压 + ECU温度 */
#define MOTEC_FRAME_INDEX_GEAR          14  /* 帧14: 档位 */

/* 齿比定义（凌云VIII号燃油赛车） */
#define GEAR_RATIO_1    2.615f
#define GEAR_RATIO_2    1.857f
#define GEAR_RATIO_3    1.565f
#define GEAR_RATIO_4    1.350f
#define FINAL_DRIVE     3.27f
#define WHEEL_CIRC      1.44f   /* 轮周长(m) */

/* Private variables --------------------------------------------------------*/
static uint8_t motecFrameBuf[MOTEC_FRAME_COUNT][MOTEC_FRAME_SIZE];
static uint8_t frameCounter = 0;
static uint8_t frameStartFlag = 0;

/* Exported variables -------------------------------------------------------*/
MotecData_t g_motecData;

/* Private functions --------------------------------------------------------*/

/**
 * @brief 判断是否为帧头
 */
static uint8_t isFrameHeader(uint8_t *data)
{
    return (data[4] == MOTEC_HEADER_BYTE4 &&
            data[5] == MOTEC_HEADER_BYTE5 &&
            data[6] == MOTEC_HEADER_BYTE6);
}

/**
 * @brief 判断帧尾校验是否正确
 */
static uint8_t isFrameEndValid(void)
{
    return (motecFrameBuf[0][4] == MOTEC_HEADER_BYTE4 &&
            motecFrameBuf[0][5] == MOTEC_HEADER_BYTE5 &&
            motecFrameBuf[0][6] == MOTEC_HEADER_BYTE6 &&
            motecFrameBuf[14][2] == 0x01 &&
            motecFrameBuf[14][3] == 0xF4);
}

/**
 * @brief 根据档位和转速计算车速
 */
static float calculateSpeed(uint16_t rpm, uint16_t gear)
{
    float ratio;

    switch (gear)
    {
        case 1:  ratio = GEAR_RATIO_1; break;
        case 2:  ratio = GEAR_RATIO_2; break;
        case 3:  ratio = GEAR_RATIO_3; break;
        case 4:  ratio = GEAR_RATIO_4; break;
        default: return 0.0f;
    }

    /* 公式: RPM × 轮周长 × 60 / 1000 / 齿比 / 终传比 */
    return (float)rpm * WHEEL_CIRC * 60.0f / 1000.0f / ratio / FINAL_DRIVE;
}

/**
 * @brief 解析MoTeC数据帧组
 */
static void motec_decode(void)
{
    g_motecData.carType = 1; /* 标记为油车 */

    /* 帧1: 发动机转速 + 节气门 */
    g_motecData.engineRPM = (motecFrameBuf[MOTEC_FRAME_INDEX_RPM][0] << 8) |
                             motecFrameBuf[MOTEC_FRAME_INDEX_RPM][1];
    g_motecData.throttlePosition = ((motecFrameBuf[MOTEC_FRAME_INDEX_RPM][2] << 8) |
                                    motecFrameBuf[MOTEC_FRAME_INDEX_RPM][3]) / 10;

    /* 帧2: 发动机温度 + Lambda */
    g_motecData.engineTemp = ((motecFrameBuf[MOTEC_FRAME_INDEX_ENG_TEMP][0] << 8) |
                              motecFrameBuf[MOTEC_FRAME_INDEX_ENG_TEMP][1]) / 10;
    g_motecData.lambda1 = ((motecFrameBuf[MOTEC_FRAME_INDEX_ENG_TEMP][2] << 8) |
                           motecFrameBuf[MOTEC_FRAME_INDEX_ENG_TEMP][3]) / 1000;

    /* 帧3: 机油温度 */
    g_motecData.oilTemp = ((motecFrameBuf[MOTEC_FRAME_INDEX_OIL_TEMP][6] << 8) |
                           motecFrameBuf[MOTEC_FRAME_INDEX_OIL_TEMP][7]) / 10;

    /* 帧4: 机油压力 */
    g_motecData.oilPressure = ((motecFrameBuf[MOTEC_FRAME_INDEX_OIL_PRESS][0] << 8) |
                               motecFrameBuf[MOTEC_FRAME_INDEX_OIL_PRESS][1]) / 10;

    /* 帧6: 低压电池电压 + ECU温度 */
    g_motecData.lowBatVol = ((motecFrameBuf[MOTEC_FRAME_INDEX_BAT_VOL][4] << 8) |
                             motecFrameBuf[MOTEC_FRAME_INDEX_BAT_VOL][5]) / 10;
    g_motecData.ecuTemp = ((motecFrameBuf[MOTEC_FRAME_INDEX_BAT_VOL][6] << 8) |
                           motecFrameBuf[MOTEC_FRAME_INDEX_BAT_VOL][7]) / 10;

    /* 帧14: 档位 */
    g_motecData.gear = (motecFrameBuf[MOTEC_FRAME_INDEX_GEAR][0] << 8) |
                        motecFrameBuf[MOTEC_FRAME_INDEX_GEAR][1];

    /* 计算车速 */
    g_motecData.frontSpeed = calculateSpeed(g_motecData.engineRPM, g_motecData.gear);
}

/* Exported functions -------------------------------------------------------*/

/**
 * @brief 初始化MoTeC模块
 */
void APP_Motec_Init(void)
{
    memset(&g_motecData, 0, sizeof(MotecData_t));
    memset(motecFrameBuf, 0, sizeof(motecFrameBuf));
    frameCounter = 0;
    frameStartFlag = 0;
}

/**
 * @brief 处理MoTeC CAN帧（每收到一帧0x5F0调用一次）
 * @param data: 8字节帧数据
 */
void APP_Motec_ProcessFrame(uint8_t *data)
{
    /* 检测帧头 */
    if (isFrameHeader(data))
    {
        frameCounter = 0;
        frameStartFlag = 1;
    }

    /* 存储帧数据 */
    if (frameStartFlag && frameCounter < MOTEC_FRAME_COUNT)
    {
        memcpy(motecFrameBuf[frameCounter], data, MOTEC_FRAME_SIZE);
        frameCounter++;
    }

    /* 18帧收满，解析 */
    if (frameCounter >= MOTEC_FRAME_COUNT)
    {
        if (isFrameEndValid())
        {
            motec_decode();
        }

        /* 清空缓冲区，准备下一组 */
        memset(motecFrameBuf, 0, sizeof(motecFrameBuf));
        frameCounter = 0;
        frameStartFlag = 0;
    }
}

/**
 * @brief 获取当前车型
 * @return 0=电车, 1=油车
 */
uint8_t APP_Motec_GetCarType(void)
{
    return g_motecData.carType;
}
