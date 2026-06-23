/**
 * @file bsp_w25q128.c
 * @brief W25Q128 Flash驱动实现
 * @version 1.0.0
 * @date 2025-01-01
 */

#include "bsp_w25q128.h"
#include "spi.h"

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

/* Private variables --------------------------------------------------------*/
static SPI_HandleTypeDef *hspi = &hspi1;

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
 * @brief 等待Flash就绪
 */
static void W25Q128_WaitBusy(void)
{
    uint8_t status;
    CS_LOW();
    SPI_WriteByte(W25Q128_READ_STATUS_REG1);
    do {
        status = SPI_ReadByte();
    } while (status & 0x01);
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
 */
uint8_t BSP_W25Q128_ReadID(uint8_t *id)
{
    CS_LOW();
    SPI_WriteByte(W25Q128_JEDEC_ID);
    id[0] = SPI_ReadByte();  /* 制造商ID */
    id[1] = SPI_ReadByte();  /* 存储器类型 */
    id[2] = SPI_ReadByte();  /* 容量 */
    CS_HIGH();
    
    /* 验证是否为W25Q128 (0xEF, 0x40, 0x18) */
    if (id[0] == 0xEF && id[1] == 0x40 && id[2] == 0x18)
        return 1;
    return 0;
}

/**
 * @brief 读取数据
 * @param addr: 起始地址
 * @param data: 数据缓冲区
 * @param len: 数据长度
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
 * @brief 页编程（256字节）
 * @param addr: 起始地址（页对齐）
 * @param data: 数据
 * @param len: 数据长度（最大256）
 */
void BSP_W25Q128_WritePage(uint32_t addr, uint8_t *data, uint16_t len)
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
 * @brief 写入数据（自动处理跨页）
 * @param addr: 起始地址
 * @param data: 数据
 * @param len: 数据长度
 */
void BSP_W25Q128_Write(uint32_t addr, uint8_t *data, uint32_t len)
{
    uint16_t pageRemain = W25Q128_PAGE_SIZE - (addr % W25Q128_PAGE_SIZE);
    
    if (len <= pageRemain)
    {
        BSP_W25Q128_WritePage(addr, data, len);
        return;
    }
    
    /* 写入第一页 */
    BSP_W25Q128_WritePage(addr, data, pageRemain);
    addr += pageRemain;
    data += pageRemain;
    len -= pageRemain;
    
    /* 写入完整页 */
    while (len >= W25Q128_PAGE_SIZE)
    {
        BSP_W25Q128_WritePage(addr, data, W25Q128_PAGE_SIZE);
        addr += W25Q128_PAGE_SIZE;
        data += W25Q128_PAGE_SIZE;
        len -= W25Q128_PAGE_SIZE;
    }
    
    /* 写入剩余数据 */
    if (len > 0)
    {
        BSP_W25Q128_WritePage(addr, data, len);
    }
}

/**
 * @brief 擦除扇区（4KB）
 * @param sector: 扇区编号
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
 * @param block: 块编号
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
 * @brief 擦除整片
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
