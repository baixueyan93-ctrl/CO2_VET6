#ifndef __BSP_I2C_MUTEX_H
#define __BSP_I2C_MUTEX_H

#include "FreeRTOS.h"
#include "semphr.h"

/* I2C1 总线互斥锁 (SHT30 + EEPROM 共用) */
void BSP_I2C1_MutexInit(void);
void BSP_I2C1_Lock(void);
void BSP_I2C1_Unlock(void);

#endif /* __BSP_I2C_MUTEX_H */
