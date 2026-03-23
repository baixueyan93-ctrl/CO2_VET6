#ifndef __TASK_SHUTDOWN_ALARM_H
#define __TASK_SHUTDOWN_ALARM_H

/* ===========================================================================
 * 逻辑1: 停机异常逻辑告警 (通知用户)
 *
 * 流程 (来源: 1.1温度控制流程.pdf 左侧逻辑图):
 *   1. 采集 VDC 电源电压, 若 VDC < VDCmin → 置 EDC 错误, 关机
 *   2. 检查 VAC 错/断相        → 置 EAC 错误, 关机
 *   3. 检查变频器过流           → 置 EFI 错误, 关机
 *   4. 检查变频器过热           → 置 EFT 错误, 关机
 *   5. 若任一异常触发 → 停压缩机 + 通知用户
 * =========================================================================== */

void Task_ShutdownAlarm_Process(void const *argument);

#endif /* __TASK_SHUTDOWN_ALARM_H */
