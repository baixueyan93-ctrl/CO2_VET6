#include "task_sht30.h"
#include "FreeRTOS.h"
#include "task.h"
#include "sys_state.h"

/**
 * @brief SHT30 环境温湿度采集任务
 *        周期: 1秒采集一次, 写入系统全局数据结构
 */
void Task_SHT30_Process(void const *argument) {
    vTaskDelay(pdMS_TO_TICKS(500));

    while (!BSP_SHT30_Init()) {
        vTaskDelay(pdMS_TO_TICKS(3000));
    }

    SHT30_Result_t sht30_data;

    for (;;) {
        if (BSP_SHT30_Read(&sht30_data)) {
            SysVarData_t sensor_snapshot;
            SysState_GetSensor(&sensor_snapshot);

            sensor_snapshot.VAR_SHT30_TEMP = sht30_data.temperature;
            sensor_snapshot.VAR_SHT30_HUMI = sht30_data.humidity;

            SysState_UpdateSensor(&sensor_snapshot);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
