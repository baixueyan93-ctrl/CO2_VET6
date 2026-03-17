#ifndef __BSP_SHT40_H
#define __BSP_SHT40_H

#include "main.h"
#include <stdbool.h>

/* =====================================================
 * SHT40 温湿度传感器驱动 (I2C)
 *
 * 与 SHT30 的区别:
 *   - SHT40 用单字节命令 (SHT30 是双字节)
 *   - SHT40 命令字完全不同
 *   - 数据格式相同: 6字节 [T_MSB][T_LSB][CRC][H_MSB][H_LSB][CRC]
 *   - CRC 算法相同: 多项式 0x31, 初始值 0xFF
 * ===================================================== */

/* SHT40 I2C 地址 (固定, 无 ADDR 引脚选择) */
#define SHT40_ADDRESS       (0x44 << 1)  /* 0x88 */

/* SHT40 单字节命令 */
#define SHT40_CMD_MEAS_HIGH     0xFD  /* 高精度测量 (等待 ~8.2ms)  */
#define SHT40_CMD_MEAS_MED      0xF6  /* 中精度测量 (等待 ~4.5ms)  */
#define SHT40_CMD_MEAS_LOW      0xE0  /* 低精度测量 (等待 ~1.7ms)  */
#define SHT40_CMD_SERIAL        0x89  /* 读序列号                   */
#define SHT40_CMD_SOFT_RESET    0x94  /* 软复位                     */

/* 测量结果结构体 */
typedef struct {
    float temperature;  /* 温度 (°C) */
    float humidity;     /* 相对湿度 (% RH) */
} SHT40_Result_t;

/* 接口函数 */
bool    BSP_SHT40_Init(void);
bool    BSP_SHT40_Read(SHT40_Result_t *result);
bool    BSP_SHT40_SoftReset(void);
uint8_t BSP_SHT40_CRC(uint8_t *data, uint8_t len);

#endif /* __BSP_SHT40_H */
