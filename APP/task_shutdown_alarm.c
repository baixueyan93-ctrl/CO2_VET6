#include "task_shutdown_alarm.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "sys_state.h"
#include "sys_config.h"
#include "bsp_relay.h"

/* ===========================================================================
 * 逻辑1: 停机异常逻辑告警 (通知用户)
 *
 * 周期: 每 200ms 执行一次全部检查
 * 逻辑: 顺序检查 VDC → VAC → 变频器过流 → 变频器过热
 *        任一异常 → 置错误标志 + 停压缩机 + 通知用户
 *        异常恢复 → 清对应错误标志
 * =========================================================================== */

/* 内部: 检测到异常时执行关机动作 */
static void shutdown_action(void)
{
    /* 关压缩机 */
    BSP_Comp_Off();
    xEventGroupClearBits(SysEventGroup, ST_COMP_RUNNING);

    /* 置通知用户标志 */
    g_AlarmFlags |= WARN_NOTIFY_USER;
}

void Task_ShutdownAlarm_Process(void const *argument)
{
    (void)argument;
    SysVarData_t sensor;

    for (;;) {
        /* =============================================
         * 步骤1: 采集 VDC 电源电压, 判断是否 >= VDCmin
         * ============================================= */
        SysState_GetSensor(&sensor);

        if (sensor.VAR_VDC_VOLTAGE < SET_VDC_MIN) {
            /* VDC 欠压 → 置 EDC 错误 */
            g_AlarmFlags |= ERR_VDC_LOW;
            shutdown_action();
        } else {
            /* VDC 正常 → 清 EDC (-EDC) */
            g_AlarmFlags &= ~ERR_VDC_LOW;
        }

        /* =============================================
         * 步骤2: VAC 错/断相检查
         * ============================================= */
        if (BSP_VAC_IsPhaseFault()) {
            /* VAC 缺相 → 置 EAC 错误 */
            g_AlarmFlags |= ERR_VAC_PHASE;
            shutdown_action();
        } else {
            /* VAC 正常 → 清 EAC (-EAC) */
            g_AlarmFlags &= ~ERR_VAC_PHASE;
        }

        /* =============================================
         * 步骤3: 变频器过流检查
         * ============================================= */
        if (BSP_INV_IsOverCurrent()) {
            /* 过流 → 置 EFI 错误 */
            g_AlarmFlags |= ERR_INV_OVERCURR;
            shutdown_action();
        } else {
            /* 正常 → 清 EFI (-EFI) */
            g_AlarmFlags &= ~ERR_INV_OVERCURR;
        }

        /* =============================================
         * 步骤4: 变频器过热检查
         * ============================================= */
        if (BSP_INV_IsOverHeat()) {
            /* 过热 → 置 EFT 错误 */
            g_AlarmFlags |= ERR_INV_OVERHEAT;
            shutdown_action();
        } else {
            /* 正常 → 清 EFT (-EFT) */
            g_AlarmFlags &= ~ERR_INV_OVERHEAT;
        }

        /* 每 200ms 检查一轮 */
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
