#include "task_oil_heater.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "sys_state.h"
#include "sys_config.h"
#include "bsp_relay.h"

/* ===========================================================================
 * 逻辑3: 油壳加热逻辑
 *
 * 判断优先级:
 *   压缩机运行 → 油壳加热关 (运行中不需要预热)
 *   压缩机停机 + 环境温度 ≤ 10°C → 油壳加热开 (防止冷凝液化)
 *   压缩机停机 + 环境温度 > 10°C  → 油壳加热关
 *
 * 周期: 每 500ms 执行一次
 * =========================================================================== */

void Task_OilHeater_Process(void const *argument)
{
    (void)argument;
    SysVarData_t sensor;

    for (;;) {
        /* 读取压缩机运行状态 */
        EventBits_t sys_bits = xEventGroupGetBits(SysEventGroup);
        bool comp_running = (sys_bits & ST_COMP_RUNNING) != 0;

        if (comp_running) {
            /* =============================================
             * 压缩机开机 → 油壳加热关
             * ============================================= */
            BSP_OilHeater_Off();
            xEventGroupClearBits(SysEventGroup, ST_OIL_HEAT_ON);

        } else {
            /* =============================================
             * 压缩机未开机 → 判断环境温度
             * ============================================= */
            SysState_GetSensor(&sensor);

            /* 优先用 SHT30 环境温度, 若无则用 VAR_AMBIENT_TEMP */
            float ambient = sensor.VAR_SHT30_TEMP;

            if (ambient <= SET_OIL_HEAT_TEMP) {
                /* 环境温度 ≤ 10°C → 油壳加热开 */
                BSP_OilHeater_On();
                xEventGroupSetBits(SysEventGroup, ST_OIL_HEAT_ON);
            } else {
                /* 环境温度 > 10°C → 油壳加热关 */
                BSP_OilHeater_Off();
                xEventGroupClearBits(SysEventGroup, ST_OIL_HEAT_ON);
            }
        }

        /* 每 500ms 检查一次 */
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
