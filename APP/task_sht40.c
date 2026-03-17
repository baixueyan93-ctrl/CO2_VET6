#include "task_sht40.h"
#include "FreeRTOS.h"
#include "task.h"
#include "sys_state.h"

/**
 * @brief SHT40 环境温湿度采集任务
 *        周期: 1秒采集一次, 写入系统全局数据结构
 */
void Task_SHT40_Process(void const *argument) {
    vTaskDelay(pdMS_TO_TICKS(500));

    /* 初始化 SHT40, 失败则持续重试 */
    while (!BSP_SHT40_Init()) {
        vTaskDelay(pdMS_TO_TICKS(3000));
    }

    SHT40_Result_t sht40_data;

    for (;;) {
        if (BSP_SHT40_Read(&sht40_data)) {
            SysVarData_t sensor_snapshot;
            SysState_GetSensor(&sensor_snapshot);

            sensor_snapshot.VAR_SHT40_TEMP = sht40_data.temperature;
            sensor_snapshot.VAR_SHT40_HUMI = sht40_data.humidity;

            SysState_UpdateSensor(&sensor_snapshot);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
