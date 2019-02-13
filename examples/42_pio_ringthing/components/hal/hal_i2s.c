#include "freertos/FreeRTOS.h"
#include "driver/i2s.h"
#include "hal_i2s.h"
#include "soc/io_mux_reg.h"
#include "soc/soc.h"

void hal_i2s_init(uint8_t i2s_num,uint32_t rate,uint8_t bits,uint8_t ch)
{

	i2s_channel_fmt_t chanel;
	if(ch==2)
		chanel=I2S_CHANNEL_FMT_RIGHT_LEFT;
	else
		chanel=I2S_CHANNEL_FMT_ONLY_LEFT;

	i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX|I2S_MODE_RX,                    
        .sample_rate = rate,
        .bits_per_sample = bits,                                              
        .channel_format = chanel,                           //2-channels
        .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
        .dma_buf_count = 3,
        .dma_buf_len = 1024,                                                      //
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1                                //Interrupt level 1
    };
    i2s_pin_config_t pin_config = {
        .bck_io_num = 33,
        .ws_io_num = 25,
        .data_out_num = 26,
        .data_in_num = 27                                                       //Not used
    };
    i2s_driver_install(i2s_num, &i2s_config, 0, NULL);
    i2s_set_pin(i2s_num, &pin_config);
    //clk out
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);
    //REG_SET_FIELD(PIN_CTRL, CLK_OUT1, 0)
    REG_WRITE(PIN_CTRL, 0xFFFFFFF0);;
    //i2s_set_clk(i2s_num, rate, bits, ch);
}
int hal_i2s_read(uint8_t i2s_num,char* dest,size_t size,TickType_t timeout)
{
    return i2s_read_bytes(i2s_num,  dest, size, timeout);
}
int hal_i2s_write(uint8_t i2s_num,char* dest,size_t size,TickType_t timeout)
{
    return i2s_write_bytes(i2s_num,  dest, size, timeout);
}



