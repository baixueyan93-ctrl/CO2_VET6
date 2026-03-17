#ifndef __BSP_SHT30_H
#define __BSP_SHT30_H

#include "main.h"
#include <stdbool.h>

/* SHT30 I2C 地址 (ADDR引脚接GND = 0x44, 接VDD = 0x45) */
#define SHT30_ADDR_LOW      (0x44 << 1)  /* 0x88 */
#define SHT30_ADDR_HIGH     (0x45 << 1)  /* 0x8A */
#define SHT30_ADDRESS       SHT30_ADDR_LOW  /* 默认 ADDR 接 GND */

/* SHT30 命令 (MSB first) */
#define SHT30_CMD_MEAS_HIGH_H   0x2C  /* 单次采集, 高重复性 */
#define SHT30_CMD_MEAS_HIGH_L   0x06
#define SHT30_CMD_SOFT_RESET_H  0x30  /* 软复位 */
#define SHT30_CMD_SOFT_RESET_L  0xA2
#define SHT30_CMD_STATUS_H      0xF3  /* 读状态寄存器 */
#define SHT30_CMD_STATUS_L      0x2D

/* 测量结果结构体 */
typedef struct {
    float temperature;  /* 温度 (°C) */
    float humidity;     /* 相对湿度 (% RH) */
} SHT30_Result_t;

/* 接口函数 */
bool    BSP_SHT30_Init(void);
bool    BSP_SHT30_Read(SHT30_Result_t *result);
bool    BSP_SHT30_SoftReset(void);
uint8_t BSP_SHT30_CRC(uint8_t *data, uint8_t len);

#endif /* __BSP_SHT30_H */
