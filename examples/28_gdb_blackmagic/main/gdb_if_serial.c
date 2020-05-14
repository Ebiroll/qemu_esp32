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

/******************************************************************************
 * Description: interface to gdb over wifi
 *******************************************************************************/

#include "rom/ets_sys.h"
#include "soc/uart_reg.h"
#include "soc/io_mux_reg.h"
//#include "esp_gdbstub.h"
#include <string.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "lwip/sockets.h"
#include "soc/cpu.h"
#include "gdb_if.h"

#define USE_SERIAL CONFIG_USE_SERIAL

//Length of buffer used to reserve GDB commands. Has to be at least able to fit the G command, which
//implies a minimum size of about 320 bytes.
#define PBUFLEN 512
static char chsum;						//Running checksum of the output packet

#define ATTR_GDBFN


int gdb_socket=0;
#define IN_BUFFER_LEN 512
static unsigned char inbuf[IN_BUFFER_LEN];
static int in_buf_head = 0;
static int in_buf_received = 0;

#if defined(USE_SERIAL) 


int ATTR_GDBFN gdbRecvChar() {
	int i;
	while (((READ_PERI_REG(UART_STATUS_REG(0))>>UART_RXFIFO_CNT_S)&UART_RXFIFO_CNT)==0) ;
	i=READ_PERI_REG(UART_FIFO_REG(0));
	return i;
}

//Send a char to the uart.
void ATTR_GDBFN gdbSendChar(char c) {
	while (((READ_PERI_REG(UART_STATUS_REG(0))>>UART_TXFIFO_CNT_S)&UART_TXFIFO_CNT)>=126) ;
	WRITE_PERI_REG(UART_FIFO_REG(0), c);
}








//Send the start of a packet; reset checksum calculation.
void ATTR_GDBFN gdbPacketStart() {
	chsum=0;
	gdbSendChar('$');
}

//Send a char as part of a packet
void ATTR_GDBFN gdbPacketChar(char c) {
	if (c=='#' || c=='$' || c=='}' || c=='*') {
		gdbSendChar('}');
		gdbSendChar(c^0x20);
		chsum+=(c^0x20)+'}';
	} else {
		gdbSendChar(c);
		chsum+=c;
	}
}

//Send a string as part of a packet
void ATTR_GDBFN gdbPacketStr(char *c) {
	while (*c!=0) {
		gdbPacketChar(*c);
		c++;
	}
}

int gdb_if_init(void) 
{
	return 0;
}
unsigned char gdb_if_getchar(void) {
	return gdbRecvChar();
}

// 
// https://github.com/espressif/esp-idf/issues/5101

uint32_t platform_time_ms(void)
{
    //return xTaskGetTickCount() / portTICK_PERIOD_MS;
	int64_t time_milli=esp_timer_get_time()/1000;
	return((uint32_t)time_milli);
}	

unsigned char gdb_if_getchar_to(int timeout) {
    unsigned char ret=0;
    uint32_t now;
    uint32_t start=platform_time_ms();
    do {
        if (((READ_PERI_REG( UART_STATUS_REG(0))>> UART_RXFIFO_CNT_S) & UART_RXFIFO_CNT)!=0) {
            return (READ_PERI_REG(UART_FIFO_REG(0)));
        }
        now=platform_time_ms();
    } while((now-start)<timeout);
	return ret;
}



void gdb_if_putchar(unsigned char c, int flush)
{
    gdbSendChar(c);
}

// See also
//gdbSendChar(c);
//gdbPacketChar(c);

//Send a hex val as part of a packet. 'bits'/4 dictates the number of hex chars sent.
void ATTR_GDBFN gdbPacketHex(int val, int bits) {
	char hexChars[]="0123456789abcdef";
	int i;
	for (i=bits; i>0; i-=4) {
		gdbPacketChar(hexChars[(val>>(i-4))&0xf]);
	}
}

void gdbPacketFlush() {
    int  nwrote;
    // TODO!!
}

//Finish sending a packet.
void ATTR_GDBFN gdbPacketEnd() {
	gdbSendChar('#');
	gdbPacketHex(chsum, 8);
	gdbPacketFlush();
}


/* 
//Register format as the Xtensa HAL has it:
STRUCT_FIELD (long, 4, XT_STK_EXIT,     exit)
STRUCT_FIELD (long, 4, XT_STK_PC,       pc)
STRUCT_FIELD (long, 4, XT_STK_PS,       ps)
STRUCT_FIELD (long, 4, XT_STK_A0,       a0)
[..]
STRUCT_FIELD (long, 4, XT_STK_A15,      a15)
STRUCT_FIELD (long, 4, XT_STK_SAR,      sar)
STRUCT_FIELD (long, 4, XT_STK_EXCCAUSE, exccause)
STRUCT_FIELD (long, 4, XT_STK_EXCVADDR, excvaddr)
STRUCT_FIELD (long, 4, XT_STK_LBEG,   lbeg)
STRUCT_FIELD (long, 4, XT_STK_LEND,   lend)
STRUCT_FIELD (long, 4, XT_STK_LCOUNT, lcount)
// Temporary space for saving stuff during window spill 
STRUCT_FIELD (long, 4, XT_STK_TMP0,   tmp0)
STRUCT_FIELD (long, 4, XT_STK_TMP1,   tmp1)
STRUCT_FIELD (long, 4, XT_STK_TMP2,   tmp2)
STRUCT_FIELD (long, 4, XT_STK_VPRI,   vpri)
STRUCT_FIELD (long, 4, XT_STK_OVLY,   ovly)
#endif
STRUCT_END(XtExcFrame)
*/






void set_gdb_socket(int socket) {
   gdb_socket=socket;
}


int gdb_if_is_running(void) {
	if (gdb_socket>0) {
		return 1;
	}
	return 0;
}

void gdb_if_close(void) {
	close(gdb_socket);
	gdb_socket=0;
}

#endif