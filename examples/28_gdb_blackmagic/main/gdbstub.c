/******************************************************************************
 * Copyright 2015 Espressif Systems
 *
 * Description: A stub to make the ESP8266 debuggable by GDB over the serial 
 * port.
 *
 * License: ESPRESSIF MIT License
 *******************************************************************************/

#include "gdbstub.h"

#include <xtensa/config/specreg.h>
#include <xtensa/config/core-isa.h>
#include <xtensa/corebits.h>

#include "gdbstub.h"
#include "gdbstub-entry.h"
#include "gdbstub-cfg.h"
#include "gdbstub-freertos.h"
#include "gdbstub-internal.h"

#include <sys/reent.h>
#include <stdlib.h>
#include <stdint.h>
#include "command.h"

//#include <esp/types.h>
//#include <esp/uart.h>
//#include <esp/uart_regs.h>
//#include <stdout_redirect.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


extern int  gdbRecvChar();
extern void  gdbSendChar(char c);

struct xtensa_exception_frame_t {
	uint32_t pc;
	uint32_t a[64];
	uint32_t lbeg;
	uint32_t lend;
	uint32_t lcount;
	uint32_t sar;
	uint32_t windowbase;
	uint32_t windowstart;
	uint32_t configid0;
	uint32_t configid1;
	uint32_t ps;
	uint32_t threadptr;
	uint32_t br;
	uint32_t scompare1;
	uint32_t acclo;
	uint32_t acchi;
	uint32_t m0;
	uint32_t m1;
	uint32_t m2;
	uint32_t m3;
	uint32_t expstate;  //I'm going to assume this is exccause...
	uint32_t f64r_lo;
	uint32_t f64r_hi;
	uint32_t f64s;
	uint32_t f[16];
	uint32_t fcr;
	uint32_t fsr;
	
#if 0
	uint32_t pc;
	uint32_t ps;
	uint32_t sar;
	uint32_t vpri;
	uint32_t a0;
	uint32_t a[14]; //a2..a15
	// These are added manually by the exception code; the HAL doesn't set these on an exception.
	uint32_t litbase;
	uint32_t sr176;
	uint32_t sr208;
	uint32_t a1;
	// 'reason' is abused for both the debug and the exception vector: if bit 7 is set,
	// this contains an exception reason, otherwise it contains a debug vector bitmap.
	uint32_t reason;
#endif
};

#include <string.h>
#include <stdio.h>

typedef void wdtfntype();

typedef enum {
	gdb_cmd_read_regs = 'g',
	gdb_cmd_write_regs = 'G',
	gdb_cmd_stop_reason = '?',
	gdb_cmd_memory_read = 'm',
	gdb_cmd_memory_write = 'M',
	gdb_cmd_query_ex = 'q',
	gdb_cmd_long_name = 'v',
	gdb_cmd_hw_breakpoint_set = 'Z',
	gdb_cmd_hw_breakpoint_clear = 'z',
	gdb_cmd_continue = 'c',
	gdb_cmd_single_step = 's',
	gdb_cmd_set_thread = 'H'
} gdb_serial_cmd_t;

static wdtfntype *wdt_feed = (wdtfntype *) 0x400d01b0;
//static wdtfntype *ets_wdt_enable = (wdtfntype *) 0x40002fa0;

// Length of buffer used to reserve GDB commands. Has to be at least able to fit the G command, which
// implies a minimum size of about 190 bytes.
#define PBUFLEN 256
#define ETS_UART_INUM 5

// Error states used by the routines that grab stuff from the incoming gdb packet
#define ST_ENDPACKET	-1
#define ST_ERR			-2
#define ST_OK			-3
#define ST_CONT			-4

// The asm stub saves the Xtensa registers here when a debugging exception happens.
struct xtensa_exception_frame_t gdbstub_savedRegs;

// This is the debugging exception stack.
uintptr_t gdbstub_exception_stack[256];

static unsigned char cmd[PBUFLEN];		// GDB command input buffer
static char gdbstub_packet_crc;			// Checksum of the output packet

static int32_t single_step_ps = -1;			// Stores ps when single-stepping instruction. -1 when not in use.

static void gdbstub_icount_ena_single_step() {
	__asm volatile (
		"wsr %0, ICOUNTLEVEL" "\n"
		"wsr %1, ICOUNT" "\n"
	:: "a" (XCHAL_DEBUGLEVEL), "a" (-2));

	__asm volatile ("isync");
}

static void gdbstub_single_step() {
	// single-step instruction
	// Single-stepping can go wrong if an interrupt is pending, especially when it is e.g. a task switch:
	// the ICOUNT register will overflow in the task switch code. That is why we disable interupts when
	// doing single-instruction stepping.
	single_step_ps = gdbstub_savedRegs.ps;
	gdbstub_savedRegs.ps = (gdbstub_savedRegs.ps & ~0xf) | (XCHAL_DEBUGLEVEL - 1);
	gdbstub_icount_ena_single_step();
}


// Send the start of a packet; reset checksum calculation.
void gdb_packet_start() {
	gdbstub_packet_crc = 0;
	gdbSendChar('$');
}

// Send a char as part of a packet
void gdb_packet_char(char c) {
	if (c=='#' || c=='$' || c=='}' || c=='*') {
		gdbSendChar('}');
		gdbSendChar(c ^ 0x20);
		gdbstub_packet_crc += (c ^ 0x20) + '}';
	} else {
		gdbSendChar(c);
		gdbstub_packet_crc += c;
	}
}

// Send a string as part of a packet
void gdb_packet_str(const char * c) {
	while (*c != 0) {
		gdb_packet_char(*c);
		c++;
	}
}

// Send a hex val as part of a packet. 'bits'/4 dictates the number of hex chars sent.
void gdb_packet_hex(int val, int bits) {
	char hexChars[] = "0123456789abcdef";
	int i;
	for (i = bits; i > 0; i -= 4) {
		gdb_packet_char(hexChars[(val >> (i - 4)) & 0xf]);
	}
}

// Finish sending a packet.
void gdb_packet_end() {
	gdbSendChar('#');
	gdb_packet_hex(gdbstub_packet_crc, 8);
}

// Grab a hex value from the gdb packet. Ptr will get positioned on the end
// of the hex string, as far as the routine has read into it. Bits/4 indicates
// the max amount of hex chars it gobbles up. Bits can be -1 to eat up as much
// hex chars as possible.
static long ATTR_GDBFN gdb_get_hex_val(uint8_t **ptr, size_t bits) {
	size_t i;
	uint32_t no;
	uint32_t v = 0;
	char c;
	no = bits / 4;
	if (bits == -1) {
		no = 64;
	}
	for (i = 0; i < no; i++) {
		c = **ptr;
		(*ptr)++;
		if (c >= '0' && c <= '9') {
			v <<= 4;
			v |= (c - '0');
		} else if (c >= 'A' && c <= 'F') {
			v <<= 4;
			v |= (c - 'A') + 10;
		} else if (c >= 'a' && c <= 'f') {
			v <<= 4;
			v |= (c - 'a') + 10;
		} else if (c == '#') {
			if (bits == -1) {
				(*ptr)--;
				return v;
			}
			return ST_ENDPACKET;
		} else {
			if (bits == -1) {
				(*ptr)--;
				return v;
			}
			return ST_ERR;
		}
	}
	return v;
}

// Read a byte from the ESP8266 memory.
static uint8_t ATTR_GDBFN mem_read_byte(uintptr_t p) {
	int * i = (int *) (p & (~3));

	// TODO: better address range check?
	if (p < 0x20000000 || p >= 0x60000000) {
		return -1;
	}

	return * i >> ((p & 3) * 8);
}

// Write a byte to the ESP8266 memory.
static void ATTR_GDBFN mem_write_byte(uintptr_t p, uint8_t d) {
	int *i = (int*) (p & (~3));

	if (p < 0x20000000 || p >= 0x60000000) {
		return;
	}

	if ((p & 3) == 0) {
		*i = (*i & 0xffffff00) | (d << 0);
	}

	if ((p & 3) == 1) {
		*i = (*i & 0xffff00ff) | (d << 8);
	}

	if ((p & 3) == 2) {
		*i = (*i & 0xff00ffff) | (d << 16);
	}

	if ((p & 3) == 3) {
		*i = (*i & 0x00ffffff) | (d << 24);
	}
}

// Returns 1 if it makes sense to write to addr p
static uint8_t ATTR_GDBFN mem_addr_valid(uintptr_t p) {
	#if 0
	if (p >= 0x3ff00000 && p < 0x40000000) {
		return 1;
	}

	if (p >= 0x40100000 && p < 0x40140000) {
		return 1;
	}

	if (p >= 0x60000000 && p < 0x60002000) {
		return 1;
	}
    #endif
	return 1;
}

/* 
 Register file in the format lx106 gdb port expects it.
 Inspired by gdb/regformats/reg-xtensa.dat from
 https://github.com/jcmvbkbc/crosstool-NG/blob/lx106-g%2B%2B/overlays/xtensa_lx106.tar
 As decoded by Cesanta.
*/
struct regfile {
	uint32_t a[16];
	uint32_t pc;
	uint32_t sar;
	uint32_t litbase;
	uint32_t sr176;
	uint32_t sr208;
	uint32_t ps;
};

// Send the reason execution is stopped to GDB.
static void ATTR_GDBFN gdb_send_reason() {
	// exception-to-signal mapping
	uint8_t exceptionSignal[] = { 4, 31, 11, 11, 2, 6, 8, 0, 6, 7, 0, 0, 7, 7, 7, 7 };
	size_t i = 0;

	gdb_packet_start();
	gdb_packet_char('T');

	if (gdbstub_savedRegs.expstate == 0xff) {
		gdb_packet_hex(2, 8); // sigint
	} else if (gdbstub_savedRegs.expstate & 0x80) {
		// We stopped because of an exception. Convert exception code to a signal number and send it.
		i = gdbstub_savedRegs.expstate & 0x7f;
		if (i < sizeof(exceptionSignal)) {
			gdb_packet_hex(exceptionSignal[i], 8);
		} else {
			gdb_packet_hex(11, 8);
		}
	} else {
		// We stopped because of a debugging exception.
		gdb_packet_hex(5, 8); // sigtrap
		// Current Xtensa GDB versions don't seem to request this, so let's leave it off.
#if 0
		if (gdbstub_savedRegs.reason&(1<<0)) {
			reason="break";
		}
		if (gdbstub_savedRegs.reason&(1<<1)) {
			reason="hwbreak";
		}
		if (gdbstub_savedRegs.reason&(1<<2)) {
			reason="watch";
		}
		if (gdbstub_savedRegs.reason&(1<<3)) {
			reason="swbreak";
		}
		if (gdbstub_savedRegs.reason&(1<<4)) {
			reason="swbreak";
		}

		gdb_packet_str(reason);
		gdb_packet_char(':');
		//TODO: watch: send address
#endif
	}

#if GDBSTUB_THREAD_AWARE
	gdbstub_freertos_report_thread();
#endif

	gdb_packet_end();
}

bool gdbstub_process_query(uint8_t* cmd) {
	char * query = (char *) &cmd[1];

	const char * q_supported = "Supported";

#if GDBSTUB_THREAD_AWARE
	const char * q_threads_read = "Xfer:threads:read";
	const char * features =
		"swbreak+;"
		"hwbreak+;"
		"qXfer:threads:read+;"
		"PacketSize=255";
#else
	const char * features =
		"swbreak+;"
		"hwbreak+;"
		"PacketSize=255";
#endif

	// TODO fix this shit
	if (strncmp(query, q_supported, 9) == 0) {
		// Capabilities query
		gdb_packet_start();
		gdb_packet_str(features);
		gdb_packet_end();
	}
#if GDBSTUB_THREAD_AWARE
	else if (strncmp(query, q_threads_read, 17) == 0) {
		gdbstub_freertos_task_list();
	}
#endif
	else {
		return false;
	}

	return true;
}

static void gdbstub_read_regs() {
#if GDBSTUB_THREAD_AWARE
	/*
	 * If the debugger wants to read state of the task
	 * not currently active, registers should be read
	 * from task stack.
	 */
	if (!gdbstub_freertos_task_selected()) {
		gdbstub_freertos_regs_read();
		return;
	}
#endif

	gdb_packet_start();
	gdb_packet_hex(bswap32(gdbstub_savedRegs.pc), 32);
	gdb_packet_hex(bswap32(gdbstub_savedRegs.a[0]), 32);
	gdb_packet_hex(bswap32(gdbstub_savedRegs.a[1]), 32);

	for (size_t i = 2; i < 64; i++) {
		gdb_packet_hex(bswap32(gdbstub_savedRegs.a[i - 2]), 32);
	}

	gdb_packet_hex(bswap32(gdbstub_savedRegs.lbeg), 32);
	gdb_packet_hex(bswap32(gdbstub_savedRegs.lend), 32);
	gdb_packet_hex(bswap32(gdbstub_savedRegs.lcount), 32);
	gdb_packet_hex(bswap32(gdbstub_savedRegs.sar), 32);
	gdb_packet_hex(bswap32(gdbstub_savedRegs.windowbase), 32);
	gdb_packet_hex(bswap32(gdbstub_savedRegs.windowstart), 32);
	gdb_packet_hex(bswap32(gdbstub_savedRegs.configid0), 32);
	gdb_packet_hex(bswap32(gdbstub_savedRegs.configid1), 32);
	gdb_packet_hex(bswap32(gdbstub_savedRegs.ps), 32);
	gdb_packet_hex(bswap32(gdbstub_savedRegs.threadptr), 32);
	gdb_packet_hex(bswap32(gdbstub_savedRegs.br), 32);
	gdb_packet_hex(bswap32(gdbstub_savedRegs.scompare1), 32);
	gdb_packet_hex(bswap32(gdbstub_savedRegs.acclo), 32);
	gdb_packet_hex(bswap32(gdbstub_savedRegs.acchi), 32);
	gdb_packet_hex(bswap32(gdbstub_savedRegs.m0), 32);
	gdb_packet_hex(bswap32(gdbstub_savedRegs.m1), 32);
	gdb_packet_hex(bswap32(gdbstub_savedRegs.m2), 32);


	gdb_packet_hex(bswap32(gdbstub_savedRegs.m3), 32);
	gdb_packet_hex(bswap32(gdbstub_savedRegs.expstate), 32);
	gdb_packet_hex(bswap32(gdbstub_savedRegs.f64r_lo), 32);
	gdb_packet_hex(bswap32(gdbstub_savedRegs.f64r_hi), 32);
	gdb_packet_hex(bswap32(gdbstub_savedRegs.f64s), 32);

	for (size_t i = 0; i < 16; i++) {
		gdb_packet_hex(bswap32(gdbstub_savedRegs.f[i]), 32);
	}
	gdb_packet_hex(bswap32(gdbstub_savedRegs.fcr), 32);
	gdb_packet_hex(bswap32(gdbstub_savedRegs.fsr), 32);

	gdb_packet_end();
}

// Handle a command as received from GDB.
int ATTR_GDBFN gdb_handle_command(uint8_t * cmd, size_t len) {
	// Handle a command
	int i, j, k;
	uint8_t * data = cmd + 1;

	switch (cmd[0]) {
	case gdb_cmd_read_regs:
		// send all registers to gdb
		gdbstub_read_regs();
		break;
	case gdb_cmd_write_regs:
		// receive content for all registers from gdb
		gdbstub_savedRegs.pc = bswap32(gdb_get_hex_val(&data, 32));

		gdbstub_savedRegs.a[0] = bswap32(gdb_get_hex_val(&data, 32));
		gdbstub_savedRegs.a[1] = bswap32(gdb_get_hex_val(&data, 32));

		for (i = 2; i < 64; i++) {
			gdbstub_savedRegs.a[i - 2] = bswap32(gdb_get_hex_val(&data, 32));
		}

		//gdbstub_savedRegs.sar = bswap32(gdb_get_hex_val(&data, 32));
		//gdbstub_savedRegs.litbase = bswap32(gdb_get_hex_val(&data, 32));
		//gdbstub_savedRegs.sr176 = bswap32(gdb_get_hex_val(&data, 32));

		gdb_get_hex_val(&data, 32);

		//gdbstub_savedRegs.ps = bswap32(gdb_get_hex_val(&data, 32));
	    gdb_packet_start();
		gdb_packet_str("OK");
		gdb_packet_end();
		break;
	case gdb_cmd_memory_read:
		// read memory to gdb
		i = gdb_get_hex_val(&data, -1);
		data++;
		j = gdb_get_hex_val(&data, -1);
		gdb_packet_start();
		for (k = 0; k < j; k++) {
			gdb_packet_hex(mem_read_byte(i++), 8);
		}
		gdb_packet_end();
		break;
	case gdb_cmd_memory_write:
		// write memory from gdb
		// addr
		i = gdb_get_hex_val(&data, -1);
		// skip
		data++;
		//length
		j = gdb_get_hex_val(&data, -1);
		data++;
		// skip
		if (mem_addr_valid(i) && mem_addr_valid(i+j)) {
			for (k = 0; k < j; k++) {
				mem_write_byte(i, gdb_get_hex_val(&data, 8));
				i++;
			}

			// Make sure caches are up-to-date. Procedure according to Xtensa ISA document, ISYNC inst desc.
			__asm volatile (
				"isync \n"
				"isync \n"
			);

			gdb_packet_start();
			gdb_packet_str("OK");
			gdb_packet_end();
		} else {
			//Trying to do a software breakpoint on a flash proc, perhaps?
			gdb_packet_start();
			gdb_packet_str("E01");
			gdb_packet_end();
		}
		break;
	case gdb_cmd_stop_reason:
		// Reply with stop reason
		gdb_send_reason();
		break;
	case gdb_cmd_continue:
		return ST_CONT;
		break;
	case gdb_cmd_single_step:
		gdbstub_single_step();
		return ST_CONT;
		break;
	case gdb_cmd_long_name:
		if (strncmp((char *)cmd, "vCont?", 6) == 0) {
			gdb_packet_start();
			gdb_packet_str("vCont;c;s;r");
			gdb_packet_end();
		} else if (strncmp((char *)cmd, "vCont;c", 7) == 0) {
			// continue execution
			return ST_CONT;
		} else if (strncmp((char *)cmd, "vCont;s", 7) == 0) {
			gdbstub_single_step();
			return ST_CONT;
		}
		break;
	case gdb_cmd_query_ex:
		// Extended query
		if (!gdbstub_process_query(cmd)) {
			// We weren't able to understand the query
			gdb_packet_start();
			gdb_packet_end();
			return ST_ERR;
		}
		break;
	case gdb_cmd_hw_breakpoint_set:
		// Set hardware break/watchpoint.
		// skip 'x,'
		data += 2;
		i = gdb_get_hex_val(&data, -1);
		// skip ','
		data += 1;
		j = gdb_get_hex_val(&data, -1);
		gdb_packet_start();

		// Set breakpoint
		if (cmd[1] == '1') {
			if (gdbstub_set_hw_breakpoint(i, j)) {
				gdb_packet_str("OK");
			} else {
				gdb_packet_str("E01");
			}
		} else if (cmd[1] == '2' || cmd[1] == '3' || cmd[1] == '4') {
			// Set watchpoint
			int access = 0;
			int mask = 0;

			switch (cmd[1]) {
			case '2':
				// write
				access = 2;
				break;
			case '3':
				// read
				access = 1;
				break;
			case '4':
				// access
				access = 3;
				break;
			}

			switch (j) {
			case 1:
				mask = 0x3F;
				break;
			case 2:
				mask = 0x3E;
				break;
			case 4:
				mask = 0x3C;
				break;
			case 8:
				mask = 0x38;
				break;
			case 16:
				mask = 0x30;
				break;
			case 32:
				mask = 0x20;
				break;
			case 64:
				mask = 0x00;
				break;
			}

			if (mask != 0 && gdbstub_set_hw_watchpoint(i, mask, access)) {
				gdb_packet_str("OK");
			} else {
				gdb_packet_str("E01");
			}
		}

		gdb_packet_end();
		break;
#if GDBSTUB_THREAD_AWARE
	case gdb_cmd_set_thread:
		// Set thread for memory and register operations
		if (cmd[1] == 'g') {
			data += 1;
			uint32_t thread = gdb_get_hex_val(&data, -1);
			gdbstub_freertos_task_select(thread);
		}

		gdb_packet_start();
		gdb_packet_str("OK");
		gdb_packet_end();
		break;
#endif
	case gdb_cmd_hw_breakpoint_clear:
		// Clear hardware break/watchpoint
		// skip 'x,'
		data += 2;
		i = gdb_get_hex_val(&data, -1);
		// skip ','
		data++;
		j = gdb_get_hex_val(&data, -1);
		gdb_packet_start();

		if (cmd[1]=='1') {
			// hardware breakpoint
			if (gdbstub_del_hw_breakpoint(i)) {
				gdb_packet_str("OK");
			} else {
				gdb_packet_str("E01");
			}
		} else if (cmd[1]=='2' || cmd[1]=='3' || cmd[1]=='4') {
			// hardware watchpoint
			if (gdbstub_del_hw_watchpoint(i)) {
				gdb_packet_str("OK");
			} else {
				gdb_packet_str("E01");
			}
		}

		gdb_packet_end();
		break;
	default:
		// We don't recognize or support whatever GDB just sent us.
		//gdb_packet_start();
		//gdb_packet_end();
		return ST_ERR;
	}

	return ST_OK;
}




int command_process(target *t, char *cmd) {
	int retval=gdb_handle_command((uint8_t *)cmd,strlen(cmd));
	if (retval==ST_OK) {
   	   return (-1);
	}
	else 
	{
     	return (1);
	}

}


//int ATTR_GDBFN gdb_handle_command(uint8_t * cmd, size_t len);

// Lower layer: grab a command packet and check the checksum
// Calls gdbHandleCommand on the packet if the checksum is OK
// Returns ST_OK on success, ST_ERR when checksum fails, a
// character if it is received instead of the GDB packet
// start char.
static int ATTR_GDBFN gdb_read_command() {
	uint8_t c;
	uint8_t chsum=0, rchsum;
	uint8_t sentchs[2];

	size_t p = 0;
	uint8_t * ptr;
	c = gdbRecvChar();

	if (c != '$') {
		return c;
	}

	while(1) {
		c = gdbRecvChar();
		if (c == '#') {	//end of packet, checksum follows
			cmd[p] = 0;
			break;
		}
		chsum += c;
		if (c == '$') {
			// Wut, restart packet?
			chsum = 0;
			p = 0;
			continue;
		}
		if (c == '}') {
			// escape the next char
			c = gdbRecvChar();
			chsum += c;
			c ^= 0x20;
		}
		cmd[p++] = c;
		if (p >= PBUFLEN) {
			return ST_ERR;
		}
	}

	// A # has been received. Get and check the received chsum.
	sentchs[0] = gdbRecvChar();
	sentchs[1] = gdbRecvChar();
	ptr = &sentchs[0];

	rchsum = gdb_get_hex_val(&ptr, 8);

	if (rchsum != chsum) {
		gdbSendChar('-');
		return ST_ERR;
	} else {
		gdbSendChar('+');
		return gdb_handle_command(cmd, p);
	}
}

//Get the value of one of the A registers
static uint32_t ATTR_GDBFN get_reg_val(size_t reg) {
	if (reg == 0) {
		return gdbstub_savedRegs.a[0];
	}
	if (reg == 1) {
		return gdbstub_savedRegs.a[1];
	}
	return gdbstub_savedRegs.a[reg - 2];
}

//Set the value of one of the A registers
static void ATTR_GDBFN set_reg_val(size_t reg, uint32_t val) {

	if (reg == 0) {
		gdbstub_savedRegs.a[0] = val;
	}
	if (reg == 1) {
		gdbstub_savedRegs.a[1] = val;
	}

	gdbstub_savedRegs.a[reg - 2] = val;
}

// Emulate the l32i/s32i instruction we've stopped at.
static void ATTR_GDBFN emulLdSt() {
	uint8_t i0 = mem_read_byte(gdbstub_savedRegs.pc);
	uint8_t i1 = mem_read_byte(gdbstub_savedRegs.pc + 1);
	uint8_t i2 = mem_read_byte(gdbstub_savedRegs.pc + 2);

	uintptr_t * p;
	if ((i0 & 0xf) == 2 && (i1 & 0xf0) == 0x20) {
		// l32i
		p = (uintptr_t *) get_reg_val(i1 & 0xf) + (i2 * 4);
		set_reg_val(i0 >> 4, *p);
		gdbstub_savedRegs.pc += 3;
	} else if ((i0 & 0xf) == 0x8) {
		// l32i.n
		p = (uintptr_t *) get_reg_val(i1 & 0xf) + ((i1 >> 4) * 4);
		set_reg_val(i0 >> 4, *p);
		gdbstub_savedRegs.pc += 2;
	} else if ((i0 & 0xf) == 2 && (i1 & 0xf0) == 0x60) {
		// s32i
		p = (uintptr_t *) get_reg_val(i1 & 0xf) + (i2 * 4);
		*p = get_reg_val(i0 >> 4);
		gdbstub_savedRegs.pc += 3;
	} else if ((i0 & 0xf) == 0x9) {
		// s32i.n
		p = (uintptr_t *) get_reg_val(i1 & 0xf) + ((i1 >> 4) * 4);
		*p = get_reg_val(i0 >> 4);
		gdbstub_savedRegs.pc += 2;
	}
}

// We just caught a debug exception and need to handle it. This is called from an assembly
// routine in gdbstub-entry.S
void ATTR_GDBFN gdbstub_handle_debug_exception() {
	wdt_feed();

	if (single_step_ps != -1) {
		// We come here after single-stepping an instruction. Interrupts are disabled
		// for the single step. Re-enable them here.
		gdbstub_savedRegs.ps = (gdbstub_savedRegs.ps & ~0xf) | (single_step_ps & 0xf);
		single_step_ps = -1;
	}

	gdb_send_reason();
	while (gdb_read_command() != ST_CONT);

	if ((gdbstub_savedRegs.expstate & 0x84) == 0x4) {
		// We stopped due to a watchpoint. We can't re-execute the current instruction
		// because it will happily re-trigger the same watchpoint, so we emulate it
		// while we're still in debugger space.
		emulLdSt();
	} else if ((gdbstub_savedRegs.expstate & 0x88) == 0x8) {
		// We stopped due to a BREAK instruction. Skip over it.
		// Check the instruction first; gdb may have replaced it with the original instruction
		// if it's one of the breakpoints it set.
		if (mem_read_byte(gdbstub_savedRegs.pc + 2) == 0
				&& (mem_read_byte(gdbstub_savedRegs.pc + 1) & 0xf0) == 0x40
				&& (mem_read_byte(gdbstub_savedRegs.pc) & 0x0f) == 0x00) {
			gdbstub_savedRegs.pc += 3;
		}
	} else if ((gdbstub_savedRegs.expstate & 0x90) == 0x10) {
		// We stopped due to a BREAK.N instruction. Skip over it, after making sure the instruction
		// actually is a BREAK.N
		if ((mem_read_byte(gdbstub_savedRegs.pc + 1) & 0xf0) == 0xf0
				&& mem_read_byte(gdbstub_savedRegs.pc) == 0x2d) {
			gdbstub_savedRegs.pc += 3;
		}
	}

	//ets_wdt_enable();
}

// Freetos exception. This routine is called by an assembly routine in gdbstub-entry.S
void ATTR_GDBFN gdbstub_handle_user_exception() {
	wdt_feed();

	// mark as an exception reason
	gdbstub_savedRegs.expstate |= 0x80;
	gdb_send_reason();

	while (gdb_read_command() != ST_CONT);

	//ets_wdt_enable();
}

extern void gdbstub_user_exception_entry();
// This will override a weak symbol in esp-open-rtos
void debug_exception_handler();

// TODO: use gdbstub stack for this function too
#if 0
void ATTR_GDBFN gdbstub_handle_uart_int() {
	uint8_t do_debug = 0;
	size_t fifolen = 0;

	fifolen = FIELD2VAL(UART_STATUS_RXFIFO_COUNT, UART(0).STATUS);

	while (fifolen != 0) {
		// Check if any of the chars is control-C. Throw away the rest.
		if (((UART(0).FIFO) & 0xFF) == 0x3) {
			do_debug = 1;
			break;
		}
		fifolen--;
	}

	UART(0).INT_CLEAR |= UART_INT_CLEAR_RXFIFO_FULL | UART_INT_CLEAR_RXFIFO_TIMEOUT;

	// TODO: restore a0, a1 as well in esp-open-rtos
	// TODO: save and restore a14, a15, a16 in esp-open-rtos

	if (do_debug) {
		extern uint32_t debug_saved_ctx;

		uint32_t * isr_stack;
		const size_t isr_stack_reg_offset = 5;
		uint32_t * debug_saved_ctx_p = &debug_saved_ctx;

		__asm volatile (
			"rsr %0, %1"
		: "=r" (gdbstub_savedRegs.pc) : "i" (EPC + XCHAL_INT5_LEVEL));

		gdbstub_savedRegs.a0 = debug_saved_ctx_p[0];
		gdbstub_savedRegs.a1 = debug_saved_ctx_p[1];

		isr_stack = (uint32_t *) (gdbstub_savedRegs.a1 - 0x50);
		gdbstub_savedRegs.ps = isr_stack[2];

		for (size_t x = 2; x < 13; x++) {
			gdbstub_savedRegs.a[x - 2] =
				isr_stack[isr_stack_reg_offset - 2 + x];
		}

		gdbstub_savedRegs.reason = 0xff; // mark as user break reason

		wdt_feed();

		gdb_send_reason();
		while (gdb_read_command() != ST_CONT);

		ets_wdt_enable();

		__asm volatile (
			"wsr %0, %1"
		: "=r" (gdbstub_savedRegs.pc) : "i" (EPC + XCHAL_INT5_LEVEL));

		isr_stack[2] = gdbstub_savedRegs.ps;

		debug_saved_ctx_p[0] = gdbstub_savedRegs.a0;
		debug_saved_ctx_p[1] = gdbstub_savedRegs.a1;

		for (size_t x = 2; x < 13; x++) {
			isr_stack[isr_stack_reg_offset - 2 + x] =
				gdbstub_savedRegs.a[x - 2];
		}
	}
}

static void ATTR_GDBINIT gdbstub_install_uart_handler() {
	_xt_isr_attach(ETS_UART_INUM, gdbstub_handle_uart_int);

	UART(0).INT_ENABLE |= UART_INT_ENABLE_RXFIFO_TIMEOUT | UART_INT_ENABLE_RXFIFO_FULL;

	// enable UART interrupt
	uint32_t intenable;
	__asm volatile (
		"rsr %0, intenable" "\n"
		"or %0, %0, %1" "\n"
		"wsr %0, intenable" "\n"
	:: "r" (intenable), "r" (BIT(ETS_UART_INUM)));
}

static long gdbstub_stdout_write(struct _reent *r, int fd, const char *ptr, int len) {
	gdb_packet_start();
	gdb_packet_char('O');

	for (size_t i = 0; i < len; i++) {
		gdb_packet_hex(ptr[i], 8);
	}

	gdb_packet_end();
	return len;
}
#endif

void ATTR_GDBINIT gdbstub_init() {
	// install stdout wrapper
	//set_write_stdout(gdbstub_stdout_write);

	// install UART interrupt handler
	//gdbstub_install_uart_handler();
}

