#include "task_rs485_log.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "bsp_eeprom.h"
#include "bsp_rs485.h"
#include "sys_state.h"
#include "rtc.h"
#include "task_adc.h"
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart4;
extern osMutexId EEPROM_MutexHandle;

// ==========================================
// ������ջ�����
// ==========================================
uint8_t rx_byte;           // ÿ��ֻ��1���ֽ�
uint8_t rx_buffer[128];    // ���ַ���������
uint16_t rx_index = 0;     // ��ǰ�浽�˵ڼ���
volatile uint8_t rx_complete = 0; // ������ɱ�־ (1��ʾ������)

// HAL ���Դ��Ľ����жϻص� (ÿ�յ�1���ֽڣ��Զ�������һ��)
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if(huart->Instance == UART4) {
        rx_buffer[rx_index++] = rx_byte;
        
        // ����յ����س����С������߿�װ���ˣ�����Ϊ�յ���һ������ָ��
        if(rx_byte == '\n' || rx_byte == '\r' || rx_index >= 127) {
            rx_buffer[rx_index] = '\0'; // �����ַ�����β
            rx_complete = 1;            // ֪ͨ����ȥ����
        } else {
            // �����û���꣬�ͼ���������һ���ֽ�
            HAL_UART_Receive_IT(&huart4, &rx_byte, 1);
        }
    }
}

// ==========================================
// ������ϼ�¼����
// ==========================================
uint8_t System_Record_Fault(uint8_t fault_code) {
    SysLog_t new_log = {0};
    RTC_DateTypeDef sDate;
    RTC_TimeTypeDef sTime;

    // 1. ��ȡ��ǰ�� RTC ʱ���
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    new_log.Year  = sDate.Year;  new_log.Month = sDate.Month; new_log.Date  = sDate.Date;
    new_log.Hours = sTime.Hours; new_log.Minutes = sTime.Minutes; new_log.Seconds = sTime.Seconds;
    new_log.EventType = fault_code;

    // ==========================================
    // 2. �������޸ġ���ȥϵͳ�İ�ȫ�ڰ��ϳ��������ݣ�
    // ==========================================
    SysVarData_t current_sensor_data;
    SysState_GetSensor(&current_sensor_data);

    new_log.EvapTemp = current_sensor_data.VAR_EVAP_TEMP;     // 10K 蒸发温度
    new_log.CondTemp = current_sensor_data.VAR_EXHAUST_TEMP;  // 50K 排气/冷凝温度 

    // 4. ��ȫ����������ϻ��
    if(osMutexWait(EEPROM_MutexHandle, 500) == osOK) {
        BSP_Log_Add(&new_log); 
        osMutexRelease(EEPROM_MutexHandle); 
        return 0; 
    }
    return 1; 
}

// ==========================================
// ����������
// ==========================================
void Task_RS485Log_Process(void const *argument) {
    SysState_Init();  // ��ʼ����
    BSP_RS485_Init();
    BSP_Log_Init(); 
    osDelay(100); 
    
    BSP_RS485_SendString("\r\n--- Simple Mode Ready! ---\r\n");
    
    // ������һ���жϽ��� (ֻ�� 1 ���ֽ�)
    HAL_UART_Receive_IT(&huart4, &rx_byte, 1);
    
    for(;;) {
        // ��������ж���˵��ָ�������ˣ�
        if (rx_complete == 1) {
            
            // 1. ���� GET ָ��
            if(strstr((char *)rx_buffer, "GET") != NULL) {
    SysVarData_t current_data;
    
    // ���޸������ȥϵͳ�İ�ȫ�ڰ��϶�ȡ�������ݣ�
    SysState_GetSensor(&current_data); 
    
    char reply_msg[64];
    // ��ӡ�ӽṹ����������������¶�(10K)�������¶�(50K)
    sprintf(reply_msg, "T_Evap(10K):%.1f | T_Exh(50K):%.1f\r\n", 
            current_data.VAR_EVAP_TEMP, current_data.VAR_EXHAUST_TEMP);
            
    BSP_RS485_SendString(reply_msg);
}
            // 2. ���� TEST ָ��
            else if(strstr((char *)rx_buffer, "TEST") != NULL) {
                if(System_Record_Fault(0x99) == 0) BSP_RS485_SendString("Test Saved!\r\n");
            }
            // 3. ���� READ ָ���Զֻ�������ʵ� 5 ������ 20 ����
            else if(strstr((char *)rx_buffer, "READ") != NULL) {
                BSP_RS485_SendString("\r\n--- LATEST LOG START ---\r\n");
                
                if(osMutexWait(EEPROM_MutexHandle, 1000) == osOK) {
                    SysLog_t temp_log;
                    
                    // ��ȡ�ײ㡰д�ֱʡ��ĵ�ǰλ��
                    uint8_t current_idx = BSP_Log_Get_Current_Index();
                    
                    // ������һ���뿴���µļ����������� 5 ��Ϊ��
                    uint8_t read_count = 5; 
                    
                    // �����µ�һ����ʼ��������ǰ�ڣ�
                    for(int i = 0; i < read_count; i++) {
                        
                        // ����ҵ�����ε����㷨���ġ���
                        // ΪʲôҪ�� LOG_MAX_COUNT����Ϊ��� current_idx �� 0��0-1=-1�ͳ����ˣ�
                        // �������ֵ��ȡ��������������ʵ�֣�0 ����һ���� 126��
                        int physical_idx = (current_idx - 1 - i + LOG_MAX_COUNT) % LOG_MAX_COUNT;
                        
                        BSP_Log_Read_By_Index(physical_idx, &temp_log); 
                        
                        // ��ѡ�Ż�������ڳ���������� 0��˵��������ȫ�µģ����л��ǿյģ���������
                        if (temp_log.Year == 0) continue; 
                        
                        char log_msg[128];
                        // ��ӡʱ�����Ǳ��ϡ������ڼ��족�ĸ������ [Newest - 0], [Newest - 1]
                        sprintf(log_msg, "[Newest-%d] (Idx:%d) 20%02d-%02d-%02d %02d:%02d:%02d | Evt:0x%02X | Temp:%.1f\r\n", 
                                i, physical_idx, temp_log.Year, temp_log.Month, temp_log.Date,
                                temp_log.Hours, temp_log.Minutes, temp_log.Seconds,
                                temp_log.EventType, temp_log.EvapTemp);
                                
                        BSP_RS485_SendString(log_msg);
                        osDelay(10); 
                    }
                    osMutexRelease(EEPROM_MutexHandle); 
                }
                BSP_RS485_SendString("--- LATEST LOG END ---\r\n");
            }
            // 4. �����޸�ʱ��� SETTIME ָ��
            // Ԥ�ڸ�ʽ����: SETTIME:26-03-15,14:30:00 (��ʾ 2026��3��15�� 14��30��00��)
            else if(strstr((char *)rx_buffer, "SETTIME") != NULL) {
                int year, month, date, hour, minute, second;
                
                // ʹ�� sscanf ���ַ�������ȡ��������ʱ����
                if (sscanf((char *)rx_buffer, "SETTIME:%d-%d-%d,%d:%d:%d", 
                           &year, &month, &date, &hour, &minute, &second) == 6) {
                    
                    RTC_TimeTypeDef sTime = {0};
                    RTC_DateTypeDef sDate = {0};
                    
                    // ����ʱ��
                    sTime.Hours = hour;
                    sTime.Minutes = minute;
                    sTime.Seconds = second;
                    sTime.TimeFormat = RTC_HOURFORMAT12_AM;
                    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
                    sTime.StoreOperation = RTC_STOREOPERATION_RESET;
                    
                    // ��������
                    sDate.WeekDay = RTC_WEEKDAY_MONDAY; // ���ڼ��Լ���־Ӱ�첻�������һ��
                    sDate.Month = month;
                    sDate.Date = date;
                    sDate.Year = year;
                    
                    // ��ע�⡿��STM32��Ӳ��Ҫ����������� Time�������� Date��
                    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) == HAL_OK &&
                        HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) == HAL_OK) {
                        BSP_RS485_SendString("RTC Time Updated Successfully!\r\n");
                    } else {
                        BSP_RS485_SendString("ERROR: RTC Hardware Fault!\r\n");
                    }
                } else {
                    // ����û���ʽ����ˣ���ʾ����ȷ��ʽ
                    BSP_RS485_SendString("Format Error! Pls use: SETTIME:YY-MM-DD,HH:MM:SS\r\n");
                }
            }
						
            // �����������ս�������¿�������
            rx_index = 0;
            memset(rx_buffer, 0, sizeof(rx_buffer));
            rx_complete = 0;
						// ==========================================
            // ���ռ�ɱ��ﵡ��������������Ӳ����������־��
            // ��ֹ RS485 �շ��л�˲���ë�̵��µ�Ƭ��Ӳ���չ�
            // ==========================================
            __HAL_UART_CLEAR_OREFLAG(&huart4); // ���������� (Overrun)
            __HAL_UART_CLEAR_NEFLAG(&huart4);  // ����������� (Noise)
            __HAL_UART_CLEAR_FEFLAG(&huart4);  // ���֡���� (Framing)
            
            huart4.ErrorCode = HAL_UART_ERROR_NONE; // ǿ��ƭ�� HAL �⣬˵û����
            huart4.RxState = HAL_UART_STATE_READY;  // ǿ�а�״̬������׼������״̬
            HAL_UART_Receive_IT(&huart4, &rx_byte, 1); 
        }
        
        // û�¸ɾ�˯ 50ms����ռ CPU
        osDelay(50);
    }
}



