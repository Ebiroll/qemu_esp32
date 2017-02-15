#include "driver/hspi.h"
#include "driver/spi.h"
#include "soc/gpio_sig_map.h"

#define HSPI_PRESCALER 4// target hspi clock speed is 40MHz/HSPI_PRESCALER, so that with prescaler 2 the hspi clock is 30MHz

#define __min(a,b) ((a > b) ? (b):(a))
uint32_t *spi_fifo;


void hspi_init(void)
{
    /*GPIO Inits according to Dev board for ILI9341*/
    spi_fifo = (uint32_t*)SPI_W0_REG(HSPI); //needed?


   // orig
    // PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO19_U, 2);  
    // PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO23_U, 2);  
    // PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO18_U, 2);  
    // PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, 2);   
           
   /* changed rudi */
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO25_U, 2);  //MISO GPIO19 // -> 25
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO23_U, 2);  //MOSI GPIO23 // -> 23
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO19_U, 2);  //CLK GPIO18  // -> 19
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO22_U, 2);   //CS GPIO5   // -> 22
    
    // orig
    //  gpio_matrix_in(GPIO_NUM_19, HSPIQ_IN_IDX,0);
    //  gpio_matrix_out(GPIO_NUM_23, HSPID_OUT_IDX,0,0);
    //  gpio_matrix_out(GPIO_NUM_18, HSPICLK_OUT_IDX,0,0);
    
     /* changed rudi */
    gpio_matrix_in(GPIO_NUM_25, HSPIQ_IN_IDX,0);
    gpio_matrix_out(GPIO_NUM_23, HSPID_OUT_IDX,0,0);
    gpio_matrix_out(GPIO_NUM_19, HSPICLK_OUT_IDX,0,0);
    
   // orig
   // gpio_matrix_out(GPIO_NUM_5, HSPICS0_OUT_IDX,0,0);
    
   /* changed rudi */
   gpio_matrix_out(GPIO_NUM_22, HSPICS0_OUT_IDX,0,0);
    

    spi_attr_t hSpiAttr;
    hSpiAttr.mode     = SpiMode_Master;
    hSpiAttr.subMode  = SpiSubMode_0;
    hSpiAttr.speed    = SpiSpeed_20MHz;
    hSpiAttr.bitOrder = SpiBitOrder_MSBFirst;
    hSpiAttr.halfMode = SpiWorkMode_Half;
    spi_init(HSPI, &hSpiAttr);

}

void hspi_send_uint16_r(uint16_t data, int32_t repeats)
{
    uint32_t i;
    uint32_t word = data << 16 | data;
    uint32_t word_tmp[16];

    while(repeats > 0)
    {
        uint16_t bytes_to_transfer = __min(repeats * sizeof(uint16_t) , SPIFIFOSIZE * sizeof(uint32_t));
        hspi_wait_ready();
        for(i = 0; i<(bytes_to_transfer + 3)/4; i++) {
            word_tmp[i] = word;
        }
        spi_data_t spi_data;
        spi_data.addr = NULL;
        spi_data.addrLen = 0;
        spi_data.cmd = 0;
        spi_data.cmdLen = 0;
        spi_data.rxData = NULL;
        spi_data.rxDataLen = 0;
        spi_data.txData = word_tmp;
        spi_data.txDataLen = bytes_to_transfer;
        spi_master_send_data(SpiNum_SPI2, &spi_data);
        repeats -= bytes_to_transfer / 2;
    }
}

void hspi_send_data(const uint8_t * data, uint8_t datasize)
{
    uint32_t *_data = (uint32_t*)data;
    //uint8_t words_to_send = __min((datasize + 3) / 4, SPIFIFOSIZE);
    spi_data_t spi_data;
    spi_data.addr = NULL;
    spi_data.addrLen = 0;
    spi_data.cmd = 0;
    spi_data.cmdLen = 0;
    spi_data.rxData = NULL;
    spi_data.rxDataLen = 0;
    spi_data.txData = _data;
    spi_data.txDataLen = datasize;
    spi_master_send_data(SpiNum_SPI2, &spi_data);
}
