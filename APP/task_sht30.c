#include "task_sht30.h"
#include "FreeRTOS.h"
#include "task.h"
#include "sys_state.h"
#include <stdio.h>

/**
 * @brief SHT30 温湿度采集任务
 *        周期: 1秒采集一次, 写入系统全局数据结构
 */
void Task_SHT30_Process(void const *argument) {
    /* 等待 I2C 总线和系统就绪 */
    vTaskDelay(pdMS_TO_TICKS(500));

    /* 初始化 SHT30 */
    if (!BSP_SHT30_Init()) {
        /* 初始化失败, 标记传感器故障, 但不退出任务(会持续重试) */
        for (;;) {
            vTaskDelay(pdMS_TO_TICKS(3000));
            if (BSP_SHT30_Init()) break;  /* 重试成功则跳出 */
        }
    }

    SHT30_Result_t sht30_data;

    for (;;) {
        if (BSP_SHT30_Read(&sht30_data)) {
            /* 读取成功: 写入系统全局数据 */
            SysVarData_t sensor_snapshot;
            SysState_GetSensor(&sensor_snapshot);

            sensor_snapshot.VAR_SHT30_TEMP = sht30_data.temperature;
            sensor_snapshot.VAR_SHT30_HUMI = sht30_data.humidity;

            SysState_UpdateSensor(&sensor_snapshot);
        }
        /* 读取失败则保持上次数据, 下个周期重试 */

        vTaskDelay(pdMS_TO_TICKS(1000));  /* 1秒采集一次 */
    }
}
