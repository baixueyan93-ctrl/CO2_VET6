#ifndef SYS_CONFIG_H
#define SYS_CONFIG_H

#include <stdint.h> // 【避坑关键】：必须包含此头文件，否则编译器不认识 uint8_t

/* =======================================================
 * CO2 制冷系统 全局参数配置字典 (常量 C_ 前缀)
 * 依据：图1~图6 逻辑流程图 & V13 硬件设计
 * ======================================================= */

/* --- 1. 温度控制与保护参数 --- */
#define C_TEMP_SET_TS       -20.0f    // 设定目标柜温 Ts (℃)
#define C_TEMP_HYST_C1      2.0f      // 温度回差 C1 (℃)
#define C_PROT_STOP_C2      180       // 停机保护时间 C2 (秒)
#define C_PROT_STOP_C3      180       // 停机保护时间 C3 (秒)
#define C_PROT_STBY_C7      300       // 待机保护时间 C7 (秒)
#define C_RUN_MIN_C8        300       // 最短运行时间 C8 (秒)
#define C_ALARM_A1          -10.0f    // 告警设定值 A1 (℃)
#define C_ALARM_A2          30.0f     // 告警设定值 A2 (℃)
#define C_ALARM_A3          80.0f     // 告警设定值 A3 (℃)
#define C_COMP_DLY          (20 * 60) // 压缩机防频繁启停延时 (20分钟)
#define C_OIL_T_THRES       10.0f     // 油壳加热开启阈值 (<=10℃)
#define C_WARN_TOUT         300       // 防呆告警超时时间 (秒)

/* --- 2. 除霜流程参数 --- */
#define C_DEF_INTV          (3 * 3600)// 除霜间隔时间 (3小时)
#define C_DEF_MAX_T         (45 * 60) // 最长允许加热时间 (45分钟)
#define C_DEF_EXIT_T        10.0f     // 除霜退出目标温度 (℃)
#define C_DRIP_TIME         (5 * 60)  // 滴水时长 (5分钟)
#define C_COMP_STEP_DLY     15        // 化霜后压机步进延时 (15秒)
#define C_DEF_DELAY_X       30        // 加热子程序特定延时 (秒)

/* --- 3. 风机控制参数 --- */
#define C_FAN_MODE_F1       1         // 蒸发风机运行模式 (1, 2, 4)
#define C_FAN_DLY_F3        30        // 蒸发风机启动延时 F3 (秒)
#define C_EVAP_T_OK         -15.0f    // 蒸发温度合格阈值 (℃)
#define C_COND_T_ON         35.0f     // 冷凝风机开启温度 (℃)
#define C_COND_T_OFF        25.0f     // 冷凝风机关闭温度 (℃)

/* --- 4. 变频器与电子膨胀阀 PID 参数 --- */
#define C_dT_MAX            5.0f      // 温差调频死区上限 Tmax
#define C_dT_MIN            1.0f      // 温差调频死区下限 Tmin
#define C_FREQ_MIN          20.0f     // 压缩机允许的最低频率 (Hz)
#define C_SH_MIN_LOW        6.5f      // 目标过热度 TP 下限底值
#define C_SH_MIN_HIGH       8.0f      // 目标过热度 TP 下限高值
#define C_HT_DIFF_TARGET    6.5f      // 传热温差 TCZ 目标值
#define C_HT_DIFF_TOL       1.5f      // 传热温差容差 (±1.5)
#define C_PID_ALPHA1        0.5f      // 膨胀阀比例系数 a1
#define C_PID_ALPHA2        0.8f      // 膨胀阀比例系数 a2
#define C_ETM_THRES         110.0f    // 排气温度 ETM 停机限制 (℃)

/* --- 5. 存储日志结构与 EEPROM 映射参数 --- */
#pragma pack(push, 1) // 强制 1 字节对齐，防止内存碎片导致存入EEPROM错位
typedef struct {
    // 1. 时间戳 (6 字节)
    uint8_t Year;     // 年 (例如 26 代表 2026)
    uint8_t Month;    // 月
    uint8_t Date;     // 日
    uint8_t Hours;    // 时
    uint8_t Minutes;  // 分
    uint8_t Seconds;  // 秒
    
    // 2. 核心业务数据 (12 字节)
    float   EvapTemp;   // 发生时的蒸发器温度 (4字节)
    float   SetTemp;    // 当时的设定温度 (4字节)
    float   CondTemp;   // 【新增】：发生时的冷凝/排气温度 (4字节)
    
    // 3. 事件类型与校验 (2 字节)
    uint8_t EventType;  // 事件码 (0=每日记录, 1=高温报警, 2=除霜记录)
    uint8_t CheckSum;   // 数据校验和
} SysLog_t;
#pragma pack(pop)

// ?? 【核心拆弹区】：跟着结构体的膨胀一起修改！
#define LOG_MAX_COUNT       100     // 【修改】：因为每条变大了，最大只能存 100 条了 (原127)
#define LOG_SIZE            20      // 【修改】：每条现在固定占用 20 字节 (原16)
#define EEPROM_INDEX_ADDR   0x0000  // 指针地址：存当前写到了第几条 (0x0000)
#define EEPROM_DATA_START   0x0010  // 数据区起始地址 (0x0010)

#endif /* SYS_CONFIG_H */


