#ifndef _HAL_I2C_H
#define _HAL_I2C_H

#include "driver/i2c.h"
esp_err_t hal_i2c_master_mem_read(i2c_port_t i2c_num, uint8_t DevAddr,uint8_t MemAddr,uint8_t* data_rd, size_t size);
esp_err_t hal_i2c_master_mem_write(i2c_port_t i2c_num, uint8_t DevAddr,uint8_t MemAddr,uint8_t* data_wr, size_t size);
void hal_i2c_init(uint8_t port ,uint8_t sda,uint8_t scl);

#endif

