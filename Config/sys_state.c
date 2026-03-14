#include "sys_state.h"
#include <string.h>

/* 全局静态变量：真正的系统数据仓库 */
static SysSensorData_t g_SensorData = {0};

/* 【关键修改】：在这里真正定义它们，分配内存！ */
EventGroupHandle_t SysEventGroup = NULL;
SemaphoreHandle_t  SensorDataMutex = NULL;

/* 新增：系统状态初始化函数 (负责把锁和事件组造出来) */
void SysState_Init(void) {
    if (SensorDataMutex == NULL) {
        SensorDataMutex = xSemaphoreCreateMutex(); // 创建互斥锁
    }
    if (SysEventGroup == NULL) {
        SysEventGroup = xEventGroupCreate();       // 创建事件标志组
    }
}

// 安全写入数据
void SysState_UpdateSensor(SysSensorData_t* newData) {
    if(SensorDataMutex != NULL && newData != NULL) {
        if(xSemaphoreTake(SensorDataMutex, portMAX_DELAY) == pdTRUE) {
            memcpy(&g_SensorData, newData, sizeof(SysSensorData_t));
            xSemaphoreGive(SensorDataMutex); 
        }
    }
}

// 安全读取数据
void SysState_GetSensor(SysSensorData_t* outData) {
    if(SensorDataMutex != NULL && outData != NULL) {
        if(xSemaphoreTake(SensorDataMutex, portMAX_DELAY) == pdTRUE) {
            memcpy(outData, &g_SensorData, sizeof(SysSensorData_t));
            xSemaphoreGive(SensorDataMutex);
        }
    }
}



