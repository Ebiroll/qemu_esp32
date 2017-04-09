

#ifndef _1306ESP_I2C_H_
#define _1306ESP_I2C_H_

#include "driver/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

void i2c_init(i2c_port_t i2c_num,uint8_t addr);

esp_err_t i2c_1306_write_data(const uint8_t* data,int len);

esp_err_t i2c_1306_write_command(uint8_t* data,int len);

void i2c_scan();


#ifdef __cplusplus
}
#endif


#endif