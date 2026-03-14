#ifndef __BSP_EEPROM_H
#define __BSP_EEPROM_H
#include "main.h"
#include "sys_config.h" // 引入全局配置和日志结构体

#define EEPROM_ADDRESS 0xA0 

// 基础读写接口 (保持不变)
void BSP_EEPROM_Write(uint16_t memAddress, uint8_t *pData, uint16_t size);
void BSP_EEPROM_Read(uint16_t memAddress, uint8_t *pData, uint16_t size);
void BSP_Log_Read(uint16_t index, SysLog_t *log); // (请根据您实际的函数参数类型写，通常是这样)
// ==========================================
// 新增：工业级环形日志存储专用接口
// ==========================================
void BSP_Log_Init(void);                      // 初始化日志指针
void BSP_Log_Add(SysLog_t *new_log);          // 存入一条新日志
void BSP_Log_Read_By_Index(uint8_t index, SysLog_t *out_log); // 按索引读出某条日志
uint8_t BSP_Log_Get_Current_Index(void);
#endif



