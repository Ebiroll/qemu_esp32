/*
 * Copyright (c) 2011, Max Filippov, Open Source and Linux Lab.
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
// wifi ... 0x6000e0c4=0xffffffff
// 0x6000e04c
// 0x6000607c
// set *((int *) 0x6000e0c4)=-1

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
#include <poll.h>

bool gdb_serial_connected=false;
#define MAX_GDB_BUFF  4096
char gdb_serial_buff[MAX_GDB_BUFF];
int  gdb_serial_buff_rd=0;
int  gdb_serial_buff_tx=0;


#define DEBUG_LOG(...) fprintf(stderr, __VA_ARGS__)

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
    ESP32_UART_STATUS,
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

typedef struct Esp32SerialState {
    MemoryRegion iomem;
    CharDriverState *chr;
    qemu_irq irq;

    unsigned rx_first;
    unsigned rx_last;
    uint8_t rx[ESP32_UART_FIFO_SIZE];
    uint32_t reg[ESP32_UART_MAX];
} Esp32SerialState;

static unsigned esp32_serial_rx_fifo_size(Esp32SerialState *s)
{
    return (s->rx_last - s->rx_first) & ESP32_UART_FIFO_MASK;
}

static int esp32_serial_can_receive(void *opaque)
{
    Esp32SerialState *s = opaque;

    return esp32_serial_rx_fifo_size(s) != ESP32_UART_FIFO_SIZE - 1;
}

static void esp32_serial_irq_update(Esp32SerialState *s)
{
    s->reg[ESP32_UART_INT_ST] |= s->reg[ESP32_UART_INT_RAW];
    if (s->reg[ESP32_UART_INT_ST] & s->reg[ESP32_UART_INT_ENA]) {
        qemu_irq_raise(s->irq);
    } else {
        qemu_irq_lower(s->irq);
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

    if ((addr & 3) || size != 4) {
        return 0;
    }

    switch (addr / 4) {
    case ESP32_UART_STATUS:
        return esp32_serial_rx_fifo_size(s);

    case ESP32_UART_FIFO:
        if (esp32_serial_rx_fifo_size(s)) {
            uint8_t r = s->rx[s->rx_first++];

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
        qemu_log("%s: unexpected read @0x%"HWADDR_PRIx"\n", __func__, addr);
        break;
    }
    return 0;
}

static void esp32_serial_receive(void *opaque, const uint8_t *buf, int size)
{
    Esp32SerialState *s = opaque;
    unsigned i;

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
    if (ESP32_UART_GET(s, CONF0, LOOPBACK)) {
        if (esp32_serial_can_receive(s)) {
            uint8_t buf[] = { (uint8_t)val };

            esp32_serial_receive(s, buf, 1);
        }

    } else if (s->chr) {
        uint8_t buf[1] = { val };
        qemu_chr_fe_write(s->chr, buf, 1);
        fprintf(stderr,"%c",(char)val);
        if (gdb_serial_connected) {
            gdb_serial_buff[gdb_serial_buff_tx++%MAX_GDB_BUFF]=(char)val;
        }

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

Esp32SerialState *gdb_serial=NULL;

static Esp32SerialState *esp32_serial_init(MemoryRegion *address_space,
                                               hwaddr base, const char *name,
                                               qemu_irq irq,
                                               CharDriverState *chr)
{
    Esp32SerialState *s = g_malloc(sizeof(Esp32SerialState));

    s->chr = chr;
    s->irq = irq;
    qemu_chr_add_handlers(s->chr, esp32_serial_can_receive,
                          esp32_serial_receive, NULL, s);
    memory_region_init_io(&s->iomem, NULL, &esp32_serial_ops, s,
                          name, 0x100);
    memory_region_add_subregion(address_space, base, &s->iomem);
    qemu_register_reset(esp32_serial_reset, s);
    gdb_serial=s;
    return s;
}

/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    int read_size;
    char *message , client_message[2000];

    gdb_serial_connected=true;
     
    printf("Started gdb socket\n");

    //struct timeval tv;
    //tv.tv_sec = 0;  /* 30 Secs Timeout */
    //tv.tv_usec = 100000;  // Not init'ing this can cause strange errors
    //setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));

    //message = "$T08\n";
    //write(sock , message , strlen(message));


    //Receive a message from client
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
                read_size = recv(sock , client_message , 1 , 0);
                break;
        }

        if (gdb_serial) {
            if (read_size>0)  {
                esp32_serial_receive(gdb_serial,client_message,read_size);
                printf("%s",client_message);
            }
        }

        int to_send=gdb_serial_buff_tx-gdb_serial_buff_rd;
        if (gdb_serial_buff_rd<gdb_serial_buff_tx) {
                //printf("%s",&gdb_serial_buff[gdb_serial_buff_rd]);
                int pos=gdb_serial_buff_rd;
                while(pos<gdb_serial_buff_tx) {
                    printf("%c",gdb_serial_buff[pos%MAX_GDB_BUFF]);
                    if (gdb_serial_buff[pos%MAX_GDB_BUFF]=='+')
                    {
                        printf("\n");
                    }
                    pos++;
                }
                if (gdb_serial_buff_rd + to_send<MAX_GDB_BUFF)
                {
                       write(sock , &gdb_serial_buff[gdb_serial_buff_rd] , to_send);
                       gdb_serial_buff_rd+=to_send;
                }
                else 
                {
                    while(gdb_serial_buff_rd<gdb_serial_buff_tx) {
                       write(sock , &gdb_serial_buff[(gdb_serial_buff_rd++)%MAX_GDB_BUFF] , 1);
                    }
                }
        }

    } while (true);
     
    if(read_size == 0)
    {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("recv failed");
    }
         
    //Free the socket pointer
    free(socket_desc);
     
    return 0;
}

void *gdb_socket_thread(void *dummy)
{
    int socket_desc , client_sock , c , *new_sock;
    struct sockaddr_in server , client;
     
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    } else {
        int enable = 1;
        if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
            error("setsockopt(SO_REUSEADDR) failed");
    }
    puts("Socket created");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 8888 );
     
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
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
        new_sock = malloc(1);
        *new_sock = client_sock;
         
        if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0)
        {
            perror("could not create thread");
            return 1;
        }
         
        //Now join the thread , so that we dont terminate before the thread
        //pthread_join( sniffer_thread , NULL);
        puts("Handler assigned");
    }
     
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
     
    return 0;

}


typedef struct ESP32BoardDesc {
    hwaddr flash_base;
    size_t flash_size;
    size_t flash_boot_base;
    size_t flash_sector_size;
    size_t sram_size;
} ESP32BoardDesc;

typedef struct Esp32FpgaState {
    MemoryRegion iomem;
    uint32_t leds;
    uint32_t switches;
} Esp32FpgaState;

static void lx60_fpga_reset(void *opaque)
{
    Esp32FpgaState *s = opaque;

    s->leds = 0;
    s->switches = 0;
}

static uint64_t lx60_fpga_read(void *opaque, hwaddr addr,
        unsigned size)
{
    Esp32FpgaState *s = opaque;

    switch (addr) {
    case 0x0: /*build date code*/
        return 0x09272011;

    case 0x4: /*processor clock frequency, Hz*/
        return 10000000;

    case 0x8: /*LEDs (off = 0, on = 1)*/
        return s->leds;

    case 0xc: /*DIP switches (off = 0, on = 1)*/
        return s->switches;
    }
    return 0;
}

static void lx60_fpga_write(void *opaque, hwaddr addr,
        uint64_t val, unsigned size)
{
    Esp32FpgaState *s = opaque;

    switch (addr) {
    case 0x8: /*LEDs (off = 0, on = 1)*/
        s->leds = val;
        break;

    case 0x10: /*board reset*/
        if (val == 0xdead) {
            qemu_system_reset_request();
        }
        break;
    }
}

static const MemoryRegionOps lx60_fpga_ops = {
    .read = lx60_fpga_read,
    .write = lx60_fpga_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static Esp32FpgaState *esp32_fpga_init(MemoryRegion *address_space,
        hwaddr base)
{
    Esp32FpgaState *s = g_malloc(sizeof(Esp32FpgaState));

    //memory_region_init_io(&s->iomem, NULL, &lx60_fpga_ops, s,
    //        "lx60.fpga", 0x10000);
    //memory_region_add_subregion(address_space, base, &s->iomem);
    lx60_fpga_reset(s);
    qemu_register_reset(lx60_fpga_reset, s);
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

static void esp_xtensa_ccompare_cb(void *opaque)
{
    XtensaCPU *cpu = opaque;
    CPUXtensaState *env = &cpu->env;
    CPUState *cs = CPU(cpu);

    if (cs->halted) {
        env->halt_clock = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
        xtensa_advance_ccount(env, env->wake_ccount - env->sregs[CCOUNT]);
        if (!cpu_has_work(cs)) {
            env->sregs[CCOUNT] = env->wake_ccount + 1;
            xtensa_rearm_ccompare_timer(env);
        }
    }
}


static void lx60_reset(void *opaque)
{
    XtensaCPU *cpu = opaque;

    cpu_reset(CPU(cpu));


    CPUXtensaState *env = &cpu->env;

    env->ccompare_timer =
        timer_new_ns(QEMU_CLOCK_VIRTUAL, &esp_xtensa_ccompare_cb, cpu);



    env->sregs[146] = 0xABAB;


}

static unsigned int sim_RTC_CNTL_DIG_ISO_REG;

static unsigned int sim_RTC_CNTL_STORE5_REG=0;

static unsigned int sim_DPORT_PRO_CACHE_CTRL_REG=0x28;


static uint64_t esp_io_read(void *opaque, hwaddr addr,
        unsigned size)
{
    if (addr!=0x04001c) printf("io read %" PRIx64 " ",addr);

    switch (addr) {
       case 0x38:
           printf(" DPORT_PRO_CACHE_CTRL_REG  3ff00038=%08X\n",28);
           return 0x28;
           break;

       case 0x40:
           printf(" DPORT_PRO_CACHE_CTRL_REG  3ff00040=%08X\n",sim_DPORT_PRO_CACHE_CTRL_REG);
           return 0x28;
           break;

       case 0x44:
           printf(" DPORT DPORT_PRO_CACHE_CTRL1_REG  3ff00044=8E6\n");
           return 0x8E6;
           break;

       case 0x58:
           printf(" DPORT_APP_CACHE_CTRL_REG  3ff00058=0x01\n");
           return 0x01;
           break;

      case 0x3F0:
           printf(" DPORT_PRO_DCACHE_DBUG0_REG  3ff003F0=0x80\n");
           return 0x80;
           break;


        case 0x42000:
           printf(" SPI_CMD_REG 3ff42000=0\n");
           return 0x0;
           break;

        case 0x42010:
           printf(" SPI_CMD_REG1 3ff42010=0\n");
           return 0x0;
           break;


        case 0x420f8:
           // The status of spi state machine
           printf(" SPI_EXT2_REG 3ff420f8=\n");
           return 0x0;
           break;

        case 0x430f8:
           // The status of spi state machine
           printf(" SPI_EXT2_REG 3ff430f8=\n");
           return 0x0;
           break;

           

        case 0x44038:
           printf("GPIO_STRAP_REG 3ff44038=0x13\n");
           // boot_sel_chip[5:0]: MTDI, GPIO0, GPIO2, GPIO4, MTDO, GPIO5
           //                             1                    1     1
           return 0x13;
           break;
        
        case 0x48044:
           printf("RTC_CNTL_INT_ST_REG 3ff48044=0x0\n");
           return 0x0;
           break;

        case 0x48034:
           printf("RTC_CNTL_RESET_STATE_REG 3ff48034=3390\n");
           return 0x0003390;
           break;

       case 0x480b4:
           printf("RTC_CNTL_STORE5_REG 3ff480b4=%08X\n",sim_RTC_CNTL_STORE5_REG);
           return sim_RTC_CNTL_STORE5_REG;
           break;

       case 0x4800c:
           printf("RTC_CNTL_TIME_UPDATE_REG 3ff4800c=%08X\n",0x40000000);
           return 0x40000000;
           break;


        case 0x58040:
           printf("SDIO or WAKEUP??\n"); 
           static int silly=0;
           return silly;
           break;
       case 0x5a010:
           printf("EFUSE_BLK0_RDATA4_REG 3ff5a010=01\n");
           return 0x01;
           break;

       case 0x5a5a018:
           printf("EFUSE_BLK0_RDATA6_REG 3ff5a018=01\n");
           return 0x01;
           break;


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


static uint64_t esp_wifi_read(void *opaque, hwaddr addr,
        unsigned size)
{
    printf("wifi read %" PRIx64 " \n",addr);
    switch(addr) {
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
        return 0x980020b6;
        // Some difference should land between these values
              // 0x980020c0;
              // 0x980020b0;
        //return   0x800000;
    case 0x33c00:
        return 0x980020b6+0x980020b0;
        //return 0;
        //return 
    default:
        break;
    }

    return 0x0;
}


static void esp_wifi_write(void *opaque, hwaddr addr,
        uint64_t val, unsigned size)
{
    printf("wifi write %" PRIx64 ",%" PRIx64 " \n",addr,val);

}


static void esp_io_write(void *opaque, hwaddr addr,
        uint64_t val, unsigned size)
{

    if (addr>0x10000 && addr<0x11ffc) {
        // Cache MMU table
    } else {
       if (addr!=0x40000) printf("io write %" PRIx64 ",%" PRIx64 " ",addr,val);
    }
    switch (addr) {

        case 0x40:
            printf("DPORT_PRO_CACHE_CTRL_REG 3ff00040\n");
            sim_DPORT_PRO_CACHE_CTRL_REG=val;
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
       default:
          if (addr>0x40000 && addr<0x40400) 
          {
              printf("UART OUTPUT");
          }
          printf("\n");
    }

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

    MemoryRegion *system_memory = get_system_memory();
    XtensaCPU *cpu = NULL;
    CPUXtensaState *env = NULL;
    MemoryRegion *ram,*ram1, *rom, *system_io;  // *rambb,
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
    int n;

    pthread_t pgdb_socket_thread;
        
    printf("esp32_init\n");
    if( pthread_create( &pgdb_socket_thread , NULL ,  gdb_socket_thread , (void*) NULL) < 0)
    {
        printf("Failed to create gdb connection thread\n");
    }


    if (!cpu_model) {
        cpu_model = XTENSA_DEFAULT_CPU_MODEL;
    }

    for (n = 0; n < smp_cpus; n++) {
        cpu = cpu_xtensa_init(cpu_model);
        if (cpu == NULL) {
            error_report("unable to find CPU definition '%s'",
                         cpu_model);
            exit(EXIT_FAILURE);
        }
        env = &cpu->env;

        if (n==0) {
           env->sregs[PRID] = 0xCDCD;
        } else {
           env->sregs[PRID] = 0xABAB;            
        }
        qemu_register_reset(lx60_reset, cpu);
        /* Need MMU initialized prior to ELF loading,
         * so that ELF gets loaded into virtual addresses
         */
        cpu_reset(CPU(cpu));
    }

    // Internal rom, 0x4000_0000 ~ 0x4005_FFFF


    // Map all as ram 
    ram = g_malloc(sizeof(*ram));
    memory_region_init_ram(ram, NULL, "iram0", 0x1fffffff,  // 00000
                           &error_abort);

    vmstate_register_ram_global(ram);
    memory_region_add_subregion(system_memory, 0x20000000, ram);

    ram1 = g_malloc(sizeof(*ram1));
    memory_region_init_ram(ram1, NULL, "iram1",  0xBFFFFF,  
                           &error_abort);

    vmstate_register_ram_global(ram1);
    memory_region_add_subregion(system_memory,0x40000000, ram1);



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



   wifi_io = g_malloc(sizeof(*wifi_io));
   memory_region_init_io(wifi_io, NULL, &esp_wifi_ops, NULL, "esp32.wifi",
                              0x80000);

   memory_region_add_subregion(system_memory, 0x60000000, wifi_io);
   

    esp32_fpga_init(system_io, 0x0d020000);

    if (nd_table[0].used) {
        printf("Open net\n");
        open_net_init(system_memory,0x3FF76000, 0x3FF76400, 0x3FF76800,
                xtensa_get_extint(env, 9), nd_table);
    } 


    if (!serial_hds[0]) {
        printf("New serial device\n");
        serial_hds[0] = qemu_chr_new("serial0", "null",NULL);
    }

    //esp32_serial_init(system_io, 0x40000, "esp32.uart0",
    //                    xtensa_get_extint(env, 5), serial_hds[0]);

    printf("No call to serial__mm_init\n");

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

    /* Use kernel file name as current elf to load and run */
    if (kernel_filename) {
        uint32_t entry_point = env->pc;
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
        if (entry_point != env->pc) {
            // elf entypoint differs, set up 
            printf("Elf entry %08X\n",entry_point);
            static const uint8_t jx_a0[] = {
                0xa0, 0, 0,
            };
            env->regs[0] = entry_point;

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
                cpu_physical_memory_write(0x40000000, rom_data, 0xC1FFF*sizeof(unsigned int));
            }

            FILE *f_rom1=fopen("rom1.bin", "r");
            
            if (f_rom == NULL) {
               printf("   Can't open 'rom1.bin' for reading.\n");
	        } else {
                unsigned int *rom1_data=(unsigned int *)malloc(0x10000*sizeof(unsigned int));
                if (fread(rom1_data,0x10000*sizeof(unsigned char),1,f_rom1)<1) {
                   printf(" File 'rom1.bin' is truncated or corrupt.\n");                
                }
                cpu_physical_memory_write(0x3FF90000, rom1_data, 0xFFFF*sizeof(unsigned int));
            }


#if 1
            // Patching rom function. ets_unpack_flash_code
            //      j forward 0x13 
            //      nop.n * 8
            //  	l32r     a9,?       a9 with ram adress user_code_start 0x3ffe0400
	        //      l32r     a8,?       a8 with entry_point (from load_elf())
            //      s32i.n   a9,a8,0
            //      movi.n	a2, 0
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
            cpu_physical_memory_write(0x40062ccc+3, retw_n, sizeof(retw_n));

            // Patch rom,  SPIWrite 0x40062d50
            cpu_physical_memory_write(0x40062d50+3, retw_n, sizeof(retw_n));

            //rom_txdc_cal_init, 0x40004e10)
            cpu_physical_memory_write(0x40004e10+3, retw_n, sizeof(retw_n));

            // Not working so well..
            // Patch rom,  ram_txdc_cal_v70 0x4008753b 
            //cpu_physical_memory_write(0x4008753b+3, retw_n, sizeof(retw_n));


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
    mc->max_cpus = 4;

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
