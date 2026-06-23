/**
 * @file bsp_w25q128.h
 * @brief W25Q128 Flash驱动
 * @version 1.0.0
 * @date 2025-01-01
 */

#ifndef __BSP_W25Q128_H
#define __BSP_W25Q128_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include "spi.h"

/* Exported defines ----------------------------------------------------------*/
#define W25Q128_PAGE_SIZE       256
#define W25Q128_SECTOR_SIZE     4096
#define W25Q128_BLOCK_SIZE      65536
#define W25Q128_FLASH_SIZE      16777216  /* 16MB */

/* Exported functions --------------------------------------------------------*/
void BSP_W25Q128_Init(void);
uint8_t BSP_W25Q128_ReadID(uint8_t *id);
void BSP_W25Q128_Read(uint32_t addr, uint8_t *data, uint32_t len);
void BSP_W25Q128_Write(uint32_t addr, uint8_t *data, uint32_t len);
void BSP_W25Q128_EraseSector(uint32_t sector);
void BSP_W25Q128_EraseBlock(uint32_t block);
void BSP_W25Q128_EraseChip(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_W25Q128_H */
