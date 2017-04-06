// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "esp32-hal-i2c.h"
//#include "esp32-hal.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "rom/ets_sys.h"
#include "soc/i2c_reg.h"
#include "soc/gpio_sig_map.h"
#include "driver/gpio.h"

#include "soc/i2c_struct.h"
#include "soc/dport_reg.h"
#include "Arduino.h"



//#define I2C_DEV(i)   (volatile i2c_dev_t *)((i)?DR_REG_I2C1_EXT_BASE:DR_REG_I2C_EXT_BASE)
//#define I2C_DEV(i)   ((i2c_dev_t *)(REG_I2C_BASE(i)))
#define I2C_SCL_IDX(p)  ((p==0)?I2CEXT0_SCL_OUT_IDX:((p==1)?I2CEXT1_SCL_OUT_IDX:0))
#define I2C_SDA_IDX(p) ((p==0)?I2CEXT0_SDA_OUT_IDX:((p==1)?I2CEXT1_SDA_OUT_IDX:0))

#define DR_REG_I2C_EXT_BASE_FIXED               0x60013000
#define DR_REG_I2C1_EXT_BASE_FIXED              0x60027000



#if CONFIG_DISABLE_HAL_LOCKS
#define I2C_MUTEX_LOCK()
#define I2C_MUTEX_UNLOCK()

static i2c_t _i2c_bus_array[2] = {
    {(volatile i2c_dev_t *)(DR_REG_I2C_EXT_BASE_FIXED), 0},
    {(volatile i2c_dev_t *)(DR_REG_I2C1_EXT_BASE_FIXED), 1}
};
#else
#define I2C_MUTEX_LOCK()    do {} while (xSemaphoreTake(i2c->lock, portMAX_DELAY) != pdPASS)
#define I2C_MUTEX_UNLOCK()  xSemaphoreGive(i2c->lock)

static i2c_t _i2c_bus_array[2] = {
    {(volatile i2c_dev_t *)(DR_REG_I2C_EXT_BASE), NULL, 0},
    {(volatile i2c_dev_t *)(DR_REG_I2C1_EXT_BASE), NULL, 1}
};
#endif

/**
 * @brief Configure GPIO signal for I2C sck and sda
 *
 * @param i2c_num I2C port number
 * @param sda_io_num GPIO number for I2C sda signal
 * @param scl_io_num GPIO number for I2C scl signal
 * @param sda_pullup_en Whether to enable the internal pullup for sda pin
 * @param scl_pullup_en Whether to enable the internal pullup for scl pin
 * @param mode I2C mode
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
#if 0
esp_err_t i2c_set_pin(i2c_port_t i2c_num, gpio_num_t sda_io_num, gpio_num_t scl_io_num, gpio_pullup_t sda_pullup_en, gpio_pullup_t scl_pullup_en, i2c_mode_t mode)
{
    I2C_CHECK(( i2c_num < I2C_NUM_MAX ), I2C_NUM_ERROR_STR, ESP_ERR_INVALID_ARG);
    I2C_CHECK(((GPIO_IS_VALID_OUTPUT_GPIO(sda_io_num))), I2C_SDA_IO_ERR_STR, ESP_ERR_INVALID_ARG);
    I2C_CHECK((GPIO_IS_VALID_OUTPUT_GPIO(scl_io_num)) ||
              (GPIO_IS_VALID_GPIO(scl_io_num) && mode == I2C_MODE_SLAVE),
              I2C_SCL_IO_ERR_STR,
              ESP_ERR_INVALID_ARG);
    I2C_CHECK((sda_pullup_en == GPIO_PULLUP_ENABLE && GPIO_IS_VALID_OUTPUT_GPIO(sda_io_num)) ||
               sda_pullup_en == GPIO_PULLUP_DISABLE, I2C_GPIO_PULLUP_ERR_STR, ESP_ERR_INVALID_ARG);
    I2C_CHECK((scl_pullup_en == GPIO_PULLUP_ENABLE && GPIO_IS_VALID_OUTPUT_GPIO(scl_io_num)) ||
               scl_pullup_en == GPIO_PULLUP_DISABLE, I2C_GPIO_PULLUP_ERR_STR, ESP_ERR_INVALID_ARG);

    int sda_in_sig, sda_out_sig, scl_in_sig, scl_out_sig;
    switch (i2c_num) {
        case I2C_NUM_1:
            sda_out_sig = I2CEXT1_SDA_OUT_IDX;
            sda_in_sig = I2CEXT1_SDA_IN_IDX;
            scl_out_sig = I2CEXT1_SCL_OUT_IDX;
            scl_in_sig = I2CEXT1_SCL_IN_IDX;
            break;
        case I2C_NUM_0:
            default:
            sda_out_sig = I2CEXT0_SDA_OUT_IDX;
            sda_in_sig = I2CEXT0_SDA_IN_IDX;
            scl_out_sig = I2CEXT0_SCL_OUT_IDX;
            scl_in_sig = I2CEXT0_SCL_IN_IDX;
            break;
    }
    if (sda_io_num >= 0) {
        gpio_set_level(sda_io_num, I2C_IO_INIT_LEVEL);
        PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[sda_io_num], PIN_FUNC_GPIO);
        gpio_set_direction(sda_io_num, GPIO_MODE_INPUT_OUTPUT_OD);
        if (sda_pullup_en == GPIO_PULLUP_ENABLE) {
            gpio_set_pull_mode(sda_io_num, GPIO_PULLUP_ONLY);
        } else {
            gpio_set_pull_mode(sda_io_num, GPIO_FLOATING);
        }
        gpio_matrix_out(sda_io_num, sda_out_sig, 0, 0);
        gpio_matrix_in(sda_io_num, sda_in_sig, 0);
    }

    if (scl_io_num >= 0) {
        gpio_set_level(scl_io_num, I2C_IO_INIT_LEVEL);
        PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[scl_io_num], PIN_FUNC_GPIO);
        if (mode == I2C_MODE_MASTER) {
            gpio_set_direction(scl_io_num, GPIO_MODE_INPUT_OUTPUT_OD);
            gpio_matrix_out(scl_io_num, scl_out_sig, 0, 0);
        } else {
            gpio_set_direction(scl_io_num, GPIO_MODE_INPUT);
        }
        if (scl_pullup_en == GPIO_PULLUP_ENABLE) {
            gpio_set_pull_mode(scl_io_num, GPIO_PULLUP_ONLY);
        } else {
            gpio_set_pull_mode(scl_io_num, GPIO_FLOATING);
        }
        gpio_matrix_in(scl_io_num, scl_in_sig, 0);
    }
    return ESP_OK;
}
#endif

#define I2C_IO_INIT_LEVEL          (1)


i2c_err_t i2cAttachSCL(i2c_t * i2c, int8_t scl_io_num)
{
    if(i2c == NULL){
        return I2C_ERROR_DEV;
    }

   int   scl_in_sig, scl_out_sig; // sda_in_sig,sda_out_sig

   switch (i2c->num) {
        case 1:
            //sda_out_sig = I2CEXT1_SDA_OUT_IDX;
            //sda_in_sig = I2CEXT1_SDA_IN_IDX;
            scl_out_sig = I2CEXT1_SCL_OUT_IDX;
            scl_in_sig = I2CEXT1_SCL_IN_IDX;
            break;
        case 0:
            //sda_out_sig = I2CEXT0_SDA_OUT_IDX;
            //sda_in_sig = I2CEXT0_SDA_IN_IDX;
            scl_out_sig = I2CEXT0_SCL_OUT_IDX;
            scl_in_sig = I2CEXT0_SCL_IN_IDX;
            break;
            default:
               return I2C_ERROR_OK;
    }


    gpio_set_level(scl_io_num, I2C_IO_INIT_LEVEL);
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[scl_io_num], PIN_FUNC_GPIO);
    if (true /*mode == I2C_MODE_MASTER*/) {
        gpio_set_direction(scl_io_num, GPIO_MODE_INPUT_OUTPUT_OD);
        gpio_matrix_out(scl_io_num, scl_out_sig, 0, 0);
    } else {
        gpio_set_direction(scl_io_num, GPIO_MODE_INPUT);
    }
    //if (scl_pullup_en == GPIO_PULLUP_ENABLE) {
    gpio_set_pull_mode(scl_io_num, GPIO_PULLUP_ONLY);
    //} else {
    //    gpio_set_pull_mode(scl_io_num, GPIO_FLOATING);
    //}
    gpio_matrix_in(scl_io_num, scl_in_sig, 0);

    //pinMode(scl, OUTPUT_OPEN_DRAIN | PULLUP);
    //pinMatrixOutAttach(scl, I2C_SCL_IDX(i2c->num), false, false);
    //pinMatrixInAttach(scl, I2C_SCL_IDX(i2c->num), false);
    return I2C_ERROR_OK;
}

i2c_err_t i2cDetachSCL(i2c_t * i2c, int8_t scl)
{
    if(i2c == NULL){
        return I2C_ERROR_DEV;
    }


    //pinMatrixOutDetach(scl, false, false);
    //pinMatrixInDetach(I2C_SCL_IDX(i2c->num), false, false);
    //pinMode(scl, INPUT);
    return I2C_ERROR_OK;
}

i2c_err_t i2cAttachSDA(i2c_t * i2c, int8_t sda_io_num)
{
    if(i2c == NULL){
        return I2C_ERROR_DEV;
    }

   int sda_in_sig, sda_out_sig; // scl_in_sig, scl_out_sig;

   switch (i2c->num) {
        case 1:
            sda_out_sig = I2CEXT1_SDA_OUT_IDX;
            sda_in_sig = I2CEXT1_SDA_IN_IDX;
            //scl_out_sig = I2CEXT1_SCL_OUT_IDX;
            //scl_in_sig = I2CEXT1_SCL_IN_IDX;
            break;
        case 0:
            sda_out_sig = I2CEXT0_SDA_OUT_IDX;
            sda_in_sig = I2CEXT0_SDA_IN_IDX;
            //scl_out_sig = I2CEXT0_SCL_OUT_IDX;
            //scl_in_sig = I2CEXT0_SCL_IN_IDX;
            break;
            default:
               return I2C_ERROR_OK;
    }

    gpio_set_level(sda_io_num, I2C_IO_INIT_LEVEL);
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[sda_io_num], PIN_FUNC_GPIO);
    gpio_set_direction(sda_io_num, GPIO_MODE_INPUT_OUTPUT_OD);
    //if (sda_pullup_en == GPIO_PULLUP_ENABLE) {
    gpio_set_pull_mode(sda_io_num, GPIO_PULLUP_ONLY);
    //} else {
    //    gpio_set_pull_mode(sda_io_num, GPIO_FLOATING);
    //}
    gpio_matrix_out(sda_io_num, sda_out_sig, 0, 0);
    gpio_matrix_in(sda_io_num, sda_in_sig, 0);

    //pinMode(sda, OUTPUT_OPEN_DRAIN | PULLUP);
    //pinMatrixOutAttach(sda, I2C_SDA_IDX(i2c->num), false, false);
    //pinMatrixInAttach(sda, I2C_SDA_IDX(i2c->num), false);
    return I2C_ERROR_OK;
}

i2c_err_t i2cDetachSDA(i2c_t * i2c, int8_t sda)
{
    if(i2c == NULL){
        return I2C_ERROR_DEV;
    }
    //pinMatrixOutDetach(sda, false, false);
    //pinMatrixInDetach(I2C_SDA_IDX(i2c->num), false, false);
    //pinMode(sda, INPUT);
    return I2C_ERROR_OK;
}

/*
 * index     - command index (0 to 15)
 * op_code   - is the command
 * byte_num  - This register is to store the amounts of data that is read and written. byte_num in RSTART, STOP, END is null.
 * ack_val   - Each data byte is terminated by an ACK bit used to set the bit level.
 * ack_exp   - This bit is to set an expected ACK value for the transmitter.
 * ack_check - This bit is to decide whether the transmitter checks ACK bit. 1 means yes and 0 means no.
 * */
void i2cSetCmd(i2c_t * i2c, uint8_t index, uint8_t op_code, uint8_t byte_num, bool ack_val, bool ack_exp, bool ack_check)
{
    i2c->dev->command[index].val = 0;
    i2c->dev->command[index].ack_en = ack_check;
    i2c->dev->command[index].ack_exp = ack_exp;
    i2c->dev->command[index].ack_val = ack_val;
    i2c->dev->command[index].byte_num = byte_num;
    i2c->dev->command[index].op_code = op_code;
}

void i2cResetCmd(i2c_t * i2c){
    int i;
    // This should not be necessary
    for(i=0;i<16;i++){
        i2c->dev->command[i].val = 0;
    }
}

void i2cResetFiFo(i2c_t * i2c)
{
    i2c->dev->fifo_conf.tx_fifo_rst = 1;
    i2c->dev->fifo_conf.tx_fifo_rst = 0;
    i2c->dev->fifo_conf.rx_fifo_rst = 1;
    i2c->dev->fifo_conf.rx_fifo_rst = 0;
}

i2c_err_t i2cWrite(i2c_t * i2c, uint16_t address, bool addr_10bit, uint8_t * data, uint8_t len, bool sendStop)
{
    int i;
    uint8_t index = 0;
    uint8_t dataLen = len + (addr_10bit?2:1);
    address = (address << 1);

    if(i2c == NULL){
        return I2C_ERROR_DEV;
    }

    I2C_MUTEX_LOCK();

    while(dataLen) {
        uint8_t willSend = (dataLen > 32)?32:dataLen;
        uint8_t dataSend = willSend;

        i2cResetFiFo(i2c);
        i2cResetCmd(i2c);
        //Clear Interrupts
        i2c->dev->int_clr.val = 0xFFFFFFFF;

        //CMD START
        i2cSetCmd(i2c, 0, I2C_CMD_RSTART, 0, false, false, false);

        //CMD WRITE(ADDRESS + DATA)
        if(!index) {
            i2c->dev->fifo_data.data = address & 0xFF;
            dataSend--;
            if(addr_10bit) {
                i2c->dev->fifo_data.data = (address >> 8) & 0xFF;
                dataSend--;
            }
        }
        i = 0;
        while(i<dataSend) {
            i++;
            i2c->dev->fifo_data.val = data[index++];
            while(i2c->dev->status_reg.tx_fifo_cnt < i);
        }
        i2cSetCmd(i2c, 1, I2C_CMD_WRITE, willSend, false, false, true);
        dataLen -= willSend;

        //CMD STOP or CMD END if there is more data
        if(dataLen || !sendStop) {
            i2cSetCmd(i2c, 2, I2C_CMD_END, 0, false, false, false);
        } else if(sendStop) {
            i2cSetCmd(i2c, 2, I2C_CMD_STOP, 0, false, false, false);
        }

        //START Transmission
        i2c->dev->ctr.trans_start = 1;

        //WAIT Transmission
        uint32_t startAt = millis();
        while(1) {
            //have been looping for too long
            if((millis() - startAt)>50){
                printf("Timeout! Addr: %x", address >> 1);
                I2C_MUTEX_UNLOCK();
                return I2C_ERROR_BUS;
            }

            //Bus failed (maybe check for this while waiting?
            if(i2c->dev->int_raw.arbitration_lost) {
                printf("Bus Fail! Addr: %x", address >> 1);
                I2C_MUTEX_UNLOCK();
                return I2C_ERROR_BUS;
            }

            //Bus timeout
            if(i2c->dev->int_raw.time_out) {
                printf("Bus Timeout! Addr: %x", address >> 1);
                I2C_MUTEX_UNLOCK();
                return I2C_ERROR_TIMEOUT;
            }

            //Transmission did not finish and ACK_ERR is set
            if(i2c->dev->int_raw.ack_err) {
                printf("Ack Error! Addr: %x", address >> 1);
                while(i2c->dev->status_reg.bus_busy);
                I2C_MUTEX_UNLOCK();
                return I2C_ERROR_ACK;
            }

            if((sendStop && i2c->dev->command[2].done) || !i2c->dev->status_reg.bus_busy){
                break;
            }
        }

    }
    I2C_MUTEX_UNLOCK();
    return I2C_ERROR_OK;
}

i2c_err_t i2cRead(i2c_t * i2c, uint16_t address, bool addr_10bit, uint8_t * data, uint8_t len, bool sendStop)
{
    address = (address << 1) | 1;
    uint8_t addrLen = (addr_10bit?2:1);
    uint8_t index = 0;
    uint8_t cmdIdx;
    uint8_t willRead;

    if(i2c == NULL){
        return I2C_ERROR_DEV;
    }

    I2C_MUTEX_LOCK();

    i2cResetFiFo(i2c);
    i2cResetCmd(i2c);

    //CMD START
    i2cSetCmd(i2c, 0, I2C_CMD_RSTART, 0, false, false, false);

    //CMD WRITE ADDRESS
    i2c->dev->fifo_data.val = address & 0xFF;
    if(addr_10bit) {
        i2c->dev->fifo_data.val = (address >> 8) & 0xFF;
    }
    i2cSetCmd(i2c, 1, I2C_CMD_WRITE, addrLen, false, false, true);

    while(len) {
        cmdIdx = (index)?0:2;
        willRead = (len > 32)?32:(len-1);
        if(cmdIdx){
            i2cResetFiFo(i2c);
        }

        if(willRead){
            i2cSetCmd(i2c, cmdIdx++, I2C_CMD_READ, willRead, false, false, false);
        }

        if((len - willRead) > 1) {
            i2cSetCmd(i2c, cmdIdx++, I2C_CMD_END, 0, false, false, false);
        } else {
            willRead++;
            i2cSetCmd(i2c, cmdIdx++, I2C_CMD_READ, 1, true, false, false);
            if(sendStop) {
                i2cSetCmd(i2c, cmdIdx++, I2C_CMD_STOP, 0, false, false, false);
            }
        }

        //Clear Interrupts
        i2c->dev->int_clr.val = 0xFFFFFFFF;

        //START Transmission
        i2c->dev->ctr.trans_start = 1;

        //WAIT Transmission
        uint32_t startAt = millis();
        while(1) {
            //have been looping for too long
            if((millis() - startAt)>50){
                //log_e("Timeout! Addr: %x", address >> 1);
                I2C_MUTEX_UNLOCK();
                return I2C_ERROR_BUS;
            }

            //Bus failed (maybe check for this while waiting?
            if(i2c->dev->int_raw.arbitration_lost) {
                //log_e("Bus Fail! Addr: %x", address >> 1);
                I2C_MUTEX_UNLOCK();
                return I2C_ERROR_BUS;
            }

            //Bus timeout
            if(i2c->dev->int_raw.time_out) {
                //log_e("Bus Timeout! Addr: %x", address >> 1);
                I2C_MUTEX_UNLOCK();
                return I2C_ERROR_TIMEOUT;
            }

            //Transmission did not finish and ACK_ERR is set
            if(i2c->dev->int_raw.ack_err) {
                //log_w("Ack Error! Addr: %x", address >> 1);
                I2C_MUTEX_UNLOCK();
                return I2C_ERROR_ACK;
            }

            if(i2c->dev->command[cmdIdx-1].done) {
                break;
            }
        }

        int i = 0;
        while(i<willRead) {
            i++;
            data[index++] = i2c->dev->fifo_data.val & 0xFF;
        }
        len -= willRead;
    }
    I2C_MUTEX_UNLOCK();
    return I2C_ERROR_OK;
}

i2c_err_t i2cSetFrequency(i2c_t * i2c, uint32_t clk_speed)
{
    if(i2c == NULL){
        return I2C_ERROR_DEV;
    }

    uint32_t period = (APB_CLK_FREQ/clk_speed) / 2;
    uint32_t halfPeriod = period/2;
    uint32_t quarterPeriod = period/4;

    I2C_MUTEX_LOCK();
    //the clock num during SCL is low level
    i2c->dev->scl_low_period.period = period;
    //the clock num during SCL is high level
    i2c->dev->scl_high_period.period = period;

    //the clock num between the negedge of SDA and negedge of SCL for start mark
    i2c->dev->scl_start_hold.time = halfPeriod;
    //the clock num between the posedge of SCL and the negedge of SDA for restart mark
    i2c->dev->scl_rstart_setup.time = halfPeriod;

    //the clock num after the STOP bit's posedge
    i2c->dev->scl_stop_hold.time = halfPeriod;
    //the clock num between the posedge of SCL and the posedge of SDA
    i2c->dev->scl_stop_setup.time = halfPeriod;

    //the clock num I2C used to hold the data after the negedge of SCL.
    i2c->dev->sda_hold.time = quarterPeriod;
    //the clock num I2C used to sample data on SDA after the posedge of SCL
    i2c->dev->sda_sample.time = quarterPeriod;
    I2C_MUTEX_UNLOCK();
    return I2C_ERROR_OK;
}

uint32_t i2cGetFrequency(i2c_t * i2c)
{
    if(i2c == NULL){
        return 0;
    }

    return APB_CLK_FREQ/(i2c->dev->scl_low_period.period+i2c->dev->scl_high_period.period);
}

/*
 * mode          - 0 = Slave, 1 = Master
 * slave_addr    - I2C Address
 * addr_10bit_en - enable slave 10bit address mode.
 * */

i2c_t * i2cInit(uint8_t i2c_num, uint16_t slave_addr, bool addr_10bit_en)
{
    if(i2c_num > 1){
        return NULL;
    }

    i2c_t * i2c = &_i2c_bus_array[i2c_num];

#if !CONFIG_DISABLE_HAL_LOCKS
    if(i2c->lock == NULL){
        i2c->lock = xSemaphoreCreateMutex();
        if(i2c->lock == NULL) {
            return NULL;
        }
    }
#endif

    if(i2c_num == 0) {
        SET_PERI_REG_MASK(DPORT_PERIP_CLK_EN_REG,DPORT_I2C_EXT0_CLK_EN);
        CLEAR_PERI_REG_MASK(DPORT_PERIP_RST_EN_REG,DPORT_I2C_EXT0_RST);
    } else {
        SET_PERI_REG_MASK(DPORT_PERIP_CLK_EN_REG,DPORT_I2C_EXT1_CLK_EN);
        CLEAR_PERI_REG_MASK(DPORT_PERIP_RST_EN_REG,DPORT_I2C_EXT1_RST);
    }
    
    I2C_MUTEX_LOCK();
    i2c->dev->ctr.val = 0;
    i2c->dev->ctr.ms_mode = (slave_addr == 0);
    i2c->dev->ctr.sda_force_out = 1 ;
    i2c->dev->ctr.scl_force_out = 1 ;
    i2c->dev->ctr.clk_en = 1;

    //the max clock number of receiving  a data
    i2c->dev->timeout.tout = 400000;//clocks max=1048575
    //disable apb nonfifo access
    i2c->dev->fifo_conf.nonfifo_en = 0;

    i2c->dev->slave_addr.val = 0;
    if (slave_addr) {
        i2c->dev->slave_addr.addr = slave_addr;
        i2c->dev->slave_addr.en_10bit = addr_10bit_en;
    }
    I2C_MUTEX_UNLOCK();

    return i2c;
}

void i2cInitFix(i2c_t * i2c){
    if(i2c == NULL){
        return;
    }
    I2C_MUTEX_LOCK();
    i2cResetFiFo(i2c);
    i2cResetCmd(i2c);
    i2c->dev->int_clr.val = 0xFFFFFFFF;
    i2cSetCmd(i2c, 0, I2C_CMD_RSTART, 0, false, false, false);
    i2c->dev->fifo_data.data = 0;
    i2cSetCmd(i2c, 1, I2C_CMD_WRITE, 1, false, false, false);
    i2cSetCmd(i2c, 2, I2C_CMD_STOP, 0, false, false, false);
    i2c->dev->ctr.trans_start = 1;
    while(!i2c->dev->command[2].done);
    I2C_MUTEX_UNLOCK();
}
