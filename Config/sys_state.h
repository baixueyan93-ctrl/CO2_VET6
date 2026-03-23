#ifndef SYS_STATE_H
#define SYS_STATE_H

#include "FreeRTOS.h"
#include "event_groups.h"
#include "semphr.h"
#include <stdint.h>
#include <stdbool.h>

/* ===========================================================================
 * CO2 ����ϵͳ �� ���� / ״̬��� / ���� (��Դ: �߼�ͼ 1~6)
 *
 * ��������:
 *   VAR_   = ��������ֵ (���������� / �����м���)
 *   ST_    = ״̬���   (bool ��, ������ǰϵͳ״̬)
 *   TMR_   = ��ʱ������ (�ɶ�ʱ�жϵ���, ��ʱ����λ��Ӧ ST_ ���)
 *   ERR_   = ����       (����, ��ͣ����֪ͨ�û�)
 *   WARN_  = ����       (��ʱ�ɺ���, ����������Ϊ����֪ͨ�û�)
 * =========================================================================== */

/* ===================================================================
 *  ��һ����: ��������ֵ (VAR_) �� �������ɼ� + �����м���
 *  ��Դ: ͼ1/ͼ2/ͼ3/ͼ4/ͼ6
 * =================================================================== */
typedef struct {
    /* --- ͼ1: �¶ȿ������� �ɼ��� --- */
    float VAR_CABINET_TEMP;     /* Tc/T1  �������¶�(����) (��C)           */
    float VAR_EXHAUST_TEMP;     /* TH     ѹ���������¶� (��C)             */
    float VAR_SUCTION_TEMP;     /* TL     �����¶� (��C)                   */
    float VAR_COMP_FREQ;        /* F      ѹ������ǰ����Ƶ�� (Hz)          */
    float VAR_SUCTION_PRES;     /* PL     ����ѹ�� (bar)                  */
    float VAR_DISCHARGE_PRES;   /* PH     ����ѹ�� (bar)                  */
    float VAR_EXV_OPENING;      /* Kp     ���ͷ����� (��/%)               */
    float VAR_VDC_VOLTAGE;      /* VDC    ��Դ��ѹ (V)                    */
    float VAR_AMBIENT_TEMP;     /* �����¶� (��C), �ͿǼ���/���������     */

    /* --- ͼ2: ��˪���� + ͼ3: ������� �ɼ��� --- */
    float VAR_EVAP_TEMP;        /* �������¶� (��C), ��˪�ж�+����ж�     */

    /* --- ͼ4: PID/���ͷ� �����м��� --- */
    float VAR_DELTA_T;          /* ��T = T1(����) - Ts(�趨�¶�) (��C)     */
    float VAR_SUPERHEAT;        /* ��TP   ���ȶ� (��C)                     */
    float VAR_HT_DIFF;          /* ��TCZ  �����²� = ���� - �����¶� (��C) */

    /* --- ͼ6: ������� �ɼ��� --- */
    float VAR_COND_TEMP;        /* �����¶� (��C)                          */

    /* --- Һλ������ --- */
    uint8_t VAR_LIQUID_LEVEL;   /* Һλ״̬ (1:��ˮ, 0:ûˮ)              */

    /* --- SHT30 ������ʪ�ȴ����� (I2C1 ����, ��ַ 0x44) --- */
    float VAR_SHT30_TEMP;       /* SHT30 �����¶� (��C)                    */
    float VAR_SHT30_HUMI;       /* SHT30 ���ʪ�� (% RH)                  */

    /* --- 硬件输入信号 (由BSP层采集写入, 逻辑层只读) --- */
    bool  HW_VAC_PHASE_FAULT;   /* VAC 错/断相 (true=异常)                 */
    bool  HW_INV_OVERCURRENT;   /* 变频器过流   (true=异常)                 */
    bool  HW_INV_OVERHEAT;      /* 变频器过热   (true=异常)                 */
} SysVarData_t;


/* ===================================================================
 *  �ڶ�����: ״̬��� (ST_) �� �� FreeRTOS EventGroup λ
 *  ��Դ: ͼ1/ͼ2/ͼ3/ͼ5/ͼ6
 *
 *  ʹ�÷���:
 *    ��λ: xEventGroupSetBits(SysEventGroup, ST_xxx);
 *    ���: xEventGroupClearBits(SysEventGroup, ST_xxx);
 *    ��ȡ: xEventGroupGetBits(SysEventGroup) & ST_xxx
 * =================================================================== */

/* --- ͼ1: �¶ȿ��� / ѹ����״̬ --- */
#define ST_COMP_RUNNING         (1 <<  0)  /* ѹ�������ڹ���                */
#define ST_SYSTEM_ON            (1 <<  1)  /* ϵͳ����״̬                  */
#define ST_FIRST_RUN            (1 <<  2)  /* �״�����(ͨ����һ������)    */
#define ST_OIL_HEAT_ON          (1 <<  3)  /* �ͿǼ����ѿ���                */
#define ST_WARMUP_DONE          (1 <<  4)  /* �ȳ�ʱ��C20�ѵ�ʱ             */

/* --- ͼ2: ��˪״̬ --- */
#define ST_DEFROST_ACTIVE       (1 <<  5)  /* ���ڳ�˪(�ܱ��)              */
#define ST_DEF_HEATING          (1 <<  6)  /* ���ڼ���(��˪���Ƚ׶�)        */
#define ST_DEF_DRIPPING         (1 <<  7)  /* ���ڵ�ˮ(��˪��ˮ�׶�)        */

/* --- ͼ3: �������״̬ --- */
#define ST_EVAP_FAN_ON          (1 <<  8)  /* ���������������              */
#define ST_EVAP_TEMP_QUAL       (1 <<  9)  /* �����¶Ⱥϸ�                  */

/* --- ͼ4: PID/��Ƶ״̬ --- */
#define ST_DT_SHRINKING         (1 << 10)  /* ��T������С(��Ƶ����)          */

/* --- ͼ6: �������״̬ --- */
#define ST_COND_FAN1_ON         (1 << 11)  /* �������1��������             */
#define ST_COND_FAN2_ON         (1 << 12)  /* �������2��������             */
#define ST_COND_FAN3_ON         (1 << 13)  /* �������3��������             */

/* ===================================================================
 *  ��������: ��ʱ��� (ST_TMR_) �� ��ͼ5��ʱ�ж���λ
 *  ʹ�õڶ��� EventGroup, ��Ϊλ�����ܲ���
 * =================================================================== */
#define ST_TMR_TICK_1S          (1 <<  0)  /* 1�붨ʱ��                     */
#define ST_TMR_PID_DONE         (1 <<  1)  /* PID����(30s)��ʱ              */
#define ST_TMR_C2_DONE          (1 <<  2)  /* ͣ������ʱ��C2��ʱ            */
#define ST_TMR_C3_DONE          (1 <<  3)  /* ͨ���ӳ�C3��ʱ                */
#define ST_TMR_C7_DONE          (1 <<  4)  /* ��������C7��ʱ                */
#define ST_TMR_C8_DONE          (1 <<  5)  /* �������C8��ʱ                */
#define ST_TMR_DEF_INTV_DONE    (1 <<  6)  /* ��˪�����ʱ                  */
#define ST_TMR_DEF_DUR_DONE     (1 <<  7)  /* ��˪����ʱ����ʱ              */
#define ST_TMR_DEF_DRIP_DONE    (1 <<  8)  /* ��ˮʱ�䵽ʱ                  */
#define ST_TMR_WARMUP_DONE      (1 <<  9)  /* �ȳ�ʱ��C20��ʱ               */
#define ST_TMR_EVAP_FAN_DLY     (1 << 10)  /* �������F3��ʱ��ʱ            */

/* ===================================================================
 *  ���Ĳ���: ��ʱ������ (TMR_) �� �ڶ�ʱ�ж��е���
 * =================================================================== */
typedef struct {
    uint32_t TMR_TICK_1S_CNT;       /* 1���׼����                   */
    uint32_t TMR_PID_CNT;           /* PID���ڼ���                   */
    uint32_t TMR_C2_CNT;            /* ͣ��ʱ������                  */
    uint32_t TMR_C3_CNT;            /* ͨ���ӳټ���                  */
    uint32_t TMR_C7_CNT;            /* ������������                  */
    uint32_t TMR_C8_CNT;            /* ������м���                  */
    uint32_t TMR_DEF_INTV_CNT;      /* ��˪�������                  */
    uint32_t TMR_DEF_DUR_CNT;       /* ��˪����ʱ������              */
    uint32_t TMR_DRIP_CNT;          /* ��ˮʱ�����                  */
    uint32_t TMR_WARMUP_CNT;        /* �ȳ�ʱ������                  */
    uint32_t TMR_EVAP_FAN_DLY_CNT;  /* ���������ʱ����              */
    uint32_t TMR_LONGRUN_CNT;       /* ��������ʱ������(������)      */
    uint32_t TMR_PRES_HIGH_CNT;     /* ��ѹ��ʱ����                  */
    uint32_t TMR_PRES_LOW_CNT;      /* ��ѹ��ʱ����                  */
} SysTimerData_t;

/* ===================================================================
 *  ���岿��: ������־ (ERR_ / WARN_) �� ��λ����, ����һ�� uint32_t
 *  ��Դ: ͼ1 �澯�߼� + ͼ4 PID�߼�
 *
 *  ERR_ = ���� (����, ����ͣ��, ����֪ͨ�û�)
 *  WARN_ = ���� (��ʱ������, ����ʱ�䳤������֪ͨ�û�)
 *
 *  ʹ�÷���:
 *    ��λ: g_AlarmFlags |= ERR_xxx;
 *    ���: g_AlarmFlags &= ~ERR_xxx;
 *    ���: if (g_AlarmFlags & ERR_xxx) { ... }
 * =================================================================== */

/* --- ���� (Error): ����ͣ�� + ֪ͨ�û� --- */
#define ERR_SENSOR_CABINET      (1U <<  0) /* E1   ���´���������           */
#define ERR_VDC_LOW             (1U <<  1) /* EDC  ��Դ��ѹ����             */
#define ERR_VAC_PHASE           (1U <<  2) /* EAC  VAC����/����             */
#define ERR_INV_OVERCURR        (1U <<  3) /* EFI  ��Ƶ������               */
#define ERR_INV_OVERHEAT        (1U <<  4) /* EFT  ��Ƶ������               */
#define ERR_TEMP_LOW_STOP       (1U <<  5) /* ETM  Ƶ�ʹ����¶��쳣ͣ��     */

/* --- ���� (Warning): ��ʱ����, ������֪ͨ�û� --- */
#define WARN_EXHAUST_HIGH       (1U <<  8) /* WTM  �����¶ȹ���             */
#define WARN_SUCTION_LOW        (1U <<  9) /* WTL  �����¶ȹ���(����1-2h)   */
#define WARN_PRES_HIGH          (1U << 10) /* WPH1 ����ѹ������             */
#define WARN_PRES_HIGH_TMO      (1U << 11) /* WPH2 ��ѹ��ʱ������֪ͨ       */
#define WARN_PRES_LOW           (1U << 12) /* WPL1 ����ѹ������             */
#define WARN_PRES_LOW_TMO       (1U << 13) /* WPL2 ��ѹ��ʱ������֪ͨ       */
#define WARN_LONGRUN            (1U << 14) /* WLC  ��������ʱ�����         */
#define WARN_SUPERHEAT_LOW      (1U << 15) /* EDT  ���ȶȹ���               */
#define WARN_NOTIFY_USER        (1U << 16) /* WEN  �ۺϸ澯-��֪ͨ�û�      */

/* --- ����/�������� (���������ж�) --- */
#define ERR_MASK_ALL            (0x0000003FU)  /* ����ERRλ                 */
#define WARN_MASK_ALL           (0x0001FF00U)  /* ����WARNλ                */

/* ===================================================================
 *  �ⲿȫ�ֱ�������
 * =================================================================== */
extern EventGroupHandle_t SysEventGroup;      /* ϵͳ״̬����¼���         */
extern EventGroupHandle_t SysTimerEventGroup;  /* ��ʱ����¼���            */
extern SemaphoreHandle_t  SensorDataMutex;     /* ���������ݻ�����          */

extern volatile uint32_t  g_AlarmFlags;        /* ������־��                */
extern SysTimerData_t     g_TimerData;         /* ��ʱ������                */

/* ===================================================================
 *  �����ӿ�����
 * =================================================================== */
void SysState_Init(void);
void SysState_UpdateSensor(SysVarData_t* newData);
void SysState_GetSensor(SysVarData_t* outData);
/* ������ԭ�Ӽ���ȫ�����ӿ� */
void SysState_Lock(void);
void SysState_Unlock(void);
SysVarData_t* SysState_GetRawPtr(void);
#endif /* SYS_STATE_H */



