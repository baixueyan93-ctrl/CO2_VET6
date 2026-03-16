#include "bsp_eeprom.h"
#include "FreeRTOS.h"
#include "task.h"

extern I2C_HandleTypeDef hi2c1;

// 彻底解决跨页覆盖问题，并完美兼容 24C16 的动态器件地址
// ==========================================
void BSP_EEPROM_Write(uint16_t memAddress, uint8_t *pData, uint16_t size) {
    for(uint16_t i = 0; i < size; i++) {
        uint16_t current_addr = memAddress + i;
        // 24C16 的前 3 位地址要藏在器件地址里
        uint8_t dev_addr = EEPROM_ADDRESS | ((current_addr >> 8) << 1); 
        uint8_t reg_addr = current_addr & 0xFF; // 低 8 位作为内部地址
        
        HAL_I2C_Mem_Write(&hi2c1, dev_addr, reg_addr, I2C_MEMADD_SIZE_8BIT, &pData[i], 1, 100);
        vTaskDelay(pdMS_TO_TICKS(5)); // 写完1个字节必须等5ms，让EEPROM完成物理烧写
    }
}

// 终极读取法 (原理同上)
void BSP_EEPROM_Read(uint16_t memAddress, uint8_t *pData, uint16_t size) {
    for(uint16_t i = 0; i < size; i++) {
        uint16_t current_addr = memAddress + i;
        uint8_t dev_addr = EEPROM_ADDRESS | ((current_addr >> 8) << 1); 
        uint8_t reg_addr = current_addr & 0xFF;
        
        HAL_I2C_Mem_Read(&hi2c1, dev_addr, reg_addr, I2C_MEMADD_SIZE_8BIT, &pData[i], 1, 100);
    }
}

// ==========================================
// 环形日志存储算法实现
// ==========================================

// 全局变量，记录当前写到了第几条 (0 ~ 126)
static uint8_t Current_Log_Index = 0; 

/**
 * @brief 日志系统初始化 (开机读取指针)
 */
void BSP_Log_Init(void) {
    // 从 0x0000 地址读出当前的索引值
    BSP_EEPROM_Read(EEPROM_INDEX_ADDR, &Current_Log_Index, 1);
    
    // 如果是全新的芯片(读出是0xFF)或索引越界，强制归零
    if(Current_Log_Index >= LOG_MAX_COUNT) {
        Current_Log_Index = 0;
        BSP_EEPROM_Write(EEPROM_INDEX_ADDR, &Current_Log_Index, 1);
    }
}

/**
 * @brief 存入一条新日志 (自动计算地址，自动循环)
 */
void BSP_Log_Add(SysLog_t *new_log) {
    // 1. 计算要存入的物理地址: 基地址 + (当前条数 * 16字节)
    uint16_t physical_addr = EEPROM_DATA_START + (Current_Log_Index * LOG_SIZE);
    
    // 2. 将结构体存入该物理地址
    BSP_EEPROM_Write(physical_addr, (uint8_t *)new_log, LOG_SIZE);
    
    // 3. 索引加 1，如果到了 127 就回到 0 (实现环形覆盖)
    Current_Log_Index++;
    if(Current_Log_Index >= LOG_MAX_COUNT) {
        Current_Log_Index = 0;
    }
    
    // 4. 把最新的索引写回 0x0000 地址，掉电不丢失
    BSP_EEPROM_Write(EEPROM_INDEX_ADDR, &Current_Log_Index, 1);
}

/**
 * @brief 读取指定索引的日志
 */
void BSP_Log_Read_By_Index(uint8_t index, SysLog_t *out_log) {
    if(index >= LOG_MAX_COUNT) return;
    
    // 计算物理地址并读出
    uint16_t physical_addr = EEPROM_DATA_START + (index * LOG_SIZE);
    BSP_EEPROM_Read(physical_addr, (uint8_t *)out_log, LOG_SIZE);
}
/**
 * @brief 暴露当前笔尖位置给外部任务使用
 */
uint8_t BSP_Log_Get_Current_Index(void) {
    return Current_Log_Index;
}


