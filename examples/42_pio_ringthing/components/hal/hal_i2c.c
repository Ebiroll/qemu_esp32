#include "hal_i2c.h"
#include "driver/i2c.h"

#define ACK_CHECK_EN   0x1     /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS  0x0     /*!< I2C master will not check ack from slave */
#define ACK_VAL    0x0         /*!< I2C ack value */
#define NACK_VAL   0x1         /*!< I2C nack value */

#define I2C_EXAMPLE_MASTER_TX_BUF_DISABLE   0   /*!< I2C master do not need buffer */
#define I2C_EXAMPLE_MASTER_RX_BUF_DISABLE   0   /*!< I2C master do not need buffer */


void hal_i2c_init(uint8_t port ,uint8_t sda,uint8_t scl)
{
	int i2c_master_port = (i2c_port_t)port;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = sda;
    conf.sda_pullup_en = GPIO_PULLDOWN_DISABLE;
    conf.scl_io_num = scl;
    conf.scl_pullup_en = GPIO_PULLDOWN_DISABLE;
    conf.master.clk_speed = 100000;
    i2c_param_config(i2c_master_port, &conf);
    i2c_driver_install(i2c_master_port, conf.mode,
                       I2C_EXAMPLE_MASTER_RX_BUF_DISABLE,
                       I2C_EXAMPLE_MASTER_TX_BUF_DISABLE, 0);
}
esp_err_t hal_i2c_master_mem_write(i2c_port_t i2c_num, uint8_t DevAddr,uint8_t MemAddr,uint8_t* data_wr, size_t size)
{
	 if (size == 0) {
        return ESP_OK;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();//a cmd list
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ( DevAddr << 1 ) | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, MemAddr, ACK_CHECK_EN);
    i2c_master_write(cmd, data_wr, size, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}
esp_err_t hal_i2c_master_mem_read(i2c_port_t i2c_num, uint8_t DevAddr,uint8_t MemAddr,uint8_t* data_rd, size_t size)
{
	 if (size == 0) {
        return ESP_OK;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();//a cmd list
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ( DevAddr << 1 ) | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, MemAddr, ACK_CHECK_EN);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ( DevAddr << 1 ) | I2C_MASTER_READ, ACK_CHECK_EN);
    if (size > 1) {
        i2c_master_read(cmd, data_rd, size - 1, ACK_VAL);
    }
    i2c_master_read_byte(cmd, data_rd + size - 1, NACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

