/**
 * @file app_iap.h
 * @brief IAP在线升级
 * @version 1.0.0
 * @date 2025-01-01
 */

#ifndef __APP_IAP_H
#define __APP_IAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

/* Exported defines ----------------------------------------------------------*/
#define IAP_BOOTLOADER_ADDR     0x1FF09800  /* System BootLoader地址 */
#define IAP_APP_ADDR            0x08000000  /* 应用程序起始地址（Sector 0） */
#define IAP_FLASH_SECTOR        FLASH_SECTOR_0
#define IAP_BUFFER_SIZE         1024
#define IAP_WRITE_RETRY         3           /* 写入重试次数 */

/* Exported types ------------------------------------------------------------*/
typedef enum {
    IAP_STATE_IDLE = 0,         /* 空闲状态 */
    IAP_STATE_RECEIVING,        /* 接收数据中 */
    IAP_STATE_COMPLETE,         /* 升级完成 */
    IAP_STATE_ERROR             /* 升级错误 */
} IAP_State_t;

typedef struct {
    uint32_t firmwareSize;      /* 固件大小 */
    uint32_t receivedSize;      /* 已接收大小 */
    uint32_t checksum;          /* 校验和 */
    IAP_State_t state;          /* 状态 */
} IAP_Info_t;

/* Exported functions --------------------------------------------------------*/
void APP_IAP_Init(void);
void APP_IAP_Process(void);
void APP_IAP_Start(uint32_t size);
void APP_IAP_WriteData(uint8_t *data, uint32_t len);
void APP_IAP_Finish(void);
void APP_IAP_JumpToBootloader(void);  /* SW-L10修复: 统一命名 */
IAP_State_t APP_IAP_GetState(void);
IAP_Info_t APP_IAP_GetInfo(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_IAP_H */
