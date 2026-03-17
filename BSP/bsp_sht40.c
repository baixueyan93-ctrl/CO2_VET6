#include "bsp_sht40.h"
#include "FreeRTOS.h"
#include "task.h"

extern I2C_HandleTypeDef hi2c1;  /* 与 EEPROM 共用 I2C1 (PB8-SCL, PB9-SDA) */

/**
 * @brief SHT40 CRC-8 校验 (多项式 0x31, 初始值 0xFF)
 *        与 SHT30 算法完全相同
 */
uint8_t BSP_SHT40_CRC(uint8_t *data, uint8_t len) {
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x31;
            else
                crc = crc << 1;
        }
    }
    return crc;
}

/**
 * @brief 软复位 SHT40
 *        注意: SHT40 用单字节命令, 不同于 SHT30 的双字节
 */
bool BSP_SHT40_SoftReset(void) {
    uint8_t cmd = SHT40_CMD_SOFT_RESET;
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(&hi2c1, SHT40_ADDRESS, &cmd, 1, 100);
    vTaskDelay(pdMS_TO_TICKS(2));
    return (ret == HAL_OK);
}

/**
 * @brief 初始化 SHT40 (软复位 + 验证通信)
 */
bool BSP_SHT40_Init(void) {
    if (!BSP_SHT40_SoftReset()) {
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(10));

    /* 尝试读一次, 验证芯片在线 */
    SHT40_Result_t test;
    return BSP_SHT40_Read(&test);
}

/**
 * @brief 高精度单次测量, 读取温湿度
 * @param result  输出温湿度结果
 * @return true=成功, false=通信失败或CRC错误
 */
bool BSP_SHT40_Read(SHT40_Result_t *result) {
    uint8_t cmd = SHT40_CMD_MEAS_HIGH;
    uint8_t buf[6];

    /* 1. 发送单字节测量命令 */
    if (HAL_I2C_Master_Transmit(&hi2c1, SHT40_ADDRESS, &cmd, 1, 100) != HAL_OK) {
        return false;
    }

    /* 2. 等待测量完成 (高精度模式最大 8.2ms, 留余量给 10ms) */
    vTaskDelay(pdMS_TO_TICKS(10));

    /* 3. 读取 6 字节: [温度MSB][温度LSB][CRC] [湿度MSB][湿度LSB][CRC] */
    if (HAL_I2C_Master_Receive(&hi2c1, SHT40_ADDRESS, buf, 6, 100) != HAL_OK) {
        return false;
    }

    /* 4. 校验温度 CRC */
    if (BSP_SHT40_CRC(buf, 2) != buf[2]) {
        return false;
    }

    /* 5. 校验湿度 CRC */
    if (BSP_SHT40_CRC(buf + 3, 2) != buf[5]) {
        return false;
    }

    /* 6. 计算温度: T = -45 + 175 * raw / 65535 */
    uint16_t raw_temp = ((uint16_t)buf[0] << 8) | buf[1];
    result->temperature = -45.0f + 175.0f * ((float)raw_temp / 65535.0f);

    /* 7. 计算湿度: RH = -6 + 125 * raw / 65535, 然后钳位到 0~100 */
    uint16_t raw_humi = ((uint16_t)buf[3] << 8) | buf[4];
    result->humidity = -6.0f + 125.0f * ((float)raw_humi / 65535.0f);
    if (result->humidity > 100.0f) result->humidity = 100.0f;
    if (result->humidity < 0.0f)   result->humidity = 0.0f;

    return true;
}
