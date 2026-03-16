#ifndef SYS_STATE_H
#define SYS_STATE_H

#include "FreeRTOS.h"
#include "event_groups.h"
#include "semphr.h"
#include <stdint.h>
#include <stdbool.h>

/* --- 系统状态标志位 (用于 FreeRTOS 事件组) --- */
#define F_COMP_ON           (1 << 1)
#define F_OIL_HEAT_ON       (1 << 2)
#define F_DEF_ACTIVE        (1 << 7)
// ...后续可随时加减

/* --- 传感器测量数据结构体 --- */
typedef struct {
    float V_CAB_T;      // 柜温
    float V_EXH_T;      // 排气温度【由 50K 探头采集】
    float V_SUC_T;      // 回气温度
    float V_EVAP_T;     // 蒸发温度【由 10K 探头采集】
	uint8_t Liquid_Level; // 液位状态 (1:有水, 0:没水)
    float   V_PRES;       // 压力真实值 (MPa)
    // ...后续扩展
} SysSensorData_t;

/* 外部句柄声明 */
extern EventGroupHandle_t SysEventGroup;
extern SemaphoreHandle_t  SensorDataMutex;

/* 安全接口声明 */
void SysState_UpdateSensor(SysSensorData_t* newData);
void SysState_GetSensor(SysSensorData_t* outData);

void SysState_Init(void);


#endif /* SYS_STATE_H */



