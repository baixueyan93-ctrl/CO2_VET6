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
// 极简接收缓冲区
// ==========================================
uint8_t rx_byte;           // 每次只收1个字节
uint8_t rx_buffer[128];    // 存字符串的数组
uint16_t rx_index = 0;     // 当前存到了第几个
volatile uint8_t rx_complete = 0; // 接收完成标志 (1表示收完了)

// HAL 库自带的接收中断回调 (每收到1个字节，自动进这里一次)
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if(huart->Instance == UART4) {
        rx_buffer[rx_index++] = rx_byte;
        
        // 如果收到“回车换行”，或者快装满了，就认为收到了一条完整指令
        if(rx_byte == '\n' || rx_byte == '\r' || rx_index >= 127) {
            rx_buffer[rx_index] = '\0'; // 加上字符串结尾
            rx_complete = 1;            // 通知任务去处理
        } else {
            // 如果还没收完，就继续监听下一个字节
            HAL_UART_Receive_IT(&huart4, &rx_byte, 1);
        }
    }
}

// ==========================================
// 极简故障记录函数
// ==========================================
uint8_t System_Record_Fault(uint8_t fault_code) {
    SysLog_t new_log = {0};
    RTC_DateTypeDef sDate;
    RTC_TimeTypeDef sTime;

    // 1. 获取当前的 RTC 时间戳
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    new_log.Year  = sDate.Year;  new_log.Month = sDate.Month; new_log.Date  = sDate.Date;
    new_log.Hours = sTime.Hours; new_log.Minutes = sTime.Minutes; new_log.Seconds = sTime.Seconds;
    new_log.EventType = fault_code;

    // ==========================================
    // 2. 【核心修改】：去系统的安全黑板上抄最新数据！
    // ==========================================
    SysSensorData_t current_sensor_data;
    SysState_GetSensor(&current_sensor_data); 

    // 3. 把黑板上的真实数据，填入 EEPROM 的记录表格里
    new_log.EvapTemp = current_sensor_data.V_EVAP_T;  // 填入 10K 蒸发温度
    
    // 【前提提示】：下面这行需要您在 bsp_eeprom.h 的 SysLog_t 结构体里提前加上 float CondTemp;
    new_log.CondTemp = current_sensor_data.V_EXH_T;   // 填入 50K 排气/冷凝温度
    
    // 【未来扩展】：如果您以后加了压力，也是在这里直接加一句：
    // new_log.Pressure = current_sensor_data.V_PRES; 

    // 4. 安全存入物理黑匣子
    if(osMutexWait(EEPROM_MutexHandle, 500) == osOK) {
        BSP_Log_Add(&new_log); 
        osMutexRelease(EEPROM_MutexHandle); 
        return 0; 
    }
    return 1; 
}

// ==========================================
// 极简主任务
// ==========================================
void Task_RS485Log_Process(void const *argument) {
    SysState_Init();  // 初始化锁
    BSP_RS485_Init();
    BSP_Log_Init(); 
    osDelay(100); 
    
    BSP_RS485_SendString("\r\n--- Simple Mode Ready! ---\r\n");
    
    // 开启第一次中断接收 (只等 1 个字节)
    HAL_UART_Receive_IT(&huart4, &rx_byte, 1);
    
    for(;;) {
        // 如果上面中断里说：指令收完了！
        if (rx_complete == 1) {
            
            // 1. 处理 GET 指令
            if(strstr((char *)rx_buffer, "GET") != NULL) {
    SysSensorData_t current_data;
    
    // 【修改这里】：去系统的安全黑板上读取最新数据！
    SysState_GetSensor(&current_data); 
    
    char reply_msg[64];
    // 打印从结构体里读出来的蒸发温度(10K)和排气温度(50K)
    sprintf(reply_msg, "T_Evap(10K):%.1f | T_Exh(50K):%.1f\r\n", 
            current_data.V_EVAP_T, current_data.V_EXH_T);
            
    BSP_RS485_SendString(reply_msg);
}
            // 2. 处理 TEST 指令
            else if(strstr((char *)rx_buffer, "TEST") != NULL) {
                if(System_Record_Fault(0x99) == 0) BSP_RS485_SendString("Test Saved!\r\n");
            }
            // 3. 处理 READ 指令：永远只看最新鲜的 5 条（或 20 条）
            else if(strstr((char *)rx_buffer, "READ") != NULL) {
                BSP_RS485_SendString("\r\n--- LATEST LOG START ---\r\n");
                
                if(osMutexWait(EEPROM_MutexHandle, 1000) == osOK) {
                    SysLog_t temp_log;
                    
                    // 获取底层“写字笔”的当前位置
                    uint8_t current_idx = BSP_Log_Get_Current_Index();
                    
                    // 决定您一次想看最新的几条？这里以 5 条为例
                    uint8_t read_count = 5; 
                    
                    // 从最新的一条开始，倒着往前挖！
                    for(int i = 0; i < read_count; i++) {
                        
                        // 【工业级环形倒推算法核心】：
                        // 为什么要加 LOG_MAX_COUNT？因为如果 current_idx 是 0，0-1=-1就出界了！
                        // 加上最大值再取余数，就能完美实现：0 的上一条是 126！
                        int physical_idx = (current_idx - 1 - i + LOG_MAX_COUNT) % LOG_MAX_COUNT;
                        
                        BSP_Log_Read_By_Index(physical_idx, &temp_log); 
                        
                        // 可选优化：如果挖出来的年份是 0，说明板子是全新的，这行还是空的，跳过不打
                        if (temp_log.Year == 0) continue; 
                        
                        char log_msg[128];
                        // 打印时，我们标上“倒数第几天”的概念，比如 [Newest - 0], [Newest - 1]
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
            // 4. 处理修改时间的 SETTIME 指令
            // 预期格式例如: SETTIME:26-03-15,14:30:00 (表示 2026年3月15日 14点30分00秒)
            else if(strstr((char *)rx_buffer, "SETTIME") != NULL) {
                int year, month, date, hour, minute, second;
                
                // 使用 sscanf 从字符串中提取出年月日时分秒
                if (sscanf((char *)rx_buffer, "SETTIME:%d-%d-%d,%d:%d:%d", 
                           &year, &month, &date, &hour, &minute, &second) == 6) {
                    
                    RTC_TimeTypeDef sTime = {0};
                    RTC_DateTypeDef sDate = {0};
                    
                    // 填入时间
                    sTime.Hours = hour;
                    sTime.Minutes = minute;
                    sTime.Seconds = second;
                    sTime.TimeFormat = RTC_HOURFORMAT12_AM;
                    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
                    sTime.StoreOperation = RTC_STOREOPERATION_RESET;
                    
                    // 填入日期
                    sDate.WeekDay = RTC_WEEKDAY_MONDAY; // 星期几对记日志影响不大，随便设一个
                    sDate.Month = month;
                    sDate.Date = date;
                    sDate.Year = year;
                    
                    // 【注意】：STM32的硬件要求必须先设置 Time，再设置 Date！
                    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) == HAL_OK &&
                        HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) == HAL_OK) {
                        BSP_RS485_SendString("RTC Time Updated Successfully!\r\n");
                    } else {
                        BSP_RS485_SendString("ERROR: RTC Hardware Fault!\r\n");
                    }
                } else {
                    // 如果用户格式输错了，提示他正确格式
                    BSP_RS485_SendString("Format Error! Pls use: SETTIME:YY-MM-DD,HH:MM:SS\r\n");
                }
            }
						
            // 处理完后，清理战场，重新开启监听
            rx_index = 0;
            memset(rx_buffer, 0, sizeof(rx_buffer));
            rx_complete = 0;
						// ==========================================
            // 【终极杀手锏】：暴力清除所有硬件级死锁标志！
            // 防止 RS485 收发切换瞬间的毛刺导致单片机硬件罢工
            // ==========================================
            __HAL_UART_CLEAR_OREFLAG(&huart4); // 清除溢出错误 (Overrun)
            __HAL_UART_CLEAR_NEFLAG(&huart4);  // 清除噪声错误 (Noise)
            __HAL_UART_CLEAR_FEFLAG(&huart4);  // 清除帧错误 (Framing)
            
            huart4.ErrorCode = HAL_UART_ERROR_NONE; // 强行骗过 HAL 库，说没错误
            huart4.RxState = HAL_UART_STATE_READY;  // 强行把状态机掰回准备就绪状态
            HAL_UART_Receive_IT(&huart4, &rx_byte, 1); 
        }
        
        // 没事干就睡 50ms，不占 CPU
        osDelay(50);
    }
}



