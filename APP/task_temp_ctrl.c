#include "task_temp_ctrl.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "sys_state.h"
#include "sys_config.h"
#include "bsp_relay.h"

/* ===========================================================================
 * 温控主任务 Task_TempCtrl
 *
 * 合并三个子逻辑, 统一在一个 200ms 周期循环内顺序执行:
 *   1. 停机异常逻辑告警 (最高优先, 先检查)
 *   2. 压缩机开机逻辑   (状态机驱动)
 *   3. 油壳加热逻辑     (根据压缩机状态联动)
 * =========================================================================== */

/* ===================================================================
 *  逻辑2 压缩机状态机枚举
 * =================================================================== */
typedef enum {
    COMP_IDLE,          /* 等待开机指令 */
    COMP_STARTING,      /* 热车中, F=125 */
    COMP_RUNNING,       /* PID 正常运行 */
    COMP_STOPPED        /* 异常停机, 等待恢复 */
} CompState_t;

/* ===================================================================
 *  逻辑1: 停机异常逻辑告警 (通知用户)
 *
 *  顺序检查: VDC欠压 → VAC缺相 → 变频器过流 → 变频器过热
 *  任一异常 → 关压缩机 + 置错误标志 + 通知用户
 *  异常恢复 → 清对应错误标志
 * =================================================================== */
static void shutdown_alarm_check(const SysVarData_t *sensor)
{
    bool need_shutdown = false;

    /* 步骤1: VDC 电源电压 >= VDCmin ? */
    if (sensor->VAR_VDC_VOLTAGE < SET_VDC_MIN) {
        g_AlarmFlags |= ERR_VDC_LOW;        /* EDC */
        need_shutdown = true;
    } else {
        g_AlarmFlags &= ~ERR_VDC_LOW;       /* -EDC */
    }

    /* 步骤2: VAC 错/断相 ? */
    if (BSP_VAC_IsPhaseFault()) {
        g_AlarmFlags |= ERR_VAC_PHASE;      /* EAC */
        need_shutdown = true;
    } else {
        g_AlarmFlags &= ~ERR_VAC_PHASE;     /* -EAC */
    }

    /* 步骤3: 变频器过流 ? */
    if (BSP_INV_IsOverCurrent()) {
        g_AlarmFlags |= ERR_INV_OVERCURR;   /* EFI */
        need_shutdown = true;
    } else {
        g_AlarmFlags &= ~ERR_INV_OVERCURR;  /* -EFI */
    }

    /* 步骤4: 变频器过热 ? */
    if (BSP_INV_IsOverHeat()) {
        g_AlarmFlags |= ERR_INV_OVERHEAT;   /* EFT */
        need_shutdown = true;
    } else {
        g_AlarmFlags &= ~ERR_INV_OVERHEAT;  /* -EFT */
    }

    /* 任一异常 → 关机 + 通知用户 */
    if (need_shutdown) {
        BSP_Comp_Off();
        xEventGroupClearBits(SysEventGroup, ST_COMP_RUNNING);
        g_AlarmFlags |= WARN_NOTIFY_USER;
    }
}

/* ===================================================================
 *  逻辑2 辅助: PID 调整模块 (简易占位)
 *  实际 PID 在后续 "图4 变频控制" 中完善
 *  此处: 根据 ΔT 在 Fmin ~ 125 之间线性调频
 * =================================================================== */
static void pid_adjust(const SysVarData_t *sensor)
{
    /* ΔT = 柜温 - 设定温度 */
    float delta_t = sensor->VAR_CABINET_TEMP - SET_TEMP_TS;

    /* 简易线性映射: ΔT >= DT_MAX → F=125, ΔT <= DT_MIN → F=Fmin */
    float freq;
    if (delta_t >= SET_DT_MAX) {
        freq = SET_FREQ_INIT;
    } else if (delta_t <= SET_DT_MIN) {
        freq = SET_FREQ_MIN;
    } else {
        freq = SET_FREQ_MIN + (delta_t - SET_DT_MIN)
               / (SET_DT_MAX - SET_DT_MIN)
               * (SET_FREQ_INIT - SET_FREQ_MIN);
    }

    SysState_Lock();
    SysState_GetRawPtr()->VAR_DELTA_T    = delta_t;
    SysState_GetRawPtr()->VAR_COMP_FREQ  = freq;
    SysState_Unlock();
}

/* ===================================================================
 *  逻辑2: 压缩机开机逻辑 (状态机)
 *
 *  IDLE     → 等 ST_SYSTEM_ON, 开机 F=125
 *  STARTING → 热车 C20 计时, 到时进 PID
 *  RUNNING  → PID 调节
 *  STOPPED  → 异常停机, 等错误清除回 IDLE
 * =================================================================== */
static void compressor_control(const SysVarData_t *sensor,
                               CompState_t *state,
                               uint32_t *warmup_cnt)
{
    const uint32_t warmup_ticks = SET_WARMUP_C20 * (1000 / 200);
    EventBits_t sys_bits = xEventGroupGetBits(SysEventGroup);
    bool has_error = (g_AlarmFlags & ERR_MASK_ALL) != 0;

    switch (*state) {

    case COMP_IDLE:
        if ((sys_bits & ST_SYSTEM_ON) && !has_error) {
            *state = COMP_STARTING;
            *warmup_cnt = 0;

            /* 开启压缩机, F = 125 */
            BSP_Comp_On();
            xEventGroupSetBits(SysEventGroup, ST_COMP_RUNNING);
            xEventGroupClearBits(SysEventGroup, ST_WARMUP_DONE);

            SysState_Lock();
            SysState_GetRawPtr()->VAR_COMP_FREQ = SET_FREQ_INIT;
            SysState_Unlock();
        }
        break;

    case COMP_STARTING:
        if (has_error) {
            BSP_Comp_Off();
            xEventGroupClearBits(SysEventGroup, ST_COMP_RUNNING);
            *state = COMP_STOPPED;
            break;
        }

        (*warmup_cnt)++;
        if (*warmup_cnt >= warmup_ticks) {
            /* 热车时长 C20 到时 → PID 运行 */
            xEventGroupSetBits(SysEventGroup, ST_WARMUP_DONE);
            xEventGroupSetBits(SysTimerEventGroup, ST_TMR_WARMUP_DONE);
            *state = COMP_RUNNING;
        }
        break;

    case COMP_RUNNING:
        if (has_error || !(sys_bits & ST_SYSTEM_ON)) {
            BSP_Comp_Off();
            xEventGroupClearBits(SysEventGroup, ST_COMP_RUNNING);
            *state = COMP_STOPPED;
            break;
        }
        /* PID 调整 */
        pid_adjust(sensor);
        break;

    case COMP_STOPPED:
        BSP_Comp_Off();
        xEventGroupClearBits(SysEventGroup, ST_COMP_RUNNING);
        xEventGroupClearBits(SysEventGroup, ST_WARMUP_DONE);
        if (!has_error) {
            *state = COMP_IDLE;
        }
        break;
    }
}

/* ===================================================================
 *  逻辑3: 油壳加热逻辑
 *
 *  压缩机运行     → 油壳加热关
 *  压缩机停机 + 环境 ≤ 10°C → 油壳加热开
 *  压缩机停机 + 环境 > 10°C  → 油壳加热关
 * =================================================================== */
static void oil_heater_control(const SysVarData_t *sensor)
{
    EventBits_t sys_bits = xEventGroupGetBits(SysEventGroup);
    bool comp_running = (sys_bits & ST_COMP_RUNNING) != 0;

    if (comp_running) {
        /* 压缩机开机 → 油壳加热关 */
        BSP_OilHeater_Off();
        xEventGroupClearBits(SysEventGroup, ST_OIL_HEAT_ON);
    } else {
        /* 压缩机未开机 → 判断环境温度 */
        float ambient = sensor->VAR_SHT30_TEMP;

        if (ambient <= SET_OIL_HEAT_TEMP) {
            BSP_OilHeater_On();
            xEventGroupSetBits(SysEventGroup, ST_OIL_HEAT_ON);
        } else {
            BSP_OilHeater_Off();
            xEventGroupClearBits(SysEventGroup, ST_OIL_HEAT_ON);
        }
    }
}

/* ===================================================================
 *  主任务入口: 200ms 周期, 顺序执行三个子逻辑
 * =================================================================== */
void Task_TempCtrl_Process(void const *argument)
{
    (void)argument;

    SysVarData_t sensor;
    CompState_t  comp_state  = COMP_IDLE;
    uint32_t     warmup_cnt  = 0;

    for (;;) {
        /* 统一采集一次传感器数据, 三个子逻辑共用 */
        SysState_GetSensor(&sensor);

        /* 逻辑1: 停机异常告警 (最先执行, 保证安全) */
        shutdown_alarm_check(&sensor);

        /* 逻辑2: 压缩机开机控制 */
        compressor_control(&sensor, &comp_state, &warmup_cnt);

        /* 逻辑3: 油壳加热控制 */
        oil_heater_control(&sensor);

        /* 200ms 统一周期 */
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
