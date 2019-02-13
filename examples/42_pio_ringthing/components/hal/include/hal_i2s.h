#ifndef HAL_I2S_H
#define HAL_I2S_H


#include "driver/i2s.h"

void hal_i2s_init(uint8_t i2s_num,uint32_t rate,uint8_t bits,uint8_t ch);
int hal_i2s_read(uint8_t i2s_num,char* dest,size_t size,TickType_t timeout);
int hal_i2s_write(uint8_t i2s_num,char* dest,size_t size,TickType_t timeout);
#endif





