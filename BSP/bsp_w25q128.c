/**
 * @file bsp_w25q128.c
 * @brief W25Q128 Flash驱动实现
 * @version 2.0.0
 * @date 2025-01-01
 *
 * 移植自MeterBox项目的W25Q16驱动，主要改进：
 * - 自动判断是否需要擦除（NAND Flash只能1→0，不能0→1）
 * - 写入前自动读-改-写整个扇区
 * - 写入后回读校验，失败自动重试3次
 */

#include "bsp_w25q128.h"
#include "spi.h"
#include <string.h>

/* Private defines ----------------------------------------------------------*/
#define W25Q128_WRITE_ENABLE        0x06
#define W25Q128_WRITE_DISABLE       0x04
#define W25Q128_READ_STATUS_REG1    0x05
#define W25Q128_READ_DATA           0x03
#define W25Q128_PAGE_PROGRAM        0x02
#define W25Q128_SECTOR_ERASE        0x20
#define W25Q128_BLOCK_ERASE         0xD8
#define W25Q128_CHIP_ERASE          0xC7
#define W25Q128_JEDEC_ID            0x9F
#define W25Q128_WIP_FLAG            0x01  /* 状态寄存器忙标志位 */

/* Private variables --------------------------------------------------------*/
static SPI_HandleTypeDef *hspi = &hspi1;

/* 扇区读写缓冲区（用于读-改-写操作） */
static uint8_t sectorBuf[W25Q128_SECTOR_SIZE];

/* Private function prototypes -----------------------------------------------*/
static void BSP_W25Q128_WritePage(uint32_t addr, uint8_t *data, uint16_t len);

/* Private functions --------------------------------------------------------*/

/**
 * @brief 片选使能
 */
static void CS_LOW(void)
{
    HAL_GPIO_WritePin(SPI_FLASH_CS_GPIO_Port, SPI_FLASH_CS_Pin, GPIO_PIN_RESET);
}

/**
 * @brief 片选失能
 */
static void CS_HIGH(void)
{
    HAL_GPIO_WritePin(SPI_FLASH_CS_GPIO_Port, SPI_FLASH_CS_Pin, GPIO_PIN_SET);
}

/**
 * @brief 发送单字节
 */
static void SPI_WriteByte(uint8_t data)
{
    HAL_SPI_Transmit(hspi, &data, 1, HAL_MAX_DELAY);
}

/**
 * @brief 接收单字节
 */
static uint8_t SPI_ReadByte(void)
{
    uint8_t data;
    HAL_SPI_Receive(hspi, &data, 1, HAL_MAX_DELAY);
    return data;
}

/**
 * @brief 等待Flash内部操作完成
 * @note  循环读取状态寄存器WIP位，直到Flash空闲
 */
static void W25Q128_WaitBusy(void)
{
    uint8_t status;
    CS_LOW();
    SPI_WriteByte(W25Q128_READ_STATUS_REG1);
    do {
        status = SPI_ReadByte();
    } while (status & W25Q128_WIP_FLAG);
    CS_HIGH();
}

/**
 * @brief 写使能
 */
static void W25Q128_WriteEnable(void)
{
    CS_LOW();
    SPI_WriteByte(W25Q128_WRITE_ENABLE);
    CS_HIGH();
}

/**
 * @brief 判断写入前是否需要擦除
 * @param oldBuf  Flash中原有的数据
 * @param newBuf  要写入的新数据
 * @param len     数据长度
 * @return 1=需要擦除，0=不需要擦除
 *
 * 原理：NAND Flash只能1→0，不能0→1
 * 如果新数据需要将oldBuf中的0位变为1，则必须先擦除（全变1）再写入
 * 算法：~old & new != 0 表示有0→1的情况，需要擦除
 */
static uint8_t W25Q128_NeedErase(uint8_t *oldBuf, uint8_t *newBuf, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        uint8_t ucOld = ~oldBuf[i];
        if ((ucOld & newBuf[i]) != 0)
        {
            return 1;  /* 有0→1的情况，需要擦除 */
        }
    }
    return 0;
}

/**
 * @brief 回读校验数据
 * @param addr  Flash地址
 * @param data  期望的数据
 * @param len   数据长度
 * @return 0=一致，1=不一致
 */
static uint8_t W25Q128_CmpData(uint32_t addr, uint8_t *data, uint32_t len)
{
    uint8_t tmp;
    CS_LOW();
    SPI_WriteByte(W25Q128_READ_DATA);
    SPI_WriteByte((addr >> 16) & 0xFF);
    SPI_WriteByte((addr >> 8) & 0xFF);
    SPI_WriteByte(addr & 0xFF);

    for (uint32_t i = 0; i < len; i++)
    {
        tmp = SPI_ReadByte();
        if (tmp != data[i])
        {
            CS_HIGH();
            return 1;  /* 数据不一致 */
        }
    }
    CS_HIGH();
    return 0;
}

/**
 * @brief 智能写入一个扇区（4KB以内）
 * @param src      源数据
 * @param addr     目标起始地址
 * @param len      数据长度（不能超过扇区大小）
 * @return 1=成功，0=失败
 *
 * 流程（移植自MeterBox）：
 * 1. 先回读目标区域，如果数据相同则跳过写入
 * 2. 判断是否需要擦除（有0→1的情况）
 * 3. 如果需要擦除：读出整个扇区→修改数据→擦除扇区→写回
 * 4. 如果不需要擦除：直接写入
 * 5. 回读校验，失败最多重试3次
 */
static uint8_t W25Q128_AutoWriteSector(uint8_t *src, uint32_t addr, uint16_t len)
{
    uint32_t sectorAddr;
    uint8_t needErase;
    uint8_t ret;

    if (len == 0) return 1;
    if (len > W25Q128_SECTOR_SIZE) return 0;

    /* 1. 回读比较，数据相同则跳过 */
    if (W25Q128_CmpData(addr, src, len) == 0)
    {
        return 1;
    }

    /* 2. 判断是否需要擦除 */
    BSP_W25Q128_Read(addr, sectorBuf, len);
    needErase = W25Q128_NeedErase(sectorBuf, src, len);

    /* 3. 计算扇区起始地址 */
    sectorAddr = addr & (~(W25Q128_SECTOR_SIZE - 1));

    if (len == W25Q128_SECTOR_SIZE)
    {
        /* 整个扇区写入，直接用源数据 */
        memcpy(sectorBuf, src, W25Q128_SECTOR_SIZE);
    }
    else
    {
        /* 部分写入：读出整个扇区，只修改目标区域 */
        BSP_W25Q128_Read(sectorAddr, sectorBuf, W25Q128_SECTOR_SIZE);
        uint16_t offset = addr & (W25Q128_SECTOR_SIZE - 1);
        memcpy(&sectorBuf[offset], src, len);
    }

    /* 4. 擦除+写入+校验，最多重试3次 */
    ret = 0;
    for (uint8_t retry = 0; retry < 3; retry++)
    {
        if (needErase)
        {
            BSP_W25Q128_EraseSector(sectorAddr / W25Q128_SECTOR_SIZE);
        }

        /* 按页写入整个扇区 */
        for (uint32_t page = 0; page < W25Q128_SECTOR_SIZE / W25Q128_PAGE_SIZE; page++)
        {
            BSP_W25Q128_WritePage(
                sectorAddr + page * W25Q128_PAGE_SIZE,
                &sectorBuf[page * W25Q128_PAGE_SIZE],
                W25Q128_PAGE_SIZE
            );
        }

        /* 回读校验目标区域 */
        if (W25Q128_CmpData(addr, src, len) == 0)
        {
            ret = 1;
            break;
        }
    }

    return ret;
}

/* Exported functions -------------------------------------------------------*/

/**
 * @brief 初始化W25Q128
 */
void BSP_W25Q128_Init(void)
{
    CS_HIGH();
    HAL_Delay(100);
}

/**
 * @brief 读取JEDEC ID
 * @param id: 3字节ID数组
 * @return 1=W25Q128确认，0=未知芯片
 */
uint8_t BSP_W25Q128_ReadID(uint8_t *id)
{
    CS_LOW();
    SPI_WriteByte(W25Q128_JEDEC_ID);
    id[0] = SPI_ReadByte();  /* 制造商ID: 0xEF */
    id[1] = SPI_ReadByte();  /* 存储器类型: 0x40 */
    id[2] = SPI_ReadByte();  /* 容量: 0x18 */
    CS_HIGH();

    if (id[0] == 0xEF && id[1] == 0x40 && id[2] == 0x18)
        return 1;
    return 0;
}

/**
 * @brief 读取数据
 * @param addr 起始地址
 * @param data 数据缓冲区
 * @param len  数据长度
 */
void BSP_W25Q128_Read(uint32_t addr, uint8_t *data, uint32_t len)
{
    CS_LOW();
    SPI_WriteByte(W25Q128_READ_DATA);
    SPI_WriteByte((addr >> 16) & 0xFF);
    SPI_WriteByte((addr >> 8) & 0xFF);
    SPI_WriteByte(addr & 0xFF);

    for (uint32_t i = 0; i < len; i++)
    {
        data[i] = SPI_ReadByte();
    }
    CS_HIGH();
}

/**
 * @brief 页编程（256字节，内部函数）
 * @param addr 起始地址（页对齐）
 * @param data 数据
 * @param len  数据长度（最大256）
 */
static void BSP_W25Q128_WritePage(uint32_t addr, uint8_t *data, uint16_t len)
{
    W25Q128_WriteEnable();
    W25Q128_WaitBusy();

    CS_LOW();
    SPI_WriteByte(W25Q128_PAGE_PROGRAM);
    SPI_WriteByte((addr >> 16) & 0xFF);
    SPI_WriteByte((addr >> 8) & 0xFF);
    SPI_WriteByte(addr & 0xFF);

    for (uint16_t i = 0; i < len; i++)
    {
        SPI_WriteByte(data[i]);
    }
    CS_HIGH();

    W25Q128_WaitBusy();
}

/**
 * @brief 智能写入数据（自动处理擦除和跨扇区）
 * @param addr 起始地址
 * @param data 数据
 * @param len  数据长度
 *
 * 与旧版BSP_W25Q128_Write的区别：
 * - 自动判断是否需要擦除
 * - 自动读-改-写整个扇区
 * - 写入后自动校验，失败重试3次
 * - 调用者无需手动先擦除
 */
void BSP_W25Q128_Write(uint32_t addr, uint8_t *data, uint32_t len)
{
    uint16_t sectorRemain = W25Q128_SECTOR_SIZE - (addr % W25Q128_SECTOR_SIZE);

    if (len <= sectorRemain)
    {
        W25Q128_AutoWriteSector(data, addr, len);
        return;
    }

    /* 写入第一个扇区的剩余部分 */
    W25Q128_AutoWriteSector(data, addr, sectorRemain);
    addr += sectorRemain;
    data += sectorRemain;
    len -= sectorRemain;

    /* 写入完整扇区 */
    while (len >= W25Q128_SECTOR_SIZE)
    {
        W25Q128_AutoWriteSector(data, addr, W25Q128_SECTOR_SIZE);
        addr += W25Q128_SECTOR_SIZE;
        data += W25Q128_SECTOR_SIZE;
        len -= W25Q128_SECTOR_SIZE;
    }

    /* 写入剩余数据 */
    if (len > 0)
    {
        W25Q128_AutoWriteSector(data, addr, len);
    }
}

/**
 * @brief 擦除扇区（4KB）
 * @param sector 扇区编号
 */
void BSP_W25Q128_EraseSector(uint32_t sector)
{
    uint32_t addr = sector * W25Q128_SECTOR_SIZE;

    W25Q128_WriteEnable();
    W25Q128_WaitBusy();

    CS_LOW();
    SPI_WriteByte(W25Q128_SECTOR_ERASE);
    SPI_WriteByte((addr >> 16) & 0xFF);
    SPI_WriteByte((addr >> 8) & 0xFF);
    SPI_WriteByte(addr & 0xFF);
    CS_HIGH();

    W25Q128_WaitBusy();
}

/**
 * @brief 擦除块（64KB）
 * @param block 块编号
 */
void BSP_W25Q128_EraseBlock(uint32_t block)
{
    uint32_t addr = block * W25Q128_BLOCK_SIZE;

    W25Q128_WriteEnable();
    W25Q128_WaitBusy();

    CS_LOW();
    SPI_WriteByte(W25Q128_BLOCK_ERASE);
    SPI_WriteByte((addr >> 16) & 0xFF);
    SPI_WriteByte((addr >> 8) & 0xFF);
    SPI_WriteByte(addr & 0xFF);
    CS_HIGH();

    W25Q128_WaitBusy();
}

/**
 * @brief 擦除整片（约100秒）
 */
void BSP_W25Q128_EraseChip(void)
{
    W25Q128_WriteEnable();
    W25Q128_WaitBusy();

    CS_LOW();
    SPI_WriteByte(W25Q128_CHIP_ERASE);
    CS_HIGH();

    W25Q128_WaitBusy();
}
