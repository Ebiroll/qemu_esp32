/*
 *
 * Written by Olof Astrand <olof.astrand@gmail.com>
 *
 * This example code is in the Public Domain (or CC0 licensed, at your option.)
 * Unless required by applicable law or agreed to in writing, this
 * software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
 *
*/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "driver/gpio.h"
#include "sdkconfig.h"
#include "driver/i2c.h"

#include "esp32_i2c.h"

// Use same inputs as sparkfun, arduino

#define I2C_MASTER_SCL_IO    22    /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO    21    /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM   I2C_NUM_0   /*!< I2C port number for master dev */
#define I2C_MASTER_TX_BUF_DISABLE   0   /*!< I2C master do not need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0   /*!< I2C master do not need buffer */
#define I2C_MASTER_FREQ_HZ    100000     /*!< I2C master clock frequency */

#define WRITE_BIT  I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT   I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN   0x1     /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS  0x0     /*!< I2C master will not check ack from slave */
#define ACK_VAL    0x0         /*!< I2C ack value */
#define NACK_VAL   0x1         /*!< I2C nack value */



/**
 * @brief test function to show buffer
 */
void show_buf(uint8_t* buf, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        printf("%02x ", buf[i]);
        if (( i + 1 ) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");
}


/**
 *
 * _______________________________________________________________________________________
 * | start | slave_addr  +ack | 
 * --------|--------------------------|----------------------|--------------------|------|
 *
 */
esp_err_t i2c_master_check_slave(i2c_port_t i2c_num,uint8_t addr)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ( addr << 1 ) , ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

i2c_port_t g_1306_port = 0;
uint8_t g_addr=0x3C;
/**
 * @brief i2c master initialization
 */
void i2c_init(i2c_port_t i2c_num,uint8_t addr)
{
    g_1306_port =i2c_num;
    g_addr=addr;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(i2c_num, &conf);
    i2c_driver_install(i2c_num, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}



/**
 * @brief Scan all i2c adresses 
 */
void i2c_scan() {
	int address;
    int ret;
	int foundCount = 0;
	for (address=1; address<127; address++) {
        ret=i2c_master_check_slave(I2C_MASTER_NUM,address);
        if (ret == ESP_OK) {
            printf("Found device addres: %02x\n", address);
            foundCount++;
        }
    }
    printf("Done scanning.. found %d devices\n",foundCount);
}


//
//
#define BLINK_GPIO 5

void blink_task(void *pvParameters)
{
    gpio_pad_select_gpio(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);


	while (1) {
          gpio_set_level(BLINK_GPIO, 0);
          vTaskDelay(1000 / portTICK_RATE_MS);
          gpio_set_level(BLINK_GPIO, 1);
          vTaskDelay(1000 / portTICK_RATE_MS);
          gpio_set_level(BLINK_GPIO, 0);
          vTaskDelay(1000 / portTICK_RATE_MS);
          gpio_set_level(BLINK_GPIO, 1);
          vTaskDelay(1000 / portTICK_RATE_MS);
	}
}


/**
 *   https://www.olimex.com/Products/Modules/LCD/MOD-OLED-128x64/resources/SSD1306.pdf
 * @brief  write data to 1306 device
 * Data start with 0x80 then the stream of command and arguments
 * ______________________________________________________________________________________
 * | start | slave_addr + wr_bit + ack | write 1 byte + ack  ... write 1 byte + nack | stop |
 * --------|---------------------------|---------------------|--------------------|------|
 */
esp_err_t i2c_1306_write_command( uint8_t* data,int len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, g_addr << 1 | WRITE_BIT, ACK_CHECK_EN);
    // Not sure if cheap chinese displays will ack, otherwise ACK_CHECK_EN would be better
    i2c_master_write_byte(cmd, 0x40, ACK_CHECK_DIS);
    i2c_master_write(cmd,  data,len,ACK_CHECK_DIS);
    i2c_master_stop(cmd);
    // Try twice if failed first attempt
    int ret = i2c_master_cmd_begin(g_1306_port, cmd, 1000 / portTICK_RATE_MS);
    if (ret==ESP_FAIL) {
       ret = i2c_master_cmd_begin(g_1306_port, cmd, 1000 / portTICK_RATE_MS);
    }
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_FAIL) {
        return ret;
    }
    return ESP_OK;


}


/**
 *   https://www.olimex.com/Products/Modules/LCD/MOD-OLED-128x64/resources/SSD1306.pdf
 * @brief  write data to 1306 device
 * Data start with 0x40 then the stream of data
 * ______________________________________________________________________________________
 * | start | slave_addr + wr_bit + ack | write 1 byte + ack  ... write 1 byte + nack | stop |
 * --------|---------------------------|---------------------|--------------------|------|
 */
esp_err_t i2c_1306_write_data(const uint8_t* data,int len)
{
    int times_tried=0;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, g_addr << 1 | WRITE_BIT, ACK_CHECK_EN);
    // Not sure if cheap chinese displays will ack, otherwise ACK_CHECK_EN would be better
    i2c_master_write_byte(cmd, 0x40, ACK_CHECK_DIS);
    i2c_master_write(cmd,  data,len,ACK_CHECK_DIS);
    i2c_master_stop(cmd);
    int ret = i2c_master_cmd_begin(g_1306_port, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_FAIL) {
        return ret;
    }
    return ESP_OK;
}
