/**
 * @file app_iap.c
 * @brief IAP在线升级实现
 * @version 1.0.0
 * @date 2025-01-01
 */

#include "app_iap.h"
#include <string.h>

/* Private variables ---------------------------------------------------------*/
static IAP_Info_t iapInfo;
static uint8_t iapBuffer[IAP_BUFFER_SIZE];
static uint32_t iapBufferIndex = 0;
static uint32_t flashWriteAddr = IAP_APP_ADDR;

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 解锁Flash
 */
static HAL_StatusTypeDef flashUnlock(void)
{
    return HAL_FLASH_Unlock();
}

/**
 * @brief 锁定Flash
 */
static HAL_StatusTypeDef flashLock(void)
{
    return HAL_FLASH_Lock();
}

/**
 * @brief 擦除Flash扇区
 */
static HAL_StatusTypeDef flashErase(uint32_t sector)
{
    FLASH_EraseInitTypeDef eraseInit;
    uint32_t sectorError;
    
    eraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
    eraseInit.Sector = sector;
    eraseInit.NbSectors = 1;
    eraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    
    return HAL_FLASHEx_Erase(&eraseInit, &sectorError);
}

/**
 * @brief 写入Flash（256位对齐）+ 写入校验重试
 *        STM32H7的FLASH_TYPEPROGRAM_FLASHWORD要求32字节对齐
 *        data必须是32字节对齐的缓冲区，len必须是32的倍数
 */
static HAL_StatusTypeDef flashWrite(uint32_t addr, uint8_t *data, uint32_t len)
{
    HAL_StatusTypeDef status = HAL_OK;
    
    for (uint32_t i = 0; i < len; i += 32)
    {
        uint32_t retry = 0;
        while (retry < IAP_WRITE_RETRY)
        {
            status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, addr + i, (uint64_t)(uint32_t)(data + i));
            if (status == HAL_OK)
            {
                /* 写入校验：回读比较 */
                if (memcmp((void *)(addr + i), data + i, 32) == 0)
                {
                    break;  /* 校验通过 */
                }
            }
            retry++;
        }
        if (retry >= IAP_WRITE_RETRY) return HAL_ERROR;
    }
    
    return HAL_OK;
}

/* Exported functions --------------------------------------------------------*/

/**
 * @brief 初始化IAP
 */
void APP_IAP_Init(void)
{
    memset(&iapInfo, 0, sizeof(IAP_Info_t));
    memset(iapBuffer, 0, IAP_BUFFER_SIZE);
    iapBufferIndex = 0;
    flashWriteAddr = IAP_APP_ADDR;
    iapInfo.state = IAP_STATE_IDLE;
}

/**
 * @brief 处理IAP（在主循环中调用）
 */
void APP_IAP_Process(void)
{
    /* 可以在这里处理超时等逻辑 */
}

/**
 * @brief 开始IAP升级
 * @param size: 固件大小
 * @warning 擦除Sector 0(当前运行代码区)，传输失败将变砖(仅JTAG恢复)
 *          已加重试+回读校验降低风险，建议后续改为双bank方案
 */
void APP_IAP_Start(uint32_t size)
{
    iapInfo.firmwareSize = size;
    iapInfo.receivedSize = 0;
    iapInfo.checksum = 0;
    iapInfo.state = IAP_STATE_RECEIVING;
    iapBufferIndex = 0;
    flashWriteAddr = IAP_APP_ADDR;
    
    /* 解锁Flash并擦除 */
    flashUnlock();
    flashErase(IAP_FLASH_SECTOR);
}

/**
 * @brief 写入数据
 * @param data: 数据指针
 * @param len: 数据长度
 */
void APP_IAP_WriteData(uint8_t *data, uint32_t len)
{
    if (iapInfo.state != IAP_STATE_RECEIVING) return;
    
    for (uint32_t i = 0; i < len; i++)
    {
        iapBuffer[iapBufferIndex++] = data[i];
        iapInfo.receivedSize++;
        iapInfo.checksum += data[i];
        
        /* 缓冲区满，写入Flash */
        if (iapBufferIndex >= IAP_BUFFER_SIZE)
        {
            /* 填充到256位对齐 */
            while (iapBufferIndex % 32 != 0)
            {
                iapBuffer[iapBufferIndex++] = 0xFF;
            }
            
            flashWrite(flashWriteAddr, iapBuffer, iapBufferIndex);
            flashWriteAddr += iapBufferIndex;
            iapBufferIndex = 0;
        }
    }
    
    /* 检查是否接收完成 */
    if (iapInfo.receivedSize >= iapInfo.firmwareSize)
    {
        APP_IAP_Finish();
    }
}

/**
 * @brief 完成IAP升级
 */
void APP_IAP_Finish(void)
{
    /* 写入剩余数据 */
    if (iapBufferIndex > 0)
    {
        /* 填充到256位对齐 */
        while (iapBufferIndex % 32 != 0)
        {
            iapBuffer[iapBufferIndex++] = 0xFF;
        }
        flashWrite(flashWriteAddr, iapBuffer, iapBufferIndex);
    }
    
    /* 锁定Flash */
    flashLock();
    
    iapInfo.state = IAP_STATE_COMPLETE;
}

/**
 * @brief 跳转到BootLoader
 */
void APP_IAP_JumpToBootloader(void)  /* SW-L10修复: 统一命名 */
{
    typedef void (*pFunction)(void);
    pFunction jumpToApplication;
    uint32_t jumpAddress;
    
    /* 关闭所有中断 */
    __disable_irq();
    
    /* 关闭所有外设 */
    HAL_DeInit();
    
    /* 获取BootLoader地址 */
    jumpAddress = *(__IO uint32_t *)(IAP_BOOTLOADER_ADDR + 4);
    jumpToApplication = (pFunction)jumpAddress;
    
    /* 设置主堆栈指针 */
    __set_MSP(*(__IO uint32_t *)IAP_BOOTLOADER_ADDR);
    
    /* 跳转 */
    jumpToApplication();
}

/**
 * @brief 获取IAP状态
 */
IAP_State_t APP_IAP_GetState(void)
{
    return iapInfo.state;
}

/**
 * @brief 获取IAP信息
 */
IAP_Info_t APP_IAP_GetInfo(void)
{
    return iapInfo;
}
