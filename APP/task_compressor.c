#include "task_compressor.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "sys_state.h"
#include "sys_config.h"
#include "bsp_relay.h"

/* ===========================================================================
 * 逻辑2: 压缩机开机逻辑
 *
 * 状态机:
 *   IDLE     → 等待温控逻辑发出开机请求 (ST_SYSTEM_ON)
 *   STARTING → 开继电器, 设 F=125, 等待 C20 热车完成
 *   RUNNING  → 热车结束, PID 正常调节中
 *   STOPPED  → 存在异常或温控关机, 等待重新请求
 * =========================================================================== */

typedef enum {
    COMP_IDLE,
    COMP_STARTING,
    COMP_RUNNING,
    COMP_STOPPED
} CompState_t;

/* -------------------------------------------------------
 * PID 调整模块 (简易占位)
 * 实际 PID 算法在后续 "图4 变频控制" 中完善
 * 此处仅做: 根据 ΔT 在 Fmin ~ 125 之间线性调频
 * ------------------------------------------------------- */
static void pid_adjust(void)
{
    SysVarData_t sensor;
    SysState_GetSensor(&sensor);

    /* ΔT = 柜温 - 设定温度 */
    float delta_t = sensor.VAR_CABINET_TEMP - SET_TEMP_TS;

    SysState_Lock();
    SysState_GetRawPtr()->VAR_DELTA_T = delta_t;

    /* 简易线性映射: ΔT >= DT_MAX → F=125, ΔT <= DT_MIN → F=Fmin */
    float freq;
    if (delta_t >= SET_DT_MAX) {
        freq = SET_FREQ_INIT;           /* 满载 125 Hz */
    } else if (delta_t <= SET_DT_MIN) {
        freq = SET_FREQ_MIN;            /* 最低 20 Hz */
    } else {
        /* 线性插值 */
        freq = SET_FREQ_MIN + (delta_t - SET_DT_MIN)
               / (SET_DT_MAX - SET_DT_MIN)
               * (SET_FREQ_INIT - SET_FREQ_MIN);
    }

    SysState_GetRawPtr()->VAR_COMP_FREQ = freq;
    SysState_Unlock();
}

void Task_Compressor_Process(void const *argument)
{
    (void)argument;

    CompState_t state = COMP_IDLE;
    uint32_t warmup_cnt = 0;            /* 热车计时 (单位: 任务周期) */
    const uint32_t warmup_ticks = SET_WARMUP_C20 * (1000 / 200);
                                        /* C20秒 × (1000ms/200ms周期) */

    for (;;) {
        EventBits_t sys_bits = xEventGroupGetBits(SysEventGroup);
        bool has_error = (g_AlarmFlags & ERR_MASK_ALL) != 0;

        switch (state) {

        /* =============================================
         * IDLE: 等待系统开机指令
         * ============================================= */
        case COMP_IDLE:
            if ((sys_bits & ST_SYSTEM_ON) && !has_error) {
                /* 收到开机请求, 进入启动阶段 */
                state = COMP_STARTING;
                warmup_cnt = 0;

                /* --- 开启压缩机, F = 125 --- */
                BSP_Comp_On();
                xEventGroupSetBits(SysEventGroup, ST_COMP_RUNNING);
                xEventGroupClearBits(SysEventGroup, ST_WARMUP_DONE);

                SysState_Lock();
                SysState_GetRawPtr()->VAR_COMP_FREQ = SET_FREQ_INIT;
                SysState_Unlock();
            }
            break;

        /* =============================================
         * STARTING: 热车阶段, 等待 C20 到时
         * ============================================= */
        case COMP_STARTING:
            if (has_error) {
                /* 异常 → 立即停机 */
                BSP_Comp_Off();
                xEventGroupClearBits(SysEventGroup, ST_COMP_RUNNING);
                state = COMP_STOPPED;
                break;
            }

            warmup_cnt++;
            if (warmup_cnt >= warmup_ticks) {
                /* 热车时长 C20 到时 → 标记热车完成, 进入 PID 运行 */
                xEventGroupSetBits(SysEventGroup, ST_WARMUP_DONE);
                xEventGroupSetBits(SysTimerEventGroup, ST_TMR_WARMUP_DONE);
                state = COMP_RUNNING;
            }
            /* C20 未到时 → 保持 F=125 继续热车 */
            break;

        /* =============================================
         * RUNNING: 热车结束, PID 调整模块
         * ============================================= */
        case COMP_RUNNING:
            if (has_error || !(sys_bits & ST_SYSTEM_ON)) {
                /* 异常或关机 → 停压缩机 */
                BSP_Comp_Off();
                xEventGroupClearBits(SysEventGroup, ST_COMP_RUNNING);
                state = COMP_STOPPED;
                break;
            }

            /* PID 调整模块 */
            pid_adjust();
            break;

        /* =============================================
         * STOPPED: 等待异常清除后回到 IDLE
         * ============================================= */
        case COMP_STOPPED:
            BSP_Comp_Off();
            xEventGroupClearBits(SysEventGroup, ST_COMP_RUNNING);
            xEventGroupClearBits(SysEventGroup, ST_WARMUP_DONE);

            if (!has_error) {
                state = COMP_IDLE;
            }
            break;
        }

        /* 每 200ms 执行一轮 */
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
