/**
 * @file bsp_w25q128.h
 * @brief W25Q128 Flash驱动（16MB SPI Flash）
 * @version 2.0.0
 * @date 2025-01-01
 *
 * 特性：
 * - 自动判断擦除：写入前检测是否需要擦除（NAND Flash只能1→0）
 * - 读-改-写：部分写入时自动读出整个扇区，修改后写回
 * - 写入校验：写完回读校验，失败自动重试3次
 * - 跨扇区写入：自动处理跨扇区边界的写入
 */

#ifndef __BSP_W25Q128_H
#define __BSP_W25Q128_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include "spi.h"

/* Exported defines ----------------------------------------------------------*/
#define W25Q128_PAGE_SIZE       256         /* 页大小 */
#define W25Q128_SECTOR_SIZE     4096        /* 扇区大小（4KB） */
#define W25Q128_BLOCK_SIZE      65536       /* 块大小（64KB） */
#define W25Q128_FLASH_SIZE      16777216    /* 总容量（16MB） */

/* Exported functions --------------------------------------------------------*/

/**
 * @brief 初始化W25Q128
 */
void BSP_W25Q128_Init(void);

/**
 * @brief 读取JEDEC ID
 * @param id 3字节ID数组（制造商ID, 类型, 容量）
 * @return 1=W25Q128确认（0xEF4018），0=未知芯片
 */
uint8_t BSP_W25Q128_ReadID(uint8_t *id);

/**
 * @brief 读取数据
 * @param addr 起始地址（0 ~ 16M-1）
 * @param data 数据缓冲区
 * @param len  数据长度
 */
void BSP_W25Q128_Read(uint32_t addr, uint8_t *data, uint32_t len);

/**
 * @brief 智能写入数据（自动擦除+校验）
 * @param addr 起始地址
 * @param data 数据
 * @param len  数据长度
 * @note  调用者无需手动擦除，驱动自动处理：
 *        1. 回读比较，数据相同则跳过
 *        2. 判断是否需要擦除
 *        3. 读-改-写整个扇区
 *        4. 写入后回读校验，失败重试3次
 */
void BSP_W25Q128_Write(uint32_t addr, uint8_t *data, uint32_t len);

/**
 * @brief 擦除扇区（4KB）
 * @param sector 扇区编号（0 ~ 4095）
 * @note  擦除时间约45ms
 */
void BSP_W25Q128_EraseSector(uint32_t sector);

/**
 * @brief 擦除块（64KB）
 * @param block 块编号（0 ~ 255）
 * @note  擦除时间约150ms
 */
void BSP_W25Q128_EraseBlock(uint32_t block);

/**
 * @brief 擦除整片
 * @note  擦除时间约100秒，慎用
 */
void BSP_W25Q128_EraseChip(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_W25Q128_H */
