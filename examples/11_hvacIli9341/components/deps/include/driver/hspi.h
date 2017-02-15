#ifndef INCLUDE_HSPI_H_
#define INCLUDE_HSPI_H_

#include "soc/spi_reg.h"
#include "driver/spi.h"
#include "driver/gpio.h"

#define SPI         0
#define HSPI        2
#define VSPI        3
#define SPIFIFOSIZE 16 //16 words length

extern uint32_t *spi_fifo;

extern void hspi_init(void);
extern void hspi_send_data(const uint8_t * data, uint8_t datasize);
extern void hspi_send_uint16_r(const uint16_t data, int32_t repeats);
inline void hspi_wait_ready(void){while (READ_PERI_REG(SPI_CMD_REG(HSPI))&SPI_USR);}

inline void hspi_prepare_tx(uint32_t bytecount)
{
    uint32_t bitcount = bytecount * 8 - 1;
    WRITE_PERI_REG(SPI_USER1_REG(HSPI), (bitcount & SPI_USR_MOSI_DBITLEN_V) << SPI_USR_MOSI_DBITLEN_S);
}


inline void hspi_start_tx()
{
    SET_PERI_REG_MASK(SPI_CMD_REG(HSPI), SPI_USR);   // send
}

inline void hspi_send_uint8(uint8_t data)
{
    spi_data_t spi_data;
    spi_data.addr = NULL;
    spi_data.addrLen = 0;
    spi_data.cmd = data;
    spi_data.cmdLen = 1;
    spi_data.rxData = NULL;
    spi_data.rxDataLen=0;
    spi_data.txData=NULL;
    spi_data.txDataLen = 0;
    spi_master_send_data(SpiNum_SPI2, &spi_data);
}

inline void hspi_send_uint16(uint16_t data)
{
    spi_data_t spi_data;
    spi_data.addr = NULL;
    spi_data.addrLen = 0;
    spi_data.cmd = data;
    spi_data.cmdLen = 2;
    spi_data.rxData = NULL;
    spi_data.rxDataLen=0;
    spi_data.txData=NULL;
    spi_data.txDataLen = 0;
    spi_master_send_data(SpiNum_SPI2, &spi_data);
}

inline void hspi_send_uint32(uint32_t data)
{
    spi_data_t spi_data;
    spi_data.addr = NULL;
    spi_data.addrLen = 0;
    spi_data.cmd = 0;
    spi_data.cmdLen = 0;
    spi_data.rxData = NULL;
    spi_data.rxDataLen=0;
    spi_data.txData=&data;
    spi_data.txDataLen = 4;
    spi_master_send_data(SpiNum_SPI2,&spi_data);
}

#endif /* INCLUDE_HSPI_H_ */
