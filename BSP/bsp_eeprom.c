#include "bsp_eeprom.h"
#include "bsp_i2c_mutex.h"
#include "FreeRTOS.h"
#include "task.h"

extern I2C_HandleTypeDef hi2c1;

void BSP_EEPROM_Write(uint16_t memAddress, uint8_t *pData, uint16_t size) {
    BSP_I2C1_Lock();
    HAL_I2C_Mem_Write(&hi2c1, EEPROM_ADDRESS, memAddress, I2C_MEMADD_SIZE_8BIT, pData, size, 100);
    BSP_I2C1_Unlock();
    vTaskDelay(pdMS_TO_TICKS(5));
}

void BSP_EEPROM_Read(uint16_t memAddress, uint8_t *pData, uint16_t size) {
    BSP_I2C1_Lock();
    HAL_I2C_Mem_Read(&hi2c1, EEPROM_ADDRESS, memAddress, I2C_MEMADD_SIZE_8BIT, pData, size, 100);
    BSP_I2C1_Unlock();
}

// ==========================================
// ������־�洢�㷨ʵ��
// ==========================================

// ȫ�ֱ�������¼��ǰд���˵ڼ��� (0 ~ 126)
static uint8_t Current_Log_Index = 0; 

/**
 * @brief ��־ϵͳ��ʼ�� (������ȡָ��)
 */
void BSP_Log_Init(void) {
    // �� 0x0000 ��ַ������ǰ������ֵ
    BSP_EEPROM_Read(EEPROM_INDEX_ADDR, &Current_Log_Index, 1);
    
    // �����ȫ�µ�оƬ(������0xFF)������Խ�磬ǿ�ƹ���
    if(Current_Log_Index >= LOG_MAX_COUNT) {
        Current_Log_Index = 0;
        BSP_EEPROM_Write(EEPROM_INDEX_ADDR, &Current_Log_Index, 1);
    }
}

/**
 * @brief ����һ������־ (�Զ������ַ���Զ�ѭ��)
 */
void BSP_Log_Add(SysLog_t *new_log) {
    // 1. ����Ҫ�����������ַ: ����ַ + (��ǰ���� * 16�ֽ�)
    uint16_t physical_addr = EEPROM_DATA_START + (Current_Log_Index * LOG_SIZE);
    
    // 2. ���ṹ������������ַ
    BSP_EEPROM_Write(physical_addr, (uint8_t *)new_log, LOG_SIZE);
    
    // 3. ������ 1��������� 127 �ͻص� 0 (ʵ�ֻ��θ���)
    Current_Log_Index++;
    if(Current_Log_Index >= LOG_MAX_COUNT) {
        Current_Log_Index = 0;
    }
    
    // 4. �����µ�����д�� 0x0000 ��ַ�����粻��ʧ
    BSP_EEPROM_Write(EEPROM_INDEX_ADDR, &Current_Log_Index, 1);
}

/**
 * @brief ��ȡָ����������־
 */
void BSP_Log_Read_By_Index(uint8_t index, SysLog_t *out_log) {
    if(index >= LOG_MAX_COUNT) return;
    
    // ����������ַ������
    uint16_t physical_addr = EEPROM_DATA_START + (index * LOG_SIZE);
    BSP_EEPROM_Read(physical_addr, (uint8_t *)out_log, LOG_SIZE);
}
/**
 * @brief ��¶��ǰ�ʼ�λ�ø��ⲿ����ʹ��
 */
uint8_t BSP_Log_Get_Current_Index(void) {
    return Current_Log_Index;
}


