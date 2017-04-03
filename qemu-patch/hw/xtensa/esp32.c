/*
 * Copyright (c) 2016, Max Filippov, Open Source and Linux Lab.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Open Source and Linux Lab nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */ 



#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu-common.h"
#include "cpu.h"
#include "sysemu/sysemu.h"
#include "hw/boards.h"
#include "hw/loader.h"
#include "elf.h"
#include "exec/memory.h"
#include "exec/address-spaces.h"
#include "hw/char/serial.h"
#include "net/net.h"
#include "hw/sysbus.h"
#include "hw/block/flash.h"
#include "sysemu/block-backend.h"
#include "sysemu/char.h"
#include "sysemu/device_tree.h"
#include "qemu/error-report.h"
#include "bootparam.h"
#include "qemu/timer.h"
#include "inttypes.h"
#include "hw/isa/isa.h"
//#include "hw/i2c/i2c_esp32.h"
#include <poll.h>
#include <error.h>

// From Memorymapped.cpp
extern const unsigned char* get_flashMemory();


typedef struct Esp32 {
    XtensaCPU *cpu[2];
} Esp32;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


typedef struct connect_data {
    int socket;
    int uart_num;
} connect_data;

qemu_irq uart0_irq;

void *connection_handler(void *connect);
void *gdb_socket_thread(void *dummy);


#define DEBUG_LOG(...) fprintf(stdout, __VA_ARGS__)

#define DEFINE_BITS(prefix, reg, field, shift, len) \
    prefix##_##reg##_##field##_SHIFT = shift, \
    prefix##_##reg##_##field##_LEN = len, \
    prefix##_##reg##_##field = ((~0U >> (32 - (len))) << (shift))

/* Serial */

enum {
    ESP32_UART_FIFO,
    ESP32_UART_INT_RAW,
    ESP32_UART_INT_ST,
    ESP32_UART_INT_ENA,
    ESP32_UART_INT_CLR,
    ESP32_UART_CLKDIV,
    ESP32_UART_AUTOBAUD,
    ESP32_UART_STATUS,    //  ESP32_UART_txfifo_cnt = 0x1c/4,
    ESP32_UART_CONF0,
    ESP32_UART_CONF1,
    ESP32_UART_LOWPULSE,
    ESP32_UART_HIGHPULSE,
    ESP32_UART_RXD_CNT,
    ESP32_UART_DATE = 0x78 / 4,
    ESP32_UART_ID,
    ESP32_UART_MAX,
};

#define ESP32_UART_BITS(reg, field, shift, len) \
    DEFINE_BITS(ESP32_UART, reg, field, shift, len)

enum {
    ESP32_UART_BITS(INT, RXFIFO_FULL, 0, 1),
    ESP32_UART_BITS(INT, TXFIFO_EMPTY, 1, 1),
    ESP32_UART_BITS(INT, RXFIFO_OVF, 4, 1),
};

enum {
    ESP32_UART_BITS(CONF0, LOOPBACK, 14, 1),
    ESP32_UART_BITS(CONF0, RXFIFO_RST, 17, 1),
};

enum {
    ESP32_UART_BITS(CONF1, RXFIFO_FULL, 0, 7),
};

#define ESP32_UART_GET(s, _reg, _field) \
    extract32(s->reg[ESP32_UART_##_reg], \
              ESP32_UART_##_reg##_##_field##_SHIFT, \
              ESP32_UART_##_reg##_##_field##_LEN)

#define ESP32_UART_FIFO_SIZE 0x80
#define ESP32_UART_FIFO_MASK 0x7f

#define MAX_GDB_BUFF  4096

typedef struct Esp32SerialState {
    MemoryRegion    iomem;
    CharBackend     chr;
    ChardevFeature  feature;
    qemu_irq irq;

    unsigned rx_first;
    unsigned rx_last;
    uint8_t rx[ESP32_UART_FIFO_SIZE];
    uint32_t reg[ESP32_UART_MAX];

    // These are used to interact with the socket thread
    int  uart_num;
    int  guard;
    char gdb_serial_data[MAX_GDB_BUFF];
    int  gdb_serial_buff_rd;
    int  gdb_serial_buff_tx;
    bool gdb_serial_connected;

} Esp32SerialState;


CharBackend* silly_serial=NULL;

static unsigned esp32_serial_rx_fifo_size(Esp32SerialState *s)
{
    return (s->rx_last - s->rx_first) & ESP32_UART_FIFO_MASK;
}

static bool esp32_serial_can_receive(void *opaque)
{
    Esp32SerialState *s = opaque;
    //if (esp32_serial_rx_fifo_size(s) > (ESP32_UART_FIFO_SIZE - 1))
    //  fprintf(stderr,"FULL\n");

    //fprintf(stderr,"FIFO SIZE %d\n",esp32_serial_rx_fifo_size(s));

    return esp32_serial_rx_fifo_size(s) < (ESP32_UART_FIFO_SIZE - 1);
}

static void esp32_serial_irq_update(Esp32SerialState *s)
{
    s->reg[ESP32_UART_INT_ST] |= s->reg[ESP32_UART_INT_RAW];
    //fprintf(stderr,"CHECKING IRQ\n");

    if (s->uart_num==0 || (s->reg[ESP32_UART_INT_ST] & s->reg[ESP32_UART_INT_ENA])) {
        //fprintf(stderr,"RAISING IRQ\n");
        if (s->uart_num==0) {
           qemu_irq_raise(uart0_irq);
        }
        else
        {
           qemu_irq_raise(s->irq);
        }
    } else {
        if (s->uart_num==0) {
           qemu_irq_lower(uart0_irq);
        }
        else
        {
           qemu_irq_lower(s->irq);
        }
    }
}

static void esp32_serial_rx_irq_update(Esp32SerialState *s)
{
    if (esp32_serial_rx_fifo_size(s) >= ESP32_UART_GET(s, CONF1, RXFIFO_FULL)) {
        s->reg[ESP32_UART_INT_RAW] |= ESP32_UART_INT_RXFIFO_FULL;
    } else {
        s->reg[ESP32_UART_INT_RAW] &= ~ESP32_UART_INT_RXFIFO_FULL;
    }

    if (!esp32_serial_can_receive(s)) {
        s->reg[ESP32_UART_INT_RAW] |= ESP32_UART_INT_RXFIFO_OVF;
    } else {
        s->reg[ESP32_UART_INT_RAW] &= ~ESP32_UART_INT_RXFIFO_OVF;
    }
    esp32_serial_irq_update(s);
}

static uint64_t esp32_serial_read(void *opaque, hwaddr addr,
                                    unsigned size)
{
    Esp32SerialState *s = opaque;

    DEBUG_LOG("%s: +0x%02x: \n", __func__, (uint32_t)addr);

    //if ((addr & 3) || size != 4) {
    //    return 0;
    //}

    switch (addr / 4) {
    case ESP32_UART_STATUS:
        // return 0;
        // TODO!! TODO!! Make read interrupt work for UARRTS!
        return esp32_serial_rx_fifo_size(s);

    case ESP32_UART_FIFO:
        //fprintf(stderr,"siz %u\n",esp32_serial_rx_fifo_size(s));
        if (esp32_serial_rx_fifo_size(s)) {
            uint8_t r = s->rx[s->rx_first++];
            //fprintf(stderr,"%c\n",(char)r);

            s->rx_first &= ESP32_UART_FIFO_MASK;
            esp32_serial_rx_irq_update(s);
            return r;
        } else {
            return 0;
        }
    case ESP32_UART_INT_RAW:
    case ESP32_UART_INT_ST:
    case ESP32_UART_INT_ENA:
    case ESP32_UART_INT_CLR:
    case ESP32_UART_CLKDIV:
    case ESP32_UART_AUTOBAUD:
    case ESP32_UART_CONF0:
    case ESP32_UART_CONF1:
    case ESP32_UART_LOWPULSE:
    case ESP32_UART_HIGHPULSE:
    case ESP32_UART_RXD_CNT:
    case ESP32_UART_DATE:
    case ESP32_UART_ID:
        return s->reg[addr / 4];

    default:
        printf("%s: unexpected read @0x%"HWADDR_PRIx"\n", __func__, addr);
        //qemu_log("%s: unexpected read @0x%"HWADDR_PRIx"\n", __func__, addr);
        break;
    }
    return 0;
}

static void esp32_serial_receive(void *opaque, const uint8_t *buf, int size)
{
    Esp32SerialState *s = opaque;
    unsigned i;
    fprintf(stderr,"UART socket received %s,%d first%d last%d\n",buf,size,s->rx_first,s->rx_last);

    for (i = 0; i < size && esp32_serial_can_receive(s); ++i) {
        s->rx[s->rx_last++] = buf[i];
        s->rx_last &= ESP32_UART_FIFO_MASK;
    }

    esp32_serial_rx_irq_update(s);
}

static void esp32_serial_ro(Esp32SerialState *s, hwaddr addr,
                              uint64_t val, unsigned size)
{
}

static void esp32_serial_tx(Esp32SerialState *s, hwaddr addr,
                              uint64_t val, unsigned size)
{
    DEBUG_LOG("FIFO: %c \n",(char)val);

    if (ESP32_UART_GET(s, CONF0, LOOPBACK)) {
        if (esp32_serial_can_receive(s)) {
            uint8_t buf[] = { (uint8_t)val };
            fprintf(stderr,"loopback %c",(char)val);
            esp32_serial_receive(s, buf, 1);
        }

    } else if (true /*s->chr*/) {
        uint8_t buf[1] = { val };
        qemu_chr_fe_write(&s->chr, buf, 1);

        s->reg[ESP32_UART_INT_RAW] |= ESP32_UART_INT_TXFIFO_EMPTY;
        esp32_serial_irq_update(s);
    }
}

static void esp32_serial_int_ena(Esp32SerialState *s, hwaddr addr,
                                   uint64_t val, unsigned size)
{
    s->reg[ESP32_UART_INT_ENA] = val & 0x1ff;
    esp32_serial_irq_update(s);
}

static void esp32_serial_int_clr(Esp32SerialState *s, hwaddr addr,
                                   uint64_t val, unsigned size)
{
    s->reg[ESP32_UART_INT_ST] &= ~val & 0x1ff;
    esp32_serial_irq_update(s);
}

static void esp32_serial_set_conf0(Esp32SerialState *s, hwaddr addr,
                                     uint64_t val, unsigned size)
{
    s->reg[ESP32_UART_CONF0] = val & 0xffffff;
    if (ESP32_UART_GET(s, CONF0, RXFIFO_RST)) {
        s->rx_first = s->rx_last = 0;
        esp32_serial_rx_irq_update(s);
    }
}

static void esp32_serial_write(void *opaque, hwaddr addr,
                                 uint64_t val, unsigned size)
{
    Esp32SerialState *s = opaque;

    DEBUG_LOG("%s: +0x%02x:  \n", __func__, (uint32_t)addr);

    if (addr==0) {
     fprintf(stderr,"%c",(char)val);
        //fprintf(stderr,"%c",(char)val);
        if (s->gdb_serial_connected) {
            pthread_mutex_lock(&mutex);
            int pos=s->gdb_serial_buff_tx%MAX_GDB_BUFF;
            s->gdb_serial_data[pos]=(char)val;
            //fprintf(stderr,"[%c,%c,%d]",(char)val,s->gdb_serial_data[pos],s->gdb_serial_buff_tx);
            s->gdb_serial_buff_tx++;
            pthread_mutex_unlock(&mutex);
        }
    }



    static void (* const handler[])(Esp32SerialState *s, hwaddr addr,
                                    uint64_t val, unsigned size) = {
        [ESP32_UART_FIFO] = esp32_serial_tx,
        [ESP32_UART_INT_RAW] = esp32_serial_ro,
        [ESP32_UART_INT_ST] = esp32_serial_ro,
        [ESP32_UART_INT_ENA] = esp32_serial_int_ena,
        [ESP32_UART_INT_CLR] = esp32_serial_int_clr,
        [ESP32_UART_STATUS] = esp32_serial_ro,
        [ESP32_UART_CONF0] = esp32_serial_set_conf0,
        [ESP32_UART_LOWPULSE] = esp32_serial_ro,
        [ESP32_UART_HIGHPULSE] = esp32_serial_ro,
        [ESP32_UART_RXD_CNT] = esp32_serial_ro,
    };

    if ((addr & 3) || size != 4 || addr / 4 >= ESP32_UART_MAX) {
        return;
    }

    if (addr / 4 < ARRAY_SIZE(handler) && handler[addr / 4]) {
        handler[addr / 4](s, addr, val, size);
    } else {
        s->reg[addr / 4] = val;
    }
}

static void esp32_serial_reset(void *opaque)
{
    Esp32SerialState *s = opaque;

    memset(s->reg, 0, sizeof(s->reg));

    s->reg[ESP32_UART_CLKDIV] = 0x2b6;
    s->reg[ESP32_UART_AUTOBAUD] = 0x1000;
    s->reg[ESP32_UART_CONF0] = 0x1c;
    s->reg[ESP32_UART_CONF1] = 0x6060;
    s->reg[ESP32_UART_LOWPULSE] = 0xfffff;
    s->reg[ESP32_UART_HIGHPULSE] = 0xfffff;
    s->reg[ESP32_UART_DATE] = 0x62000;
    s->reg[ESP32_UART_ID] = 0x500;

    esp32_serial_irq_update(s);
}

static const MemoryRegionOps esp32_serial_ops = {
    .read = esp32_serial_read,
    .write = esp32_serial_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};


static void esp32_serial_event(void *opaque, int event)
{
    Esp32SerialState *s = (Esp32SerialState *)opaque;

    if (event == CHR_EVENT_BREAK) {
    }
}


Esp32SerialState *gdb_serial[4]={0};

Esp32SerialState *esp32_serial_init(int uart_num,MemoryRegion *address_space,
                                               hwaddr base, const char *name,
                                               qemu_irq irq,
                                               Chardev *chr_dev);

Esp32SerialState *esp32_serial_init(int uart_num,MemoryRegion *address_space,
                                               hwaddr base, const char *name,
                                               qemu_irq irq,
                                               Chardev *chr_dev)
{
    Esp32SerialState *s = g_malloc(sizeof(Esp32SerialState));


    s->chr.chr=chr_dev;

    s->irq = irq;
    s->uart_num=uart_num;


    //qemu_chr_add_handlers(s->chr, esp32_serial_can_receive,
    //                      esp32_serial_receive, NULL, s);
    memory_region_init_io(&s->iomem, NULL, &esp32_serial_ops, s,
                          name, 0x100);
    memory_region_add_subregion(address_space, base, &s->iomem);
    qemu_register_reset(esp32_serial_reset, s);
    s->rx_last=s->rx_first=0;
    s->guard=0xbeef;


    qemu_chr_fe_set_handlers(&s->chr, esp32_serial_can_receive,
                             esp32_serial_receive, esp32_serial_event,
                             s, NULL, true);

    if (uart_num==0) {
        silly_serial=&s->chr;
    }



    return s;
}


//////// SPI //////////

/* SPI */


enum {
ESP32_SPI_FLASH_CMD,     // 00
ESP32_SPI_FLASH_ADDR,    // 04
ESP32_SPI_FLASH_CTRL,    // 08
ESP32_SPI_FLASH_CTRL1,   // 0C
ESP32_SPI_FLASH_STATUS,  // 10
ESP32_SPI_FLASH_CTRL2,   // 14
ESP32_SPI_FLASH_CLOCK,   // 18
ESP32_SPI_FLASH_USER,    // 1c
ESP32_SPI_FLASH_USER1,   // 20 
ESP32_SPI_FLASH_USER2,   
ESP32_MOSI_DLEN,         
ESP32_MISO_DLEN,
ESP32_SLV_WR_STATUS,     // 30
ESP32_SPI_FLASH_PIN,
ESP32_SPI_FLASH_SLAVE,   
ESP32_SPI_FLASH_SLAVE1,
ESP32_SPI_FLASH_SLAVE2,  // 40
ESP32_SPI_FLASH_SLAVE3,
ESP32_SLV_WRBUF_DLEN,   
ESP32_SLV_RDBUF_DLEN,   
ESP32_CACHE_FCTRL,      //   50
ESP32_CACHE_SCTRL,
ESP32_SRAM_CMD,         
ESP32_SRAM_DRD_CMD,
sram_dwr_cmd,           // 60
slv_rd_bit,
reserved_68,           
reserved_6c,
reserved_70,           // 70
reserved_74,
reserved_78,             
reserved_7c,
//uint32_t data_buf[16],                                  /*data buffer*/
data_w0,               //  80
data_w1,           
data_w2,           
data_w3,           
data_w4,               //  90
data_w5,
data_w6,            
data_w7,          
data_w8,               //  a0
data_w9,
data_w10,
data_w11,          
data_w12,              // b0
data_w13,
data_w14,
data_w15,
tx_crc,               // c0                  /*For SPI1  the value of crc32 for 256 bits data.*/
reserved_c4,
reserved_c8,
reserved_cc,
reserved_d0,          // d0
reserved_d4,
reserved_d8,
reserved_dc,
reserved_e0,          // e0
reserved_e4,
reserved_e8,
reserved_ec,
SPI_EXT0_REG,         // f0
SPI_EXT1_REG,         // f4
SPI_EXT2_REG,         // f8
SPI_EXT3_REG,       
SPI_DMA_CONF,         // 100
SPI_DMA_OUT_LINK,
SPI_DMA_IN_LINK,
SPI_DMA_STATUS,     
SPI_DMA_INT_ENA,      // 110
SPI_DMA_INT_RAW,
SPI_DMA_INT_ST,
SPI_DMA_INT_CLR,      // 11c
SPI_SUC_EOF_DES_ADDR, // 120
SPI_INLINK_DSCR,      // 12c
SPI_INLINK_DSCR_BF1,  // 130
SPI_OUT_EOF_BFR_DES_ADDR, 
SPI_OUT_EOF_DES_ADDR,
SPI_OUTLINK_DSCR,
SPI_OUTLINK_DSCR_BF0,  // 140
SPI_OUTLINK_DSCR_BF1,
SPI_DMA_RSTATUS,
SPI_DMA_TSTATUS_REG,   // 14c
SPI_DATE_REG=0x3fc/4,  // 3fc
R_MAX = 0x100
};


#define ESP32_SPI_FLASH_BITS(reg, field, shift, len) \
    DEFINE_BITS(ESP32_SPI_FLASH, reg, field, shift, len)

#define ESP32_SPI_GET_VAL(v, _reg, _field) \
    extract32(v, \
              ESP32_SPI_FLASH_##_reg##_##_field##_SHIFT, \
              ESP32_SPI_FLASH_##_reg##_##_field##_LEN)

#define ESP32_SPI_GET(s, _reg, _field) \
    extract32(s->reg[ESP32_SPI_FLASH_##_reg], \
              ESP32_SPI_FLASH_##_reg##_##_field##_SHIFT, \
              ESP32_SPI_FLASH_##_reg##_##_field##_LEN)

#define ESP32_MAX_FLASH_SZ (1 << 24)


enum {
    ESP32_SPI_FLASH_BITS(CMD, USR, 18, 1),
    ESP32_SPI_FLASH_BITS(CMD, WRDI, 29, 1),
    ESP32_SPI_FLASH_BITS(CMD, WREN, 30, 1),
    ESP32_SPI_FLASH_BITS(CMD, READ, 31, 1),
};

enum {
    ESP32_SPI_FLASH_BITS(ADDR, OFFSET, 0, 24),
    ESP32_SPI_FLASH_BITS(ADDR, LENGTH, 24, 8),
};

enum {
    ESP32_SPI_FLASH_BITS(CTRL, ENABLE_AHB, 17, 1),
};

enum {
    ESP32_SPI_FLASH_BITS(STATUS, BUSY, 0, 1),
    ESP32_SPI_FLASH_BITS(STATUS, WRENABLE, 1, 1),
};

enum {
    ESP32_SPI_FLASH_BITS(CLOCK, CLKCNT_L, 0, 6),
    ESP32_SPI_FLASH_BITS(CLOCK, CLKCNT_H, 6, 6),
    ESP32_SPI_FLASH_BITS(CLOCK, CLKCNT_N, 12, 6),
    ESP32_SPI_FLASH_BITS(CLOCK, CLK_DIV_PRE, 18, 13),
    ESP32_SPI_FLASH_BITS(CLOCK, CLK_EQU_SYSCLK, 31, 1),
};

enum {
    ESP32_SPI_FLASH_BITS(USER, FLASH_MODE, 2, 1),
    ESP32_SPI_FLASH_BITS(USER, COMMAND, 30, 1),
    ESP32_SPI_FLASH_BITS(USER, ADDR, 31, 1),
};

enum {
    ESP32_SPI_FLASH_BITS(USER2, COMMAND_VALUE, 0, 16),
    ESP32_SPI_FLASH_BITS(USER2, COMMAND_BITLEN, 28, 4),
};

enum {
    ESP32_SPI_FLASH_BITS(ADDR, ADDR_VALUE, 0, 24),
    ESP32_SPI_FLASH_BITS(ADDR, ADDR_RESERVED, 24,8),
};


typedef struct Esp32SpiState {
    int spiNum;
    MemoryRegion iomem;
    //MemoryRegion cache;
    qemu_irq irq;
    void *flash_image;
    int length;

    uint32_t reg[R_MAX];
} Esp32SpiState;

static uint64_t esp32_spi_read(void *opaque, hwaddr addr, unsigned size)
{
    Esp32SpiState *s = opaque;

    DEBUG_LOG("%d %s: +0x%02x: ",s->spiNum, __func__, (uint32_t)addr);
    if (addr / 4 >= R_MAX || addr % 4 || size != 4) {
        DEBUG_LOG("SPI ADRESS ERROR 0\n");
        return 0;
    }
    DEBUG_LOG("0x%08x\n", s->reg[addr / 4]);


    if (s->spiNum==2) {
        if (addr==0x38) {
            // SPI_TRANS_DONE
            s->reg[addr / 4]=0xff;
        }
    }
    return s->reg[addr / 4];
}

static void esp32_spi_cmd(Esp32SpiState *s, hwaddr addr,
                            uint64_t val, unsigned size)
{
    //DEBUG_LOG("esp32_spi_cmd %08x\n",val);
    //s->reg[addr / 4] = val;

    if (s->spiNum!=0) {
        return;
    }

    if (val & 0x1000000) {
            DEBUG_LOG("esp32_spi_cmd_erase??? %08x\n",val);
            unsigned int write_addr=ESP32_SPI_GET(s, ADDR, OFFSET);

    // Set all in sector to 0xff
        memset(s->flash_image + write_addr,
            0xff,  
            0x1000);  // (ESP32_SPI_GET(s, ADDR, LENGTH) + 3) & 0x3c 

    }

    if (val & ESP32_SPI_FLASH_CMD_READ) {
        if (ESP32_SPI_GET(s, USER, FLASH_MODE)) {
            DEBUG_LOG("%s: READ FLASH 0x%02x@0x%08x\n",
                      __func__,
                      ESP32_SPI_GET(s, ADDR, LENGTH),
                      ESP32_SPI_GET(s, ADDR, OFFSET));
        } else {
            DEBUG_LOG("%s: READ ?????\n", __func__);
        }
    }
    if (val & ESP32_SPI_FLASH_CMD_WRDI) {
        DEBUG_LOG("status wrdi\n");
        s->reg[ESP32_SPI_FLASH_STATUS] &= ~ESP32_SPI_FLASH_STATUS_WRENABLE;
    }
    if (val & ESP32_SPI_FLASH_CMD_WREN) {
        DEBUG_LOG("status wren\n");
        unsigned int write_addr=ESP32_SPI_GET(s, ADDR, OFFSET);
        // Not sure this is a good idea??
        // Where is length field?
        DEBUG_LOG("len %d\n",s->length);

        memcpy(s->flash_image + write_addr,
            &s->reg[data_w0],  // ESP32_SPI_GET(s, ADDR, OFFSET)
            4*8);  // (ESP32_SPI_GET(s, ADDR, LENGTH) + 3) & 0x3c 

        s->reg[ESP32_SPI_FLASH_STATUS] |= ESP32_SPI_FLASH_STATUS_WRENABLE;
    }
    if (val & ESP32_SPI_FLASH_CMD_USR) {
        DEBUG_LOG("%s: TX %04x[%d bits]\n",
                  __func__,
                  ESP32_SPI_GET(s, USER2, COMMAND_VALUE),
                  ESP32_SPI_GET(s, USER2, COMMAND_BITLEN));

       int numBits=ESP32_SPI_GET(s, USER2, COMMAND_BITLEN);
       int command=ESP32_SPI_GET(s, USER2, COMMAND_VALUE) & (( 1 << numBits) -1);
       DEBUG_LOG("command %04x\n",command);
       if (command==0x35) {
           DEBUG_LOG("CMD 0x35 (RDSR2) read status register\n");
       }
       if (command==0x05) {
           DEBUG_LOG("CMD 0x05 (RDSR).\n");
       }       
       if (command==0x03) {
           DEBUG_LOG("SPI_READ 0x03. %08X\n",ESP32_SPI_GET(s, ADDR, OFFSET));
           // TODO, ignore bit 0-7 !!!
           unsigned int silly=ESP32_SPI_GET(s, ADDR, OFFSET) >> 8;
           DEBUG_LOG("Silly %08X\n",silly);

/*
            unsigned int *data1=(unsigned int *)s->flash_image +silly;
            int q=0;
            for(q=0;q<16;q++) 
            {
                printf( "%08X", *data1);
                data1++;
                if (q%8==7) {
                    printf("\n");
                }
            }
*/


            memcpy(&s->reg[data_w0],
                   s->flash_image + silly,  // ESP32_SPI_GET(s, ADDR, OFFSET)
                   4*16);  // (ESP32_SPI_GET(s, ADDR, LENGTH) + 3) & 0x3c 
/*
                    unsigned int *data=(unsigned int *)&s->reg[data_w0];
                    int j=0;
                    for(j=0;j<16;j++) 
                    {
                        fprintf(stderr, "%08X", *data);
                        data++;
                        if (j%8==7) {
                            fprintf(stderr,"\n");
                        }
                    }
*/


       }
       /*
       if (command==0x02) {
           DEBUG_LOG("SPI_WRITE 0x02. %08X\n",ESP32_SPI_GET(s, ADDR, OFFSET));
           // TODO, ignore bit 0-7 !!!
           unsigned int silly=ESP32_SPI_GET(s, ADDR, OFFSET) >> 8;
           DEBUG_LOG("Silly %08X\n",silly);

            memcpy(s->flash_image + silly,
                &s->reg[data_w0],  // ESP32_SPI_GET(s, ADDR, OFFSET)
                4*16);  // (ESP32_SPI_GET(s, ADDR, LENGTH) + 3) & 0x3c 

       }
       */

    }
}

static void esp32_spi_write_address(Esp32SpiState *s, hwaddr addr,
                                   uint64_t val, unsigned size)
{
        s->reg[ESP32_SPI_FLASH_ADDR] = val;
        //DEBUG_LOG("equal? %04X , %04X" , SPI_EXT2_REG*4,0xf8);

        DEBUG_LOG("Address %s: TX %08x[%d reserved]\n",
                  __func__,
                  ESP32_SPI_GET(s, ADDR, ADDR_VALUE),
                  ESP32_SPI_GET(s, ADDR, ADDR_RESERVED));


}


static void esp32_spi_write_ctrl(Esp32SpiState *s, hwaddr addr,
                                   uint64_t val, unsigned size)
{
    s->reg[ESP32_SPI_FLASH_CTRL] = val;
    DEBUG_LOG("esp32_spi_write_ctrl,ENABLE_AHB?\n");
    //memory_region_set_enabled(&s->cache,
    //                          val & ESP32_SPI_FLASH_CTRL_ENABLE_AHB);
}

static void esp32_spi_status(Esp32SpiState *s, hwaddr addr,
                               uint64_t val, unsigned size)
{
}

static void esp32_spi_clock(Esp32SpiState *s, hwaddr addr,
                              uint64_t val, unsigned size)
{
    DEBUG_LOG("%s: L: %d, H: %d, N: %d, PRE: %d, SYSCLK: %d\n",
              __func__,
              ESP32_SPI_GET_VAL(val, CLOCK, CLKCNT_L),
              ESP32_SPI_GET_VAL(val, CLOCK, CLKCNT_H),
              ESP32_SPI_GET_VAL(val, CLOCK, CLKCNT_N),
              ESP32_SPI_GET_VAL(val, CLOCK, CLK_DIV_PRE),
              ESP32_SPI_GET_VAL(val, CLOCK, CLK_EQU_SYSCLK));
    s->reg[ESP32_SPI_FLASH_CLOCK] = val;
}

static void esp32_spi_write(void *opaque, hwaddr addr, uint64_t val,
                              unsigned size)
{
    Esp32SpiState *s = opaque;
    static void (* const handler[])(Esp32SpiState *s, hwaddr addr,
                                    uint64_t val, unsigned size) = {
        [ESP32_SPI_FLASH_CMD] = esp32_spi_cmd,
        [ESP32_SPI_FLASH_ADDR] =esp32_spi_write_address,
        [ESP32_SPI_FLASH_CTRL] = esp32_spi_write_ctrl,
        [ESP32_SPI_FLASH_STATUS] = esp32_spi_status,
        [ESP32_SPI_FLASH_CLOCK] = esp32_spi_clock,
    };

    if (addr>=0x80 && addr <= 0x9c) {
       unsigned int write_addr=ESP32_SPI_GET(s, ADDR, OFFSET);

#if 0
       int offset=(addr-0x80);
       unsigned int *data_ptr=(unsigned int *)((unsigned int)s->flash_image + (unsigned int )write_addr/4 + (unsigned int )offset/4);
       int to_switch=offset%4;
       if (data_ptr>=s->flash_image && data_ptr<=(s->flash_image+4000000)) {
           unsigned char  original[4];
           original[0] = *data_ptr >> 24 & 0xff;
           original[1] = *data_ptr >> 16 & 0xff;
           original[2] = *data_ptr >> 8 & 0xff;
           original[3] = *data_ptr >> 0 & 0xff;
           original[to_switch]= val;
           *data_ptr = (original[0] << 24  | original[1] << 16  |  original[2] << 8 |  original[3] << 0 );  
       }
#endif    
       int length=1+(addr-0x80)/4;

       if (addr==0x9c)
       {
         s->reg[addr / 4] = val;
         //unsigned int *set=(uint32_t)s->flash_image + (uint32_t)silly +(uint32_t)(addr-0x80);
         //*set=val;
         // Memory mapped file
         if (s->spiNum!=0) {
             return;
         }


         memcpy(s->flash_image + write_addr,
            &s->reg[data_w0],  // ESP32_SPI_GET(s, ADDR, OFFSET)
            4*8);  // (ESP32_SPI_GET(s, ADDR, LENGTH) + 3) & 0x3c 

       }

       
       //DEBUG_LOG("SPI data 0x%08x\n",val);     
    }


    DEBUG_LOG("%d %s: +0x%02x = 0x%08x\n",s->spiNum,
            __func__, (uint32_t)addr, (uint32_t)val);
    if (addr / 4 >= R_MAX || addr % 4 || size != 4) {
        return;
    }
    if (addr / 4 < ARRAY_SIZE(handler) && handler[addr / 4]) {
        handler[addr / 4](s, addr, val, size);
    } else {
         DEBUG_LOG("written\n");
        s->reg[addr / 4] = val;
    }
}

static void esp32_spi_reset(void *opaque)
{
    Esp32SpiState *s = opaque;

    memset(s->reg, 0, sizeof(s->reg));
    //memory_region_set_enabled(&s->cache, false);

    //esp32_spi_irq_update(s);
}

static const MemoryRegionOps esp32_spi_ops = {
    .read = esp32_spi_read,
    .write = esp32_spi_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static Esp32SpiState *esp32_spi_init(int spinum,MemoryRegion *address_space,
                                         hwaddr base, const char *name,
                                         MemoryRegion *cache_space,
                                         hwaddr cache_base,
                                         const char *cache_name,
                                         qemu_irq irq, void **flash_image)
{
    Esp32SpiState *s = g_malloc(sizeof(Esp32SpiState));
    s->spiNum=spinum;

    s->irq = irq;
    memory_region_init_io(&s->iomem, NULL, &esp32_spi_ops, s,
                          name, 0x100);

    s->flash_image=get_flashMemory();
    *flash_image=s->flash_image;
    //int *test=(int *)s->flash_image;
    //*test=0xdead;

    //memory_region_init_rom_device(&s->cache, NULL, NULL, s,
    //                              cache_name, ESP32_MAX_FLASH_SZ,
    //                              NULL);
    //s->flash_image = memory_region_get_ram_ptr(&s->cache);
    //if (flash_image) {
    //    *flash_image = s->flash_image;
    //}
    memory_region_add_subregion(address_space, base, &s->iomem);
    //memory_region_add_subregion(cache_space, cache_base, &s->cache);
    //memory_region_set_enabled(&s->cache, false);
    qemu_register_reset(esp32_spi_reset, s);
    return s;
}

/*
spi = esp32_spi_init(system_io, 0x0000100, "esp32.spi0",
                    system_memory, 0x3ff4300, "esp32.flash",
                    xtensa_get_extint(env, 6), &flash_image);

*/


/*
 * This will handle connection for each client
 * */
void *connection_handler(void *connect)
{
    //Get the socket descriptor
    connect_data *start_data=(connect_data *)connect;

    int sock = start_data->socket;
    int uart_num=start_data->uart_num;
    int read_size;
    char *client_message;

    client_message=(char *) malloc(4096);

    gdb_serial[uart_num]->gdb_serial_connected=true;
     
    fprintf(stderr,"Started uart socket\n");

    //struct timeval tv;
    //tv.tv_sec = 0;  /* 30 Secs Timeout */
    //tv.tv_usec = 100000;  
    //setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));

    //message = "$T08\n";
    //write(sock , message , strlen(message));


    //Receive a message from client
    bool socket_ok=true;
    do 
    {
        struct pollfd fd;
        int ret;
        read_size=0;

        fd.fd = sock; // your socket handler 
        fd.events = POLLIN;
        ret = poll(&fd, 1, 20); // 1 second for timeout
        switch (ret) {
            case -1:
                // Error
                break;
            case 0:
                // Timeout 
                break;
            default:
                read_size = recv(sock , client_message , 512 , 0);
                break;
        }

        if (gdb_serial[uart_num]) {
            if (read_size>0)  {
                esp32_serial_receive(gdb_serial[0],(unsigned char *)client_message,read_size);
                printf("%s",client_message);
            }
        }


        //if (gdb_serial[uart_num]->guard!=0xbeef) {
        //  fprintf(stderr,"GUARD OVERWRITTEN!!!!!");            
        //}


        pthread_mutex_lock(&mutex);
        int to_send=gdb_serial[uart_num]->gdb_serial_buff_tx-gdb_serial[uart_num]->gdb_serial_buff_rd;
        //if (gdb_serial[uart_num]->gdb_serial_buff_tx!=gdb_serial[uart_num]->gdb_serial_buff_rd) fprintf(stderr,"%d %d %d\n",gdb_serial[uart_num]->gdb_serial_buff_tx,gdb_serial[uart_num]->gdb_serial_buff_rd,to_send);

        //char *testme="TEST ME\n";
        //int test=write(sock ,testme , 8);

        if (gdb_serial[uart_num]->gdb_serial_buff_rd<gdb_serial[uart_num]->gdb_serial_buff_tx) {
                //gdb_serial[uart_num]->gdb_serial_data[gdb_serial[uart_num]->gdb_serial_buff_tx]=0;
                //char *data=(char *) gdb_serial[uart_num]->gdb_serial_data;
                //fprintf(stderr,"->%s<-",data); 
                /*
                int pos=gdb_serial[uart_num]->gdb_serial_buff_rd;
                while(pos<gdb_serial[uart_num]->gdb_serial_buff_tx) {
                    fprintf(stderr,"*%d",gdb_serial[uart_num]->gdb_serial_data[pos]);
                    if (gdb_serial[uart_num]->gdb_serial_data[pos%MAX_GDB_BUFF]=='+')
                    {
                        fprintf(stderr,"\n");
                    }
                    pos++;
                }
                */
                if (gdb_serial[uart_num]->gdb_serial_buff_rd + to_send<MAX_GDB_BUFF)
                {
                       char *data=&gdb_serial[uart_num]->gdb_serial_data[gdb_serial[uart_num]->gdb_serial_buff_rd];
                       int test=write(sock , data , to_send);
                       if (test!=to_send) { fprintf(stderr,"send failed 1\n"); socket_ok=false;};
                       gdb_serial[uart_num]->gdb_serial_buff_rd+=to_send;
                }
                else 
                {
                    while(gdb_serial[uart_num]->gdb_serial_buff_rd<gdb_serial[uart_num]->gdb_serial_buff_tx) {
                       int test=write(sock , &(gdb_serial[uart_num]->gdb_serial_data[(gdb_serial[uart_num]->gdb_serial_buff_rd++)%MAX_GDB_BUFF]) , 1);
                       if (test!=1) { fprintf(stderr,"send failed 2\n"); socket_ok=false;};
                       gdb_serial[uart_num]->gdb_serial_buff_rd+=1;
                    }
                }
        }
        pthread_mutex_unlock(&mutex);


    } while (socket_ok);
     
    if(read_size == 0)
    {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("recv failed");
    }
         
    gdb_serial[uart_num]->gdb_serial_connected=false;


    //Free data pointer
    free(start_data);
    free(client_message);
     
    return 0 ;
}

void *gdb_socket_thread(void *uart_num)
{
    int socket_desc , client_sock , c ; //, *new_sock;
    struct sockaddr_in server , client;
     
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    } else {
        int enable = 1;
        if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
            printf("setsockopt(SO_REUSEADDR) failed");
    }
    puts("Socket created");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    if (uart_num==0) {
       server.sin_port = htons( 8880 );
    }
    if (uart_num==1) {
       server.sin_port = htons( 8881 );
    }
    if (uart_num==2) {
       server.sin_port = htons( 8882 );
    }


    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return((void *)0);
    }
    puts("bind done");
     
    //Listen
    listen(socket_desc , 3);
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
     
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        puts("Connection accepted");
         
        pthread_t sniffer_thread;
        connect_data* data=(connect_data*)malloc(sizeof(connect_data));

        data->socket=client_sock;
        data->uart_num=uart_num;
        //new_sock = malloc(1);
        //*new_sock = client_sock;
         
        if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) data) < 0)
        {
            perror("could not create thread");
            return 0;
        }
         
        //Now join the thread , so that we dont terminate before the thread
        //pthread_join( sniffer_thread , NULL);
        puts("Handler assigned");
    }
     
    if (client_sock < 0)
    {
        perror("accept failed");
        return((void *) 1);
    }
     
    return((void *) 0);
}


typedef struct ESP32BoardDesc {
    hwaddr flash_base;
    size_t flash_size;
    size_t flash_boot_base;
    size_t flash_sector_size;
    size_t sram_size;
} ESP32BoardDesc;

typedef struct ULP_State {
    MemoryRegion iomem;
} ULP_State;

static void ulp_reset(void *opaque)
{
    //ULP_State *s = opaque;
    //s->leds = 0;
    //s->switches = 0;
}

static uint64_t ulp_read(void *opaque, hwaddr addr,
        unsigned size)
{
    printf("ulp read %" PRIx64 " \n",addr);

    //ULP_State *s = opaque;
    switch (addr) {
    case 0x0: /*boot start time*/
        return 0x0;
    }
    return 0;
}

static void ulp_write(void *opaque, hwaddr addr,
        uint64_t val, unsigned size)
{
    //ULP_State *s = opaque;
    printf("ulp write %" PRIx64 " \n",addr);

    switch (addr) {
    case 0x0: 
        //s->leds = val;
        break;
    }
}

static const MemoryRegionOps ulp_ops = {
    .read = ulp_read,
    .write = ulp_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static ULP_State *esp32_fpga_init(MemoryRegion *address_space,
        hwaddr base)
{
    ULP_State *s = g_malloc(sizeof(ULP_State));

    //memory_region_init_io(&s->iomem, NULL, &lx60_fpga_ops, s,
    //        "lx60.fpga", 0x10000);
    //memory_region_add_subregion(address_space, base, &s->iomem);
    ulp_reset(s);
    qemu_register_reset(ulp_reset, s);
    return s;
}

static void open_net_init(MemoryRegion *address_space,
        hwaddr base,
        hwaddr descriptors,
        hwaddr buffers,
        qemu_irq irq, NICInfo *nd)
{
    DeviceState *dev;
    SysBusDevice *s;
    MemoryRegion *ram;

    dev = qdev_create(NULL, "open_eth");
    qdev_set_nic_properties(dev, nd);
    qdev_init_nofail(dev);

    s = SYS_BUS_DEVICE(dev);
    sysbus_connect_irq(s, 0, irq);
    memory_region_add_subregion(address_space, base,
            sysbus_mmio_get_region(s, 0));
    memory_region_add_subregion(address_space, descriptors,
            sysbus_mmio_get_region(s, 1));

    ram = g_malloc(sizeof(*ram));
    memory_region_init_ram(ram, OBJECT(s), "open_eth.ram", 16384,
                           &error_fatal);
    vmstate_register_ram_global(ram);
    memory_region_add_subregion(address_space, buffers, ram);
}

#if 0
static uint64_t translate_phys_addr(void *opaque, uint64_t addr)
{
    XtensaCPU *cpu = opaque;

    return cpu_get_phys_page_debug(CPU(cpu), addr);
}
#endif


static void esp32_reset(void *opaque)
{
    Esp32 *esp32 = opaque;
    int i;

    for (i = 0; i < 2; ++i) {

        cpu_reset(CPU(esp32->cpu[i]));
        xtensa_runstall(&esp32->cpu[i]->env, i > 0);

       CPUXtensaState *env = &esp32->cpu[i]->env;

      //env->ccompare_timer =
      // No need for this in latest version
      //timer_new_ns(QEMU_CLOCK_VIRTUAL, &esp_xtensa_ccompare_cb, esp32->cpu[i]);
    }
}


static unsigned int sim_RTC_CNTL_DIG_ISO_REG;

static unsigned int sim_RTC_CNTL_STORE5_REG=0;

static unsigned int sim_DPORT_PRO_CACHE_CTRL_REG=0x28;

static unsigned int sim_DPORT_PRO_CACHE_CTRL1_REG=0x8E6;

static unsigned int sim_DPORT_APP_CACHE_CTRL1_REG=0x8E6;

static unsigned int sim_DPORT_PRO_CACHE_LOCK_0_ADDR_REG=0;


//  0x3ff5f06c 
// TIMG_RTCCALICFG1_REG 3ff5f06c=25

static unsigned int sim_DPORT_APP_CACHE_CTRL_REG=0x28;

static unsigned int sim_DPORT_APPCPU_CTRL_D_REG=0;


static unsigned int pro_MMU_REG[0x2000];

static unsigned int app_MMU_REG[0x2000];

bool nv_init_called=false;

void memdump(uint32_t mem,uint32_t len)
{
    unsigned int *rom_data = (unsigned int *)malloc(64*0x400 * sizeof(unsigned int));
    cpu_physical_memory_read(mem, rom_data, 60*0x400 );
    int q=0;
    for(q=0;q<(64/4)*0x400;q++) 
    {
        printf( "%08X", rom_data[q]);
    }
    free(rom_data);
}

void mapFlashToMem(uint32_t flash_start,uint32_t mem_addr,uint32_t len)
{
        if (flash_start==0x01000000) {
            // Special value 0x100 to mark begining. Should not be mapped
            return;
        }
        fprintf(stderr,"(qemu)  %08X to memory, %08X\n",flash_start,mem_addr);
        printf("Flash map data to  %08X to memory, %08X\n",flash_start,mem_addr);
        // I dont know how this works. Guessing
        
        FILE *f_flash = fopen("esp32flash.bin", "r");

        if (f_flash == NULL)
        {
            fprintf(stderr, "   Can't open 'esp32flash.bin' for reading.\n");
        }
        else
        {
            unsigned int *rom_data = (unsigned int *)malloc(len);
            fseek(f_flash,flash_start,SEEK_SET);
            if (fread(rom_data, len, 1, f_flash) < 1)
            {
                fprintf(stderr, " File 'esp32flash.bin' is truncated or corrupt.\n");
            }
            //fprintf(stderr,"-%8X\n",mem_addr);
            //fprintf(stderr,"->%8X\n",mem_addr+0x10000);

            //memdump(mem_addr,0x4000);
            cpu_physical_memory_write(mem_addr, rom_data, len );

            //fprintf(stderr, "(qemu) Flash partition data is loaded.\n");
            free(rom_data);
        }        
}


static uint64_t esp_io_read(void *opaque, hwaddr addr,
        unsigned size)
{


    if ((addr!=0x04001c) && (addr!=0x38)) printf("io read %" PRIx64 " ",addr);

    if (addr>=0x10000 && addr<0x11ffc) {

        return(pro_MMU_REG[addr/4-0x10000/4]);
    }

    if (addr>=0x12000 && addr<0x13ffc) {

        if (addr==0x123fc)
        {
            // Bootlader already did this, it should be safe to map this, app expects it
            mapFlashToMem(0x8000, 0x3f408000,0x10000-0x8000);
            // TODO!! Only works for factory app
            //mapFlashToMem(2*0x10000, 0x3f400000,0x10000);  
            nv_init_called=true;
        }

        return(app_MMU_REG[addr/4-0x12000/4]);
    }




    switch (addr) {

       case 0x38:
           // PRO cpu is busy reading this
           //printf(" DPORT_APPCPU_CTRL_D_REG  3ff00038");
           //return 0x28;
           return sim_DPORT_APPCPU_CTRL_D_REG;
           break;

      case 0x48:
           printf("DPORT_PRO_CACHE_LOCK_0_ADDR_REG 3ff00048=%08X\n\n",sim_DPORT_PRO_CACHE_LOCK_0_ADDR_REG);
           return sim_DPORT_PRO_CACHE_LOCK_0_ADDR_REG;
           break;

       case 0x40:
           printf(" DPORT_PRO_CACHE_CTRL_REG  3ff00040=%08X\n",sim_DPORT_PRO_CACHE_CTRL_REG);
           return sim_DPORT_PRO_CACHE_CTRL_REG;
           break;

       case 0x44:
           printf("DPORT_PRO_CACHE_CTRL1_REG  3ff00044=8E6\n");
           return sim_DPORT_PRO_CACHE_CTRL1_REG;
           //return 0x8E6;
           break;

       case 0x58:
           printf(" DPORT_APP_CACHE_CTRL_REG  3ff00058=%08X\n",sim_DPORT_APP_CACHE_CTRL_REG);
           // OLAS was 1
           return sim_DPORT_APP_CACHE_CTRL_REG;
           break;

       case 0x5C:
           printf(" DPORT_APP_CACHE_CTRL1_REG  3ff0005C=%08X\n",sim_DPORT_APP_CACHE_CTRL1_REG);
           return (sim_DPORT_APP_CACHE_CTRL1_REG);
           break;

      case 0x5F0:
            // This can be used to test if we are running qemu or actual hardware
            printf("(qemu) QEMU_TEST_REGISTER\n");
            return(0x42);
      case 0x3F0:
           printf(" DPORT_PRO_DCACHE_DBUG0_REG  3ff003F0=0x80\n");
           return 0x80;
           break;

           //while (GET_PERI_REG_BITS2(DPORT_APP_DCACHE_DBUG0_REG, DPORT_APP_CACHE_S
      case 0x418:
           printf(" DPORT_APP_DCACHE_DBUG0_REG  3ff00418=0x80\n");
           return 0x80;
           break;

        case 0x42000:
           printf(" SPI1_CMD_REG 3ff42000=0\n");
           return 0x0;
           break;

        case 0x43000:
           printf(" SPI0_CMD_REG 3ff42000=0\n");
           return 0x0;
           break;


        case 0x42010:
           printf(" SPI_CMD_REG1 3ff42010=0\n");
           return 0x0;
           break;

        case 0x43010:
           printf(" SPI_CMD_REG1 3ff42010=0\n");
           return 0x0;
           break;


        case 0x420f8:
           // The status of spi state machine
           printf(" SPI1 state machine 3ff420f8=\n");
           return 0x0;
           break;

        case 0x430f8:
           // The status of spi state machine
           printf(" SPI0 state status 3ff430f8=\n");
           return 0x0;
           break;
           
        case 0x44038:
           printf("GPIO_STRAP_REG 3ff44038=0x13\n");
           // boot_sel_chip[5:0]: MTDI, GPIO0, GPIO2, GPIO4, MTDO, GPIO5
           //                             1                    1     1
           return 0x13;
           break;
        
        case 0x47004:
            //  READ_PERI_REG(FRC_TIMER_COUNT_REG(0));
            {
                static int timerCountReg=0;
                timerCountReg++;  // Ticks??
                return(timerCountReg);
            }
            break;
        case 0x48044:
           printf("RTC_CNTL_INT_ST_REG 3ff48044=0x0\n");
           return 0x0;
           break;

        case 0x48034:
           printf("RTC_CNTL_RESET_STATE_REG 3ff48034=3390\n");
           return 0x0003390;
           break;

    case 0x48854:
           printf("SENS_SAR_MEAS_START1_REG,3ff48854 =ffffffff\n");        
           return 0xffffffff;
           break;

       case 0x480b4:
           printf("RTC_CNTL_STORE5_REG 3ff480b4=%08X\n",sim_RTC_CNTL_STORE5_REG);
           return sim_RTC_CNTL_STORE5_REG;
           break;

       case 0x4800c:
           printf("RTC_CNTL_TIME_UPDATE_REG 3ff4800c=%08X\n",0x40000000);
           return 0x40000000;
           break;

       // Handled by i2c 
       //case 0x5302c:
       //    printf(" I2C INTSTATUS 3ff5302c=%08X\n",0x0);
       //    return 0x0;
       //    break;


        case 0x58040:
           printf("SDIO or WAKEUP??\n"); 
           static int silly=0;
           return silly;
           break;
    
      // Mac adress with crc, 24:0a:c4:00:c8:70,   crc ec
      case 0x5a004:
           printf("EFUSE MAC\n"); 
           return 0xc400c870;
           break;
      case 0x5a008:
           printf("EFUSE MAC\n"); 
           return 0xffda240a;
           break;


       case 0x5a010:
           printf("EFUSE_BLK0_RDATA4_REG 3ff5a010=01\n");
           return 0x01;
           break;

       case 0x53060:
           printf("I2C command done 3ff53060=ffffffff\n");
           return 0xffffffff;
           break;

       case 0x53008:
           printf("I2C status reg 3ff53008=ffffffff\n");
           return 0xffffffff;
           break;

       case 0x5a018:
           printf("EFUSE_BLK0_RDATA6_REG 3ff5a018=01\n");
           return 0x01;
           break;
           //case 0xb05f0:
           //printf(" boot_time_lock 3ffb05f0=01\n");

       case 0x5f06c:
           printf("TIMG_RTCCALICFG1_REG 3ff5f06c=25\n");
           return 0x25;
           break;
        case 0x48088:
            printf("RTC_CNTL_DIG_ISO_REG 3ff48088=%08X\n",sim_RTC_CNTL_DIG_ISO_REG);
            return sim_RTC_CNTL_DIG_ISO_REG;
            break;
        case 0x40020:
            printf("UART data=0\n");
            return 0x0;
            break;
        case 0x6a014:
            printf("EMAC_GMACGMIIDATA_REG");
            return 0x2000;
           break;                       

       default:
          {
            if (addr>0x40000 && addr<0x40400) 
            {
                if (addr!=0x04001c) printf("UART READ");
            }
          }

          if (addr!=0x04001c) printf("\n");

    }

    return 0x0;
}



XtensaCPU *APPcpu = NULL;

XtensaCPU *PROcpu = NULL;



static void esp_io_write(void *opaque, hwaddr addr,
        uint64_t val, unsigned size)
{

/* Flash MMU table for PRO CPU */
//#define DPORT_PRO_FLASH_MMU_TABLE ((volatile uint32_t*) 0x3FF10000)

/* Flash MMU table for APP CPU */
//#define DPORT_APP_FLASH_MMU_TABLE ((volatile uint32_t*) 0x3FF12000)
// (addr>0x10000 && addr<0x11ffc) ||

if ((addr==0x60054) ||
    (addr==0x60050) ||
    (addr==0x60060) ||
    (addr==0x60064)) {
    return;
}

if (addr>=0x10000 && addr<0x11ffc) {
  if (addr==0x100c8) {
    // Bootloader, loads instruction cache
    if (sim_DPORT_PRO_CACHE_CTRL1_REG==0x8ff) {
        mapFlashToMem(val*0x10000, 0x3f720000,0x10000);            
    }
  }


  if (addr==0x10134 || addr==0x10138 || addr==0x1013c || addr==0x10140 || 
      addr==0x10144 || addr==0x10148 || addr==0x1014c || addr==0x10150 ) {
    // Bootloader, loads instruction cache
    if (sim_DPORT_PRO_CACHE_CTRL1_REG==0x8ff) {
        // TO TEST BOOTLOADER UNCOMMENT THIS ---->
        //mapFlashToMem(val*0x10000, 0x400d0000+(val-5)*0x10000,0x10000);            
    }
  }


  if (addr==0x10000) {
     if (sim_DPORT_PRO_CACHE_CTRL1_REG==0x8ff) {
            // Partition table
            // 0x8000
            // TO TEST BOOTLOADER ----> Ignore  nv_init_called when testing bootloader         
            if (nv_init_called) {
                // Data is located and used at 0x3f400000  0x3f404000 ???
                // Try this for bootloader
                // TO TEST BOOTLOADER UNCOMMENT THIS ---->
                //mapFlashToMem(val*0x10000, 0x3f400000,0x10000);  
                // for application.. flash.rodata is would be overwritten if mapped on 0x3f400000 
                if (val!=0x100) {
                    mapFlashToMem(val*0x10000 + 0x7000, 0x3f407000,0x10000-0x7000); 
                }
            }
      }
   }
   // TO TEST BOOTLOADER comment test for nv_init_called ---->
   if (nv_init_called) {
        if (addr==0x10004) {
                mapFlashToMem(val*0x10000, 0x3f410000,0x10000);
        }
        if (addr==0x10008) {
                mapFlashToMem(val*0x10000, 0x3f420000,0x10000);
        }
        if (addr==0x1000c) {
                mapFlashToMem(val*0x10000, 0x3f430000,0x10000);
        }
        if (addr==0x10010) {
                mapFlashToMem(val*0x10000, 0x3f440000,0x10000);
        }
        if (addr==0x10014) {
                mapFlashToMem(val*0x10000, 0x3f450000,0x10000);
        }
        if (addr==0x10018) {
                mapFlashToMem(val*0x10000, 0x3f460000,0x10000);
        }

   }


    pro_MMU_REG[addr/4-0x10000/4]=val;
    if (val!=0 && val!=0x100) {
       fprintf (stderr, "(qemu) MMU %" PRIx64 "  %" PRIx64 "\n" ,addr,val); 
    }
}

if (addr>=0x12000 && addr<0x13ffc) {
    app_MMU_REG[addr/4-0x12000/4]=val;
}



    if ( addr==0x69440 || addr==0x69454 || addr==0x6945c || addr==0x69458
     ||  (addr>=0x69440 && addr<=0x6947c) || addr==0x44008  || addr==0x4400c
    )  {
        // Cache MMU table?
    } else {
       if (addr!=0x40000) printf("io write %" PRIx64 ",%" PRIx64 " \n",addr,val);
    }
    switch (addr) {

        case 0x2c:
            //SET_PERI_REG_MASK(DPORT_APPCPU_CTRL_A_REG, DPORT_APPCPU_RESETTING); 
            if (val==1) {
                printf("DPORT_APPCPU_CTRL_A_REG 0x3ff0002c\n");
                if (APPcpu) {
                    printf("RESET 0x3ff0002c\n");
                    //CPUClass *cc = CPU_CLASS(APPcpu);
                    //XtensaCPUClass *xcc = XTENSA_CPU_GET_CLASS(APPcpu);
                    CPUXtensaState *env = &APPcpu->env;

                    xtensa_runstall(env,false);
                    
                }
            }
            break; 

        case 0x38:
            printf("DPORT_APPCPU_CTRL_D_REG 3ff00038\n");
            sim_DPORT_APPCPU_CTRL_D_REG=val;
            break;

        case 0x48:
            printf("DPORT_PRO_CACHE_LOCK_0_ADDR_REG 3ff00048\n");
            sim_DPORT_PRO_CACHE_LOCK_0_ADDR_REG=val;
            break;            

        case 0x40:
            {
                printf("DPORT_PRO_CACHE_CTRL_REG 3ff00040\n");
                sim_DPORT_PRO_CACHE_CTRL_REG = val;

            }
            break; 

        case 0x44:
           {
              printf(" DPORT_PRO_CACHE_CTRL1_REG  3ff00044  %" PRIx64 "\n" ,val);

                sim_DPORT_PRO_CACHE_CTRL1_REG=val;
           }
           break;

        case 0x10000:
           {
              printf(" MMU CACHE  3ff10000  %" PRIx64 "\n" ,val);
           }
           break;

        case 0x5C:
           printf(" DPORT_APP_CACHE_CTRL1_REG  3ff0005C=%08X\n",val);
           sim_DPORT_APP_CACHE_CTRL1_REG=val;
           break;

        case 0x58:
           printf(" DPORT_APP_CACHE_CTRL_REG  3ff00058\n");
           sim_DPORT_APP_CACHE_CTRL_REG=val;
           break;

        case 0x5F0:
            // TODO!! CHECK IF UNUSED, Just unpatches the rom patches
           printf(" OLAS_EMULATION_ROM_UNPATCH  3ff005F0\n");
           {
             FILE *f_rom=fopen("rom.bin", "r");
            
             if (f_rom == NULL) {
                   fprintf(stderr,"   Can't open 'rom.bin' for reading.\n");
	        } else {
                 unsigned int *rom_data=(unsigned int *)malloc(0x63000*sizeof(unsigned int));
                                                              //62ccc last patch adress
                 if (fread(rom_data,0x63000*sizeof(unsigned char),1,f_rom)<1) {
                    fprintf(stderr," File 'rom.bin' is truncated or corrupt.\n");                
                 }
                 cpu_physical_memory_write(0x40000000, rom_data, 0x63000*sizeof(unsigned char));
                fprintf(stderr,"Rom is restored.\n");
                free(rom_data);
            }
           }

/*
            // Load data to partiotion to make  nvs_flash_init() happy
            {
                fprintf(stderr,"Flash map data to memory\n");
                printf("Flash map data to memory\n");
                // I dont know how this works. Guessing
                
                FILE *f_flash = fopen("esp32flash.bin", "r");

                if (f_flash == NULL)
                {
                    fprintf(stderr, "   Can't open 'esp32flash.bin' for reading.\n");
                }
                else
                {
                    unsigned int *rom_data = (unsigned int *)malloc(64*0x400 * sizeof(unsigned char));
                    if (fread(rom_data, 64*0x400 * sizeof(unsigned char), 1, f_flash) < 1)
                    {
                        fprintf(stderr, " File 'esp32flash.bin' is truncated or corrupt.\n");
                    }
                    cpu_physical_memory_write(0x3f720000, rom_data, 64*0x400 * sizeof(unsigned char));

                    cpu_physical_memory_write(0x3f410000, rom_data, 64*0x400 * sizeof(unsigned char));
                    fprintf(stderr, "Flash partition data is loaded.\n");
                }
                
            }
*/

           break;
        case 0xcc:
           printf("EMAC_CLK_EN_REG %" PRIx64 "\n" ,val);
           // REG_SET_BIT(EMAC_CLK_EN_REG, EMAC_CLK_EN); 
           break;

       case 0x1c8:
           printf("DPORT_PRO_I2C_EXT0_INTR_MAP_REG %" PRIx64 "\n" ,val);   
           //esp32_i2c_interruptSet(PROcpu->env.irq_inputs[val]);
           break;

       case 0x18c:
           printf("DPORT_PRO_UART_INTR_MAP_REG %" PRIx64 "\n" ,val);  
           uart0_irq=PROcpu->env.irq_inputs[(int)val];
           break;

        case 0x48000:
           {
              printf("RTC_CNTL_OPTIONS0_REG, 3ff48000\n");
              if (val==0x30) {
                if (APPcpu) {
                    cpu_reset(APPcpu);
                    xtensa_runstall(&APPcpu->env, true);
                }
                if (PROcpu) {
                    cpu_reset(PROcpu);
                    //0x400076dd
                    PROcpu->env.pc=0x40000400;
                    xtensa_runstall(&PROcpu->env, false);
                }

              }
           }
           break;
        case 0x48088:
           printf("RTC_CNTL_DIG_ISO_REG 3ff48088\n");
           sim_RTC_CNTL_DIG_ISO_REG=val;
           break;
       case 0x480b4:
           printf("RTC_CNTL_STORE5_REG 3ff480b4 %" PRIx64 "\n" ,val);
           sim_RTC_CNTL_STORE5_REG=val;
           break;

       case 0x40000:
            // Outupt uart data to stderr.
            fprintf(stderr,"%c",(char)val);
            break;

       case 0x69000: 
           printf("EMAC_DMABUSMODE_REG %" PRIx64 "\n" ,val);
           break;

       case 0x69804:
            printf("EMAC_EX_OSCCLK_CONF_REG %" PRIx64 "\n" ,val);
            break;

       case 0x69808:
            printf("EMAC_EX_CLK_CTRL_REG %" PRIx64 "\n" ,val);
            break;

            // desc2 = 0x3ffb214c, desc3 = 0x3ffb9e6c
            // desc2 = 0x3ffb408c, desc3 = 0x3ffb9f0c
            // desc2 = 0x3ffb598c, desc3 = 0x3ffb9e4c

       case  0x6980c:
            printf("EMAC_EX_PHYINF_CONF_REG 3ff6980c %" PRIx64 "\n" ,val);
            // Outupt uart data to stderr.
            //fprintf(stderr,"%c",(char)val);
              //emac_enable_clk(true);                                                                                                  
              //REG_SET_FIELD(EMAC_EX_PHYINF_CONF_REG, EMAC_EX_PHY_INTF_SEL, EMAC_EX_PHY_INTF_RMII);      
            break;
       case 0x69018:
           //REG_SET_BIT(EMAC_DMAOPERATION_MODE_REG, EMAC_FORWARD_UNDERSIZED_GOOD_FRAMES);
           printf("EMAC_DMAOPERATION_MODE_REG %" PRIx64 "\n" ,val);
           break;
       case 0x6a010:
            printf("EMAC_GMACGMIIADDR_REG %" PRIx64 "\n" ,val);
           break;
       case 0x6a014:
            printf("EMAC_GMACGMIIDATA_REG %" PRIx64 "\n" ,val);
           break;                       

       default:
          if (addr>0x40000 && addr<0x40400) 
          {
              printf("UART OUTPUT");
          }
          //printf("\n");
    }

}

typedef struct Esp32WifiState {

    uint32_t reg[0x10000];
    int i2c_block;
    int i2c_reg;
} Esp32WifiState;

static unsigned char guess=0x98;


static uint64_t esp_wifi_read(void *opaque, hwaddr addr,
        unsigned size)
{

    Esp32WifiState *s=opaque;

    printf("wifi read %" PRIx64 " \n",addr);

    // e004   rom i2c 
    // e044   rom i2c
/*
    uint8_t rom_i2c_readReg(uint8_t block, uint8_t host_id, uint8_t reg_add);
    uint8_t rom_i2c_readReg_Mask(uint8_t block, uint8_t host_id, uint8_t reg_add, uint8_t msb, uint8_t lsb);
    void rom_i2c_writeReg(uint8_t block, uint8_t host_id, uint8_t reg_add, uint8_t pData);
    void rom_i2c_writeReg_Mask(uint8_t block, uint8_t host_id, uint8_t reg_add, uint8_t msb, uint8_t lsb, uint8_t indata);
stack 0x3ffe3cc0,
p/x $a2-7
0x62,0x01,0x06,  0x03,0x00, -256 (0xffffff00)

#0  0x400041c3 in rom_i2c_readReg_Mask ()

(gdb) p/x $a2
$35 = 0x62
(gdb) p/x $a3
$36 = 0x1
(gdb) p/x $a4
$37 = 0x6

wifi read e044 
wifi write e044,fffff0ff 
wifi read e044 
wifi write e044,fffff7ff 
wifi read e004 
wifi write e004,662 
wifi read e004 
wifi read e004 


get_rf_freq_init

#-1 #0  0x40004148 in rom_i2c_readReg ()
#0  0x400041c3 in rom_i2c_readReg_Mask ()
#1  0x40085b3c in get_rf_freq_cap (freq=<optimized out>, freq_offset=0, x_reg=<optimized out>,
    cap_array=0x3ffe3d0f "") at phy_chip_v7_ana.c:810
#2  0x40085cd9 in get_rf_freq_init () at phy_chip_v7_ana.c:900
#3  0x40086aae in set_chan_freq_hw_init (tx_freq_offset=2 '\002', rx_freq_offset=4 '\004') at phy_chip_v7_ana.c:1492
#4  0x40086d01 in rf_init () at phy_chip_v7_ana.c:795
#5  0x40089b6a in register_chipv7_phy (init_param=<optimized out>, rf_cal_data=0x3ffb79e8 "", rf_cal_level=2 '\002')
#6  0x400d13f0 in esp_phy_init (init_data=0x3f401828 <phy_init_data>, mode=PHY_RF_CAL_FULL,
    calibration_data=0x3ffb79e8) at /home/olas/esp/esp-idf/components/esp32/./phy_init.c:50
#7  0x400d0e43 in do_phy_init () at /home/olas/esp/esp-idf/components/esp32/./cpu_start.c:295


rom_i2c_reg block 0x62 reg 0x6 02
bf
4f
88
77
b4
00
02
00
00
07
b0
08
00
00
00
00
rom_i2c_reg block 0x67 reg 0x6 57
00
b5
bf
41
43
55
57
55
57
71
10
71
10
00
ea
ec
*/


/*    
rom_i2c_reg block 0x67 reg 0x6 57
*/

    if (addr==0xe004) {
        //fprintf(stderr,"(qemu ret) internal i2c block 0x62 %02x %d\n",s->i2c_reg,guess );
    }

    if (s->i2c_block==0x67) {
        if (addr==0xe004) {
            //fprintf(stderr,"(qemu ret) internal i2c block 0x67 %02x\n",s->i2c_reg );            
            switch (s->i2c_reg) {
                case 0: return 0x00;
                case 1: return 0xb5;
                case 2: return 0xbf;
                case 3: return 0x41;
                case 4: return 0x43;
                case 5: return 0x55;
                case 6: return 0x57;
                case 7: return 0x55;
                case 8: return 0x57;
                case 9: return 0x71;
                case 10: return 0x10;
                case 11: return 0x71;
                case 12: return 0x10;
                case 13: return 0x00;
                case 14: return 0xea;
                case 15: return 0xec;   
                default: return 0xff;
            }
        }
    }



    if (s->i2c_block==0x62) {
        if (addr==0xe004) {

            //fprintf(stderr,"(qemu ret) internal i2c block 0x62 %02x %d\n",s->i2c_reg,guess );
            
            switch (s->i2c_reg) {
                case 0: return 0xbf;
                case 1: return 0x4f;
                case 2: return 0x88;
                case 3: return 0x77;
                case 4: return 0xb4;
                case 5: return 0x00;
                case 6: return 0x02;
                case 7: return (100*guess++);
/*              case 8: return 0x00;
                case 9: return 0x07;
                case 10: return 0xb0;
                case 11: return 0x08;
*/
                default: return 0xff;
            }
        }
    }

    switch(addr) {
    case 0x0:
        printf("0x0000000000000000\n");
        break;
    case 0x40:
        return 0x00A40100;
        break;
    case 0x10000:
        printf("0x10000 UART 1 AHB FIFO ???\n");
        break;
    case 0xe04c:
        return 0xffffffff;
        break;
    case 0xe0c4:
        return 0xffffffff;
        break;
    case 0x607c:
        return 0xffffffff;
        break;
    case 0x1c018:
        //return 0;
        return 0x11E15054;
        //return 0x80000000;
        // Some difference should land between these values
              // 0x980020c0;
              // 0x980020b0;
        //return   0x800000;
        break;
    case 0x33c00:
        return 0x0002BBED;
        break;
        //return 0x01000000;
        {
         //static int test=0x0002BBED;
         //printf("(qemu) %d\n",test);
         //return (test--);
        }
        //return 0;
        //return 
    default:
        return(s->reg[addr]);
        break;
    }

    return 0x0;
}

extern void esp32_i2c_fifo_dataSet(int offset,unsigned int data);

extern void esp32_i2c_interruptSet(qemu_irq new_irq);


static void esp_wifi_write(void *opaque, hwaddr addr,
        uint64_t val, unsigned size)
{
    Esp32WifiState *s=opaque;


    // 0x01301c
    if (addr>=0x01301c && addr<=0x01311c) {        
        fprintf(stderr,"i2c apb write %" PRIx64 ",%" PRIx64 " \n",addr,val);
        //esp32_i2c_fifo_dataSet((addr-0x01301c)/4,val);
    }



    pthread_mutex_lock(&mutex);


    if (addr==0xe004) {
      s->i2c_block= val & 0xff;
      s->i2c_reg= (val>>8) & 0xff;
      //if (s->i2c_block!=0x62 && s->i2c_block!=0x67) 
      {
         //fprintf(stderr,"(qemu) internal i2c %02x %d\n",s->i2c_block,s->i2c_reg );
      }
    }


    if (addr==0x0) {
        // FIFO UART NUM 0 --  UART_FIFO_AHB_REG
        if (gdb_serial[0]->gdb_serial_connected) {
            gdb_serial[0]->gdb_serial_data[gdb_serial[0]->gdb_serial_buff_tx%MAX_GDB_BUFF]=(char)val;
            gdb_serial[0]->gdb_serial_buff_tx++;
        }
        fprintf(stderr,"ZZZ %c",val);
        uint8_t buf[1] = { val };
        qemu_chr_fe_write(silly_serial, buf, 1);

    } else if (addr==0x10000) {
        // FIFO UART NUM 1 --  UART_FIFO_AHB_REG
        if (gdb_serial[1]->gdb_serial_connected) {
            gdb_serial[1]->gdb_serial_data[gdb_serial[1]->gdb_serial_buff_tx%MAX_GDB_BUFF]=(char)val;
            gdb_serial[1]->gdb_serial_buff_tx++;
        }

        fprintf(stderr,"YY %c",val);
    } else if (addr==0x2e000) {
        // FIFO UART NUM 2  -- UART_FIFO_AHB_REG
        if (gdb_serial[2]->gdb_serial_connected) {
            gdb_serial[2]->gdb_serial_data[gdb_serial[2]->gdb_serial_buff_tx%MAX_GDB_BUFF]=(char)val;
            gdb_serial[2]->gdb_serial_buff_tx++;
        }
        fprintf(stderr,"XX %c",val);
    } else {
        printf("wifi write %" PRIx64 ",%" PRIx64 " \n",addr,val);
    }
    pthread_mutex_unlock(&mutex);

    s->reg[addr]=val;
}



static const MemoryRegionOps esp_io_ops = {
    .read = esp_io_read,
    .write = esp_io_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};


static const MemoryRegionOps esp_wifi_ops = {
    .read = esp_wifi_read,
    .write = esp_wifi_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};



// MMU not setup? Or maybe cpu_get_phys_page_debug returns wrong adress.
// We try this mapping instead to get us started
static uint64_t translate_esp32_address(void *opaque, uint64_t addr)
{
    return addr;
}



static void esp32_init(const ESP32BoardDesc *board, MachineState *machine)
{
    int be = 0;
    int i;

    Esp32 *esp32 = g_malloc0(sizeof(*esp32));
    MemoryRegion *system_memory = get_system_memory();
    XtensaCPU *cpu = NULL;
    CPUXtensaState *env = NULL;
    Esp32SpiState *spi;
    void *flash_image;
    DeviceState *dev;
    I2CBus *i2c;



    MemoryRegion *ram,*ram1, *rom, *system_io, *ulp_slowmem;
    static MemoryRegion *wifi_io;

    DriveInfo *dinfo;
    pflash_t *flash = NULL;
    //pflash_t *flash2 = NULL;

    QemuOpts *machine_opts = qemu_get_machine_opts();
    const char *cpu_model = machine->cpu_model;
    const char *kernel_filename = qemu_opt_get(machine_opts, "kernel");
    const char *kernel_cmdline = qemu_opt_get(machine_opts, "append");
    const char *dtb_filename = qemu_opt_get(machine_opts, "dtb");
    const char *initrd_filename = qemu_opt_get(machine_opts, "initrd");

    pthread_t pgdb_socket_thread;
    int q;
        
    printf("esp32_init\n");
    for (q=0;q<3;q++) {
        if( pthread_create( &pgdb_socket_thread , NULL ,  gdb_socket_thread , (void*) q) < 0)
        {
            printf("Failed to create gdb connection thread\n");
        }
    }

    if (!cpu_model) {
        cpu_model = "esp32";
    }


    for (i = 0; i < 2; ++i) {
        static const uint32_t prid[] = {
            0xcdcd,
            0xabab,
        };
        XtensaCPU *cpu = cpu_xtensa_init(cpu_model);

        if (cpu == NULL) {
            error_report("unable to find CPU definition '%s'",
                         cpu_model);
            exit(EXIT_FAILURE);
        }

        if (i==1) {
          APPcpu =cpu;
        }

        if (i==0) {
          PROcpu=cpu;
        }
        
        esp32->cpu[i] = cpu;
        cpu->env.sregs[PRID] = prid[i];
    }

    qemu_register_reset(esp32_reset, esp32);

    nv_init_called=false;
    // TO TEST BOOTLOADER maybe dont initialise these ---->

    // Initate same as after running bootloader
    //pro_MMU_REG[0]=2;
    //app_MMU_REG[0]=2;
    //pro_MMU_REG[50]=4;
    // This requires a valid flash image
    //mapFlashToMem(0x0000, 0x3f400000,0x10000);

    pro_MMU_REG[77]=4;
    app_MMU_REG[77]=4;
    pro_MMU_REG[78]=5;
    app_MMU_REG[78]=5;
    //app_MMU_REG[79]=6;
    //pro_MMU_REG[79]=6;


    // Map all as ram 
    ram = g_malloc(sizeof(*ram));
    // memory_region_init_ram(ram, NULL, "iram0", 0x1fffffff,  // 00000
    //                       &error_abort);
     memory_region_init_ram(ram, NULL, "iram0", 0x20000000,  // 00000
                           &error_abort);

    vmstate_register_ram_global(ram);
    memory_region_add_subregion(system_memory, 0x20000000, ram);




    ram1 = g_malloc(sizeof(*ram1));
    memory_region_init_ram(ram1, NULL, "iram1",  0xBFFFFF,  
                           &error_abort);

    vmstate_register_ram_global(ram1);
    memory_region_add_subregion(system_memory,0x40000000, ram1);


// ULP


    ulp_slowmem = g_malloc(sizeof(*ulp_slowmem));
    memory_region_init_io(ulp_slowmem, NULL, &ulp_ops, NULL, "esp32.ulpmem",
                          0x80000);

    memory_region_add_subregion(system_memory, 0x50000000, ulp_slowmem);


    // Cant really see this in documentation, maybe leftover from ESP31
    //rambb = g_malloc(sizeof(*rambb));
    //memory_region_init_ram(rambb, NULL, "rambb",  0xBFFFFF,  
    //                       &error_abort);

    //vmstate_register_ram_global(rambb);
    //memory_region_add_subregion(system_memory,0x60000000, rambb);


    // iram0  -- 0x4008_0000, len = 0x20000


    // Sram1 can be remapped by 
    // setting bit 0 of register DPORT_PRO_BOOT_REMAP_CTRL_REG or
    // DPORT_APP_BOOT_REMAP_CTRL_REG


    // sram1 -- 0x400B_0000 ~ 0x400B_7FFF

    // dram0 3ffc0000 

    system_io = g_malloc(sizeof(*system_io));
    memory_region_init_io(system_io, NULL, &esp_io_ops, NULL, "esp32.io",
                          0x80000);

    memory_region_add_subregion(system_memory, 0x3ff00000, system_io);


   Esp32WifiState *s = g_malloc(sizeof(Esp32WifiState));


   wifi_io = g_malloc(sizeof(*wifi_io));
   memory_region_init_io(wifi_io, NULL, &esp_wifi_ops, s, "esp32.wifi",
                              0x80000);

   memory_region_add_subregion(system_memory, 0x60000000, wifi_io);
   

    esp32_fpga_init(system_io, 0x0d020000);

    if (nd_table[0].used) {
        printf("Open net\n");
        open_net_init(system_memory,0x3ff69000,0x3ff69400 , 0x3FFF8000,
                      esp32->cpu[0]->env.irq_inputs[9]  //env->irq_inputs[9]
                      , nd_table);   //   xtensa_get_extint(env, 9)
    } 

    env = (CPUXtensaState *) &esp32->cpu[0]->env;


    if (!serial_hds[0]) {
        printf("New serial device\n");
        serial_hds[0] = qemu_chr_new("serial0", "replay.txt");
    }

    //esp32_serial_init(system_io, 0x40000, "esp32.uart0",
    //                    xtensa_get_extint(env, 5), serial_hds[0]);


    //  xtensa_get_extint(env, 5)

    gdb_serial[0]=esp32_serial_init(0,system_io, 0x40000, "esp32.uart0",
                        esp32->cpu[0]->env.irq_inputs[0], serial_hds[0]);

                        uart0_irq=esp32->cpu[0]->env.irq_inputs[0];


    gdb_serial[1]=esp32_serial_init(1,system_io, 0x50000, "esp32.uart1",
                        esp32->cpu[0]->env.irq_inputs[1], serial_hds[0]);

     
    // CHECK app.elf  with p _xt_interrupt_table to see what interrupts that are used
    // b xt_set_interrupt_handler   UART2 has interrupt soucre 36??
    // b esp_intr_alloc_intrstatus

    gdb_serial[2]=esp32_serial_init(2,system_io, 0x6e000, "esp32.uart2",
                          env->irq_inputs[2], serial_hds[0]);



    //printf("No call to serial__mm_init\n");

                            //0x0d050020
    //serial_mm_init(system_io, 0x3FF40020 , 2, xtensa_get_extint(env, 0),
    //        115200, serial_hds[0], DEVICE_NATIVE_ENDIAN);

   /*SerialState *serial_mm_init(MemoryRegion *address_space,
                            hwaddr base, int it_shift,
                            qemu_irq irq, int baudbase,
                            CharDriverState *chr, enum device_endian end)
   */
    // What interrupt for this???
    //serial_mm_init(system_io, 0x40000 , 2, xtensa_get_extint(env, 0),
    //        9600, serial_hds[0], DEVICE_LITTLE_ENDIAN);

    //serial_mm_init(system_io, 0x40000 , 2, xtensa_get_extint(env, 0),
    //        115200, serial_hds[0], DEVICE_LITTLE_ENDIAN);

    dinfo = drive_get(IF_PFLASH, 0, 0);
    if (dinfo) {
        printf("FLASH EMULATION!!!!\n");
        flash = pflash_cfi01_register(0x3ff42000 /*board->flash_base*/,
                NULL, "esp32.io.flash", board->flash_size,
                blk_by_legacy_dinfo(dinfo),
                board->flash_sector_size,
                board->flash_size / board->flash_sector_size,
                4, 0x0000, 0x0000, 0x0000, 0x0000, be);
        if (flash == NULL) {
            error_report("unable to mount pflash");
            exit(EXIT_FAILURE);
        }

       // OLAS, SPI flash 
       /*
        MemoryRegion *flash_mr = pflash_cfi01_get_memory(flash);
        MemoryRegion *flash_io = g_malloc(sizeof(*flash_io));

        memory_region_init_alias(flash_io, NULL, "esp32.io.flash",
                flash_mr, board->flash_boot_base,
                board->flash_size - board->flash_boot_base < 0x02000000 ?
                board->flash_size - board->flash_boot_base : 0x02000000);
        memory_region_add_subregion(system_memory, 0xfe000000,
                flash_io);

       */


    }


spi = esp32_spi_init(2,system_io, 0x64000, "esp32.spi0",
                    system_memory, /*cache*/ 0x800000, "esp32.flash",
                    xtensa_get_extint(&esp32->cpu[0]->env, 6), &flash_image);

spi = esp32_spi_init(1,system_io, 0x43000, "esp32.spi0",
                    system_memory, /*cache*/ 0x800000, "esp32.flash",
                    xtensa_get_extint(&esp32->cpu[0]->env, 6), &flash_image);


spi = esp32_spi_init(0,system_io, 0x42000, "esp32.spi1",
                    system_memory, /*cache*/ 0x800000, "esp32.flash.odd",
                    xtensa_get_extint(&esp32->cpu[0]->env, 6), &flash_image);


    // OLED & tempsensor , on i2c0
#if 0
    if (true) {  // 0x40020000
        // qdev_get_gpio_in(nvic, 8)
        // I2c0
        dev = sysbus_create_simple(TYPE_ESP32_I2C, 0x3FF53000 , NULL);
        i2c = (I2CBus *)qdev_get_child_bus(dev, "i2c");
        if (true) {
            i2c_create_slave(i2c, "ssd1306", 0x78);
            //i2c_create_slave(i2c, "ssd1306", 0x02);
            i2c_create_slave(i2c, "tmpbme280", 0x77);
        }
    }
#endif




    /* Use kernel file name as current elf to load and run */
    if (kernel_filename) {
        uint32_t entry_point =  esp32->cpu[0]->env.pc; //env->pc;
        size_t bp_size = 3 * get_tag_size(0); /* first/last and memory tags */
        //uint32_t tagptr = 0xfe000000 + board->sram_size;
        //uint32_t cur_tagptr;
        //BpMemInfo memory_location = {
        //    .type = tswap32(MEMORY_TYPE_CONVENTIONAL),
        //    .start = tswap32(0),
        //    .end = tswap32(machine->ram_size),
        //};
        //uint32_t lowmem_end = machine->ram_size < 0x08000000 ?
        //    machine->ram_size : 0x08000000;
        //uint32_t cur_lowmem = QEMU_ALIGN_UP(lowmem_end / 2, 4096);

        rom = g_malloc(sizeof(*rom));
        memory_region_init_ram(rom, NULL, "lx60.sram", 0x20000,
                               &error_abort);
        vmstate_register_ram_global(rom);
        memory_region_add_subregion(system_memory, 0x40000000, rom);

        if (kernel_cmdline) {
            bp_size += get_tag_size(strlen(kernel_cmdline) + 1);
        }
        if (dtb_filename) {
            bp_size += get_tag_size(sizeof(uint32_t));
        }
        if (initrd_filename) {
            bp_size += get_tag_size(sizeof(BpMemInfo));
        }

        /* Put kernel bootparameters to the end of that SRAM */
        #if 0
        tagptr = (tagptr - bp_size) & ~0xff;
        cur_tagptr = put_tag(tagptr, BP_TAG_FIRST, 0, NULL);
        cur_tagptr = put_tag(cur_tagptr, BP_TAG_MEMORY,
                             sizeof(memory_location), &memory_location);

        if (kernel_cmdline) {
            cur_tagptr = put_tag(cur_tagptr, BP_TAG_COMMAND_LINE,
                                 strlen(kernel_cmdline) + 1, kernel_cmdline);
        }

        if (dtb_filename) {
            int fdt_size;
            void *fdt = load_device_tree(dtb_filename, &fdt_size);
            uint32_t dtb_addr = tswap32(cur_lowmem);

            if (!fdt) {
                error_report("could not load DTB '%s'", dtb_filename);
                exit(EXIT_FAILURE);
            }

            cpu_physical_memory_write(cur_lowmem, fdt, fdt_size);
            cur_tagptr = put_tag(cur_tagptr, BP_TAG_FDT,
                                 sizeof(dtb_addr), &dtb_addr);
            cur_lowmem = QEMU_ALIGN_UP(cur_lowmem + fdt_size, 4096);
        }
 
        if (initrd_filename) {
            BpMemInfo initrd_location = { 0 };
            int initrd_size = load_ramdisk(initrd_filename, cur_lowmem,
                                           lowmem_end - cur_lowmem);

            if (initrd_size < 0) {
                initrd_size = load_image_targphys(initrd_filename,
                                                  cur_lowmem,
                                                  lowmem_end - cur_lowmem);
            }
            if (initrd_size < 0) {
                error_report("could not load initrd '%s'", initrd_filename);
                exit(EXIT_FAILURE);
            }
            initrd_location.start = tswap32(cur_lowmem);
            initrd_location.end = tswap32(cur_lowmem + initrd_size);
            cur_tagptr = put_tag(cur_tagptr, BP_TAG_INITRD,
                                 sizeof(initrd_location), &initrd_location);
            cur_lowmem = QEMU_ALIGN_UP(cur_lowmem + initrd_size, 4096);
        }

        cur_tagptr = put_tag(cur_tagptr, BP_TAG_LAST, 0, NULL);
        env->regs[2] = tagptr;
#endif

        uint64_t elf_entry;
        uint64_t elf_lowaddr;
        int success = load_elf(kernel_filename, translate_esp32_address, cpu,
                &elf_entry, &elf_lowaddr, NULL, be, EM_XTENSA, 0,0);
        if (success > 0) {
            entry_point = elf_entry;
        } else {
            hwaddr ep;
            int is_linux;
            success = load_uimage(kernel_filename, &ep, NULL, &is_linux,
                                  translate_esp32_address, cpu);
            if (success > 0 && is_linux) {
                entry_point = ep;
            } else {
                error_report("could not load kernel '%s'",
                             kernel_filename);
                exit(EXIT_FAILURE);
            }
        }
        if (entry_point !=esp32->cpu[0]->env.pc) {
            env = (CPUXtensaState *) &esp32->cpu[0]->env;
            // elf entypoint differs, set up 
            printf("Elf entry %08X\n",entry_point);
            static const uint8_t jx_a0[] = {
                0xa0, 0, 0,
            };            
            esp32->cpu[0]->env.regs[0] = entry_point;

            cpu_physical_memory_write(env->pc, jx_a0, sizeof(jx_a0));

#if 0
            static const uint8_t rfde[] = {
                0x0, 0x32, 0,
            };

            // Return from double interrupt
            //cpu_physical_memory_write(0x400003c0, rfde, sizeof(rfde));


            // Return interrupt
            static const uint8_t rfe[] = {
                0x0, 0x30, 0,
            };
#endif

            //cpu_physical_memory_write(0x40000300, rfe, sizeof(rfe));

            // Add rom from file
            FILE *f_rom=fopen("rom.bin", "r");
            
            if (f_rom == NULL) {
               printf("   Can't open 'rom.bin' for reading.\n");
	        } else {
                unsigned int *rom_data=(unsigned int *)malloc(0xC2000*sizeof(unsigned int));
                if (fread(rom_data,0xC1FFF*sizeof(unsigned char),1,f_rom)<1) {
                   printf(" File 'rom.bin' is truncated or corrupt.\n");                
                }
                cpu_physical_memory_write(0x40000000, rom_data, 0xC1FFF*sizeof(unsigned char));
                fclose(f_rom);
            }

            FILE *f_rom1=fopen("rom1.bin", "r");
            
            if (f_rom1 == NULL) {
               printf("   Can't open 'rom1.bin' for reading.\n");
	        } else {
                unsigned int *rom1_data=(unsigned int *)malloc(0x10000*sizeof(unsigned int));
                if (fread(rom1_data,0x10000*sizeof(unsigned char),1,f_rom1)<1) {
                   printf(" File 'rom1.bin' is truncated or corrupt.\n");                
                }
                cpu_physical_memory_write(0x3FF90000, rom1_data, 0xFFFF*sizeof(unsigned char));
                fclose(f_rom1);
            }


#if 1

            // Patching rom function. ets_unpack_flash_code
            //      j forward 0x13 
            //      nop.n * 8
            //      l32r     a9,?       a9 with ram adress user_code_start 0x3ffe0400
            //      l32r     a8,?       a8 with entry_point (from load_elf())
            //      s32i.n   a9,a8,0
            //      movi.n   a2, 0
            //      retw.n
            static const uint8_t patch_ret[] = {
                 0x06,0x05,0x00,  // Jump
                 0x3d,0xf0,   // NOP
                 0x3d,0xf0,   // NOP
                 0x3d,0xf0,   // NOP
                 0x3d,0xf0,   // NOP
                 0x3d,0xf0,   // NOP
                 0x3d,0xf0,   // NOP 
                 0x3d,0xf0,   // NOP 
                 0x3d,0xf0,   // NOP 
                 0x3d,0xf0,   // NOP 
                 0x3d,0xf0,   // NOP 
                 0xff,        // To get next instruction correctly
                 0x91, 0xfc, 0xff, 0x81 , 0xfc , 0xff , 0x99, 0x08 , 0xc, 0x2, 0x1d, 0xf0
            };

            // Patch rom, ets_unpack_flash_code
            cpu_physical_memory_write(0x40007018+3, patch_ret, sizeof(patch_ret));

            // Add entry point in rom patch 
            cpu_physical_memory_write(0x40007024, &entry_point, 1*sizeof(unsigned int));

            // Add user_code_start to rom
            unsigned int location=0x3ffe0400;
            cpu_physical_memory_write(0x40007028, &location , 1*sizeof(unsigned int));


           //Patch SPIEraseSector b *0x40062ccc
           static const uint8_t retw_n[] = {
               0xc, 0x2,0x1d, 0xf0
           };

            // Patch rom, SPIEraseSector 0x40062ccc
            //cpu_physical_memory_write(0x40062ccc+3, retw_n, sizeof(retw_n));

            // Patch rom,  SPIWrite 0x40062d50
            //cpu_physical_memory_write(0x40062d50+3, retw_n, sizeof(retw_n));

            //rom_txdc_cal_init, 0x40004e10)
            //cpu_physical_memory_write(0x40004e10+3, retw_n, sizeof(retw_n));

            // Not working so well..
            // Not rom,  ram_txdc_cal_v70 0x4008753b 
            //cpu_physical_memory_write(0x4008753b+3, retw_n, sizeof(retw_n));



            //esp_phy_init, 0x400d0e2f
            //cpu_physical_memory_write(0x4000522d+3, retw_n, sizeof(retw_n));


            // This would have been nicer, however ram will be cleared by rom-functions later... 
            //cpu_physical_memory_write(0x3ffe0400, &entry_point, 1*sizeof(unsigned int));


            // Skip bootloader initialisation, jump to the fresh elf
            //cpu_physical_memory_write(env->pc, jx_a0, sizeof(jx_a0));

            // Overwrite rom interrupt function
            //cpu_physical_memory_write(0x400003c0, rfde, sizeof(rfde));
#endif
        }
    } else {
        // No elf, try booting from flash...
        if (flash) {
#if 0
            MemoryRegion *flash_mr = pflash_cfi01_get_memory(flash);
            MemoryRegion *flash_io = g_malloc(sizeof(*flash_io));

            memory_region_init_alias(flash_io, NULL, "esp32.flash",
                    flash_mr, board->flash_boot_base,
                    board->flash_size - board->flash_boot_base < 0x02000000 ?
                    board->flash_size - board->flash_boot_base : 0x02000000);
            memory_region_add_subregion(system_memory, 0xfe000000,
                    flash_io);
#endif                    
        }
    }
}

static void xtensa_esp32_init(MachineState *machine)
{
    static const ESP32BoardDesc esp32_board = {
        //.flash_base = 0xf0000000,
        .flash_base = 0x3ff42000,
        .flash_size = 0x01000000,
        .flash_boot_base = 0x06000000,
        .flash_sector_size = 0x20000,
        .sram_size = 0x2000000,
    };
    esp32_init(&esp32_board, machine);
}

/*
*/

static void xtensa_esp32_class_init(ObjectClass *oc, void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);

    mc->desc = "esp32 DEV (esp32)";
    mc->init = xtensa_esp32_init;
    mc->max_cpus = 2;

}

static const TypeInfo xtensa_esp32_machine = {
    .name = MACHINE_TYPE_NAME("esp32"),
    .parent = TYPE_MACHINE,
    .class_init = xtensa_esp32_class_init,
};


static void xtensa_esp32_machines_init(void)
{
    type_register_static(&xtensa_esp32_machine);
}

type_init(xtensa_esp32_machines_init);
