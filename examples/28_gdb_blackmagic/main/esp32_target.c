/*
 * This file is part of the Black Magic Debug project.
 *
 * Copyright (C) 2012  Black Sphere Technologies Ltd.
 * Written by Gareth McMullin <gareth@blacksphere.co.nz>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* This file implements debugging functionality specific to ARM
 * the Cortex-M3 core.  This should be generic to ARMv7-M as it is
 * implemented according to the "ARMv7-M Architectue Reference Manual",
 * ARM doc DDI0403C.
 *
 * Also supports Cortex-M0 / ARMv6-M
 */
#include "general.h"
#include "target.h"
#include "target_internal.h"
#include "gdbstub-freertos.h"
#include "gdb_if.h"
#include "rom/ets_sys.h"
#include "freertos/FreeRTOS.h"
#include <xtensa/config/specreg.h>
#include <xtensa/config/core-isa.h>
#include <xtensa/corebits.h>
#include <soc/cpu.h>
#include "gdb_main.h"
#include <freertos/xtensa_api.h>

#include <unistd.h>

typedef struct tskTaskControlBlock {
	volatile portSTACK_TYPE * pxTopOfStack;
} tskTCB;

extern tskTCB * volatile pxCurrentTCB;

static const char esp32_driver_str[] = "esp32";

static bool esp32_vector_catch(target *t, int argc, char *argv[]);
static bool esp32_scan_i2c(target *t, int argc, char *argv[]);

const struct command_s esp32_cmd_list[] = {
	{"vector_catch", (cmd_handler)esp32_vector_catch, "Catch exception vectors"},
	{"scan_i2c", (cmd_handler)esp32_scan_i2c, "Scan i2c test"},
	{NULL, NULL, NULL}
};


static void esp32_regs_read(target *t, void *data);
static void esp32_regs_write(target *t, const void *data);
static uint32_t esp32_pc_read(target *t);

static void esp32_reset(target *t);
static enum target_halt_reason esp32_halt_poll(target *t, target_addr *watch);
static void esp32_halt_request(target *t);
static int esp32_fault_unwind(target *t);

static int esp32_breakwatch_set(target *t, struct breakwatch *);
static int esp32_breakwatch_clear(target *t, struct breakwatch *);
target_addr esp32_check_watch(target *t);
bool esp32_attach(target *t);
void esp32_detach(target *t);
void esp32_halt_resume(target *t, bool step);

GdbRegFile gdbRegFile;


static void dumpHwToRegfile() {
	int i;
	long *frameAregs=pxCurrentTCB->pxTopOfStack;
	gdbRegFile.pc=*(unsigned int *)(pxCurrentTCB->pxTopOfStack+4);
	for (i=0; i<16; i++) gdbRegFile.a[i]=frameAregs[i];
	for (i=16; i<64; i++) gdbRegFile.a[i]=0xDEADBEEF;
	gdbRegFile.a[4]=0x44444444;
	gdbRegFile.a[5]=0x55555555;

	gdbRegFile.a[7]=0x77777777;
	gdbRegFile.a[8]=0x88888888;

	//gdbRegFile.lbeg=0xdeadbeef;  // frame->lbeg;
	//RSR(LBEG, gdbRegFile.lbeg);
	gdbRegFile.lbeg=0x66666666;

	gdbRegFile.lend=0xdeadbeef; //frame->lend;
	RSR(LEND, gdbRegFile.lend);

	gdbRegFile.lcount=0xdeadbeef; //frame->lcount;
	gdbRegFile.sar=0xdeadbeef; //frame->sar;
	//All windows have been spilled to the stack by the ISR routines. The following values should indicate that.
	gdbRegFile.sar=0xdeadbeef; //frame->sar;
	RSR(SAR, gdbRegFile.sar);

	gdbRegFile.windowbase=0; //0
	RSR(WINDOWBASE, gdbRegFile.windowbase);

	gdbRegFile.windowstart=0x1; //1
	RSR(WINDOWSTART, gdbRegFile.windowstart);

	gdbRegFile.configid0=0xdeadbeef; //ToDo
	gdbRegFile.configid1=0xdeadbeef; //ToDo
	gdbRegFile.ps=0xdaadbeef; //frame->ps-PS_EXCM_MASK;
	//RSR(PS, gdbRegFile.ps);

	gdbRegFile.threadptr=0xdeadbeef; //ToDo
	gdbRegFile.br=0xdeadbeef; //ToDo
	gdbRegFile.scompare1=0xdeadbeef; //ToDo
	gdbRegFile.acclo=0xdeadbeef; //ToDo
	gdbRegFile.acchi=0xdeadbeef; //ToDo
	gdbRegFile.m0=0xdeadbeef; //ToDo
	gdbRegFile.m1=0xdeadbeef; //ToDo
	gdbRegFile.m2=0xdeadbeef; //ToDo
	gdbRegFile.m3=0xdeadbeef; //ToDo
	gdbRegFile.expstate=0; //frame->exccause; //ToDo
}


//Send the reason execution is stopped to GDB.
static void sendReason() {
	//exception-to-signal mapping
	char exceptionSignal[]={4,31,11,11,2,6,8,0,6,7,0,0,7,7,7,7};
	int i=0;
	gdbPacketStart();
	gdbPacketChar('T');
	i=gdbRegFile.expstate&0x7f;
	if (i<sizeof(exceptionSignal)) {
		gdbPacketHex(exceptionSignal[i], 8); 
	} else {
		gdbPacketHex(11, 8);
	}
	gdbPacketEnd();
}


#define ESP32_MAX_WATCHPOINTS	2	
#define ESP32_MAX_BREAKPOINTS	2	

static int esp32_hostio_request(target *t);

struct esp32_priv {
	bool stepping;
	bool on_bkpt;
	/* Watchpoint unit status */
	bool hw_watchpoint[ESP32_MAX_WATCHPOINTS];
	unsigned hw_watchpoint_max;
	/* Breakpoint unit status */
	bool hw_breakpoint[ESP32_MAX_BREAKPOINTS];
	unsigned hw_breakpoint_max;
	/* Copy of DEMCR for vector-catch */
	uint32_t demcr;
};



static const char tdesc_esp32[] =
	"<?xml version=\"1.0\"?>"
	"<!DOCTYPE target SYSTEM \"gdb-target.dtd\">"
	"<target>"
	"  <architecture>xtensa</architecture>"
	"  <feature name=\"org.gnu.gdb.xtensa.m-profile\">"
	"    <reg name=\"pc\" bitsize=\"32\"/>"
	"    <reg name=\"ar0\" bitsize=\"32\"/>"
    "    <reg name=\"ar1\" bitsize=\"32\"/>"
    "    <reg name=\"ar2\" bitsize=\"32\"/>"
    "    <reg name=\"ar3\" bitsize=\"32\"/>"
    "    <reg name=\"ar4\" bitsize=\"32\"/>"
	"    <reg name=\"ar5\" bitsize=\"32\"/>"
    "    <reg name=\"ar6\" bitsize=\"32\"/>"
    "    <reg name=\"ar7\" bitsize=\"32\"/>"
    "    <reg name=\"ar8\" bitsize=\"32\"/>"
    "    <reg name=\"ar9\" bitsize=\"32\"/>"
	"    <reg name=\"ar10\" bitsize=\"32\"/>"
    "    <reg name=\"ar11\" bitsize=\"32\"/>"
    "    <reg name=\"ar12\" bitsize=\"32\"/>"
    "    <reg name=\"ar13\" bitsize=\"32\"/>"
    "    <reg name=\"ar14\" bitsize=\"32\"/>"
	"    <reg name=\"ar15\" bitsize=\"32\"/>"
    "    <reg name=\"ar16\" bitsize=\"32\"/>"
    "    <reg name=\"ar17\" bitsize=\"32\"/>"
    "    <reg name=\"ar18\" bitsize=\"32\"/>"
    "    <reg name=\"ar19\" bitsize=\"32\"/>"
	"    <reg name=\"ar20\" bitsize=\"32\"/>"
    "    <reg name=\"ar21\" bitsize=\"32\"/>"
    "    <reg name=\"ar22\" bitsize=\"32\"/>"
    "    <reg name=\"ar23\" bitsize=\"32\"/>"
    "    <reg name=\"ar24\" bitsize=\"32\"/>"
	"    <reg name=\"ar25\" bitsize=\"32\"/>"
    "    <reg name=\"ar26\" bitsize=\"32\"/>"
    "    <reg name=\"ar27\" bitsize=\"32\"/>"
    "    <reg name=\"ar28\" bitsize=\"32\"/>"
    "    <reg name=\"ar29\" bitsize=\"32\"/>"
	"    <reg name=\"ar30\" bitsize=\"32\"/>"
    "    <reg name=\"ar31\" bitsize=\"32\"/>"
    "    <reg name=\"ar32\" bitsize=\"32\"/>"
    "    <reg name=\"ar33\" bitsize=\"32\"/>"
    "    <reg name=\"ar34\" bitsize=\"32\"/>"
	"    <reg name=\"ar35\" bitsize=\"32\"/>"
    "    <reg name=\"ar36\" bitsize=\"32\"/>"
    "    <reg name=\"ar37\" bitsize=\"32\"/>"
    "    <reg name=\"ar38\" bitsize=\"32\"/>"
    "    <reg name=\"ar39\" bitsize=\"32\"/>"
	"    <reg name=\"ar40\" bitsize=\"32\"/>"
    "    <reg name=\"ar41\" bitsize=\"32\"/>"
    "    <reg name=\"ar42\" bitsize=\"32\"/>"
    "    <reg name=\"ar43\" bitsize=\"32\"/>"
    "    <reg name=\"ar44\" bitsize=\"32\"/>"
	"    <reg name=\"ar45\" bitsize=\"32\"/>"
    "    <reg name=\"ar46\" bitsize=\"32\"/>"
    "    <reg name=\"ar47\" bitsize=\"32\"/>"
    "    <reg name=\"ar48\" bitsize=\"32\"/>"
    "    <reg name=\"ar49\" bitsize=\"32\"/>"
	"    <reg name=\"ar50\" bitsize=\"32\"/>"
    "    <reg name=\"ar51\" bitsize=\"32\"/>"
    "    <reg name=\"ar52\" bitsize=\"32\"/>"
    "    <reg name=\"ar53\" bitsize=\"32\"/>"
    "    <reg name=\"ar54\" bitsize=\"32\"/>"
	"    <reg name=\"ar55\" bitsize=\"32\"/>"
    "    <reg name=\"ar56\" bitsize=\"32\"/>"
    "    <reg name=\"ar57\" bitsize=\"32\"/>"
    "    <reg name=\"ar58\" bitsize=\"32\"/>"
    "    <reg name=\"ar59\" bitsize=\"32\"/>"
	"    <reg name=\"ar60\" bitsize=\"32\"/>"
    "    <reg name=\"ar61\" bitsize=\"32\"/>"
    "    <reg name=\"ar62\" bitsize=\"32\"/>"
    "    <reg name=\"ar63\" bitsize=\"32\"/>"
	"    <reg name=\"lbeg\" bitsize=\"32\"/>"
	"    <reg name=\"lend\" bitsize=\"32\"/>"
	"    <reg name=\"lcount\" bitsize=\"32\"/>"
	"    <reg name=\"sar\" bitsize=\"32\"/>"
	"    <reg name=\"windowbase\" bitsize=\"32\"/>"
	"    <reg name=\"windowstart\" bitsize=\"32\"/>"
	"    <reg name=\"configid0\" bitsize=\"32\"/>"
	"    <reg name=\"configid1\" bitsize=\"32\"/>"
	"    <reg name=\"ps\" bitsize=\"32\"/>"
	"    <reg name=\"threadptr\" bitsize=\"32\"/>"
	"    <reg name=\"br\" bitsize=\"32\"/>"
	"    <reg name=\"scompare1\" bitsize=\"32\"/>"
	"    <reg name=\"acclo\" bitsize=\"32\"/>"
	"    <reg name=\"acchi\" bitsize=\"32\"/>"
	"    <reg name=\"m0\" bitsize=\"32\"/>"
	"    <reg name=\"m1\" bitsize=\"32\"/>"
	"    <reg name=\"m2\" bitsize=\"32\"/>"
	"    <reg name=\"m3\" bitsize=\"32\"/>"
	"    <reg name=\"expstate\" bitsize=\"32\"/>"
	"    <reg name=\"f64r_lo\" bitsize=\"32\"/>"
	"    <reg name=\"f64r_hi\" bitsize=\"32\"/>"
	"    <reg name=\"f64s\" bitsize=\"32\"/>"
	"    <reg name=\"f0\" bitsize=\"32\"/>"
	"    <reg name=\"f1\" bitsize=\"32\"/>"
	"    <reg name=\"f2\" bitsize=\"32\"/>"
	"    <reg name=\"f3\" bitsize=\"32\"/>"
	"    <reg name=\"f4\" bitsize=\"32\"/>"
	"    <reg name=\"f5\" bitsize=\"32\"/>"
	"    <reg name=\"f6\" bitsize=\"32\"/>"
	"    <reg name=\"f7\" bitsize=\"32\"/>"
	"    <reg name=\"f8\" bitsize=\"32\"/>"
	"    <reg name=\"f0\" bitsize=\"32\"/>"
	"    <reg name=\"f10\" bitsize=\"32\"/>"
	"    <reg name=\"f11\" bitsize=\"32\"/>"
	"    <reg name=\"f12\" bitsize=\"32\"/>"
	"    <reg name=\"f13\" bitsize=\"32\"/>"
	"    <reg name=\"f14\" bitsize=\"32\"/>"
	"    <reg name=\"f15\" bitsize=\"32\"/>"
	"    <reg name=\"fcr\" bitsize=\"32\"/>"
	"    <reg name=\"fsr\" bitsize=\"32\"/>"
	"  </feature>"
	"</target>";


static void esp32_mem_read(target *t, void *dest, target_addr src, size_t len)
{
	printf("esp32_mem_read %d\n",len);
	int * i = (int *) (src & (~3));

	// TODO: better address range check?
	if (src < 0x20000000 || src >= 0x60000000) {
		return;
	}

	int *to=(int *)dest;
	 *to= * i >> ((src & 3) * 8);
}

static void esp32_mem_write(target *t, target_addr dest, const void *src, size_t len)
{
	printf("esp32_mem_write %d\n",len);
}

static bool esp32_check_error(target *t)
{
	return false;
}

static void esp32_priv_free(void *priv)
{
	free(priv);
}

target *esp32_probe(struct target_controller *controller)
{
	target *t;

	t = target_new();
	struct esp32_priv *priv = calloc(1, sizeof(*priv));
	t->priv = priv;
	t->priv_free = esp32_priv_free;

	t->check_error = esp32_check_error;
	t->mem_read = esp32_mem_read;
	t->mem_write = esp32_mem_write;

	t->driver = esp32_driver_str;

	t->attach = esp32_attach;
	t->detach = esp32_detach;

	/* Should probe here to make sure it's Cortex-M3 */
	t->tdesc = tdesc_esp32;
	t->regs_read = esp32_regs_read;
	t->regs_write = esp32_regs_write;

	t->reset = esp32_reset;
	t->halt_request = esp32_halt_request;
	t->halt_poll = esp32_halt_poll;
	t->halt_resume = esp32_halt_resume;
	t->regs_size = sizeof(GdbRegFile);

	t->breakwatch_set = esp32_breakwatch_set;
	t->breakwatch_clear = esp32_breakwatch_clear;

	target_add_commands(t, esp32_cmd_list, esp32_driver_str);

    target_check_error(t);


    target_attach(t,controller);

	return t;
}

bool esp32_attach(target *t)
{
	struct esp32_priv *priv = t->priv;
	unsigned i;
	uint32_t r;
	int tries;

	/* Clear any pending fault condition */
	target_check_error(t);

	target_halt_request(t);
	tries = 10;
	//while(!platform_srst_get_val() && !target_halt_poll(t, NULL) && --tries)
	//	platform_delay(200);
	//if(!tries)
	//	return false;


	/* size the break/watchpoint units */
	priv->hw_breakpoint_max = ESP32_MAX_BREAKPOINTS;
	priv->hw_watchpoint_max = ESP32_MAX_WATCHPOINTS;


    dumpHwToRegfile();

	return true;
}

void esp32_detach(target *t)
{
	printf("esp32_detach\n");
	gdb_if_close();
}

enum { DB_DHCSR, DB_DCRSR, DB_DCRDR, DB_DEMCR };

static void esp32_regs_read(target *t, void *data)
{
	printf("esp32_regs_read\n");

    dumpHwToRegfile();
	if (gdbstub_freertos_task_selected()) {
	  printf("task\n");
  	  gdbstub_freertos_set_reg_data(&gdbRegFile);
	}
	//gdbRegFile;
	if (gdbRegFile.pc & 0x80000000) {
        gdbRegFile.pc = (gdbRegFile.pc & 0x3fffffff) | 0x40000000;
    }
	int *p=(int*)&gdbRegFile.pc;
	memcpy(p,data,sizeof(gdbRegFile));
}

static void esp32_regs_write(target *t, const void *data)
{
		printf("esp32_regs_write\n");
		int *p=(int*)&gdbRegFile;
		memcpy(data,p,sizeof(gdbRegFile));
}

static uint32_t esp32_pc_read(target *t)
{
	printf("esp32_pc_read\n");

	return 0;
}

static void esp32_pc_write(target *t, const uint32_t val)
{
		printf("esp32_pc_write\n");

}

/* The following three routines implement target halt/resume
 * using the core debug registers in the NVIC. */
static void esp32_reset(target *t)
{
   printf("esp32_reset\n");
}

static void esp32_halt_request(target *t)
{
	  printf("esp32_halt_request\n");

	  
}

static enum target_halt_reason esp32_halt_poll(target *t, target_addr *watch)
{
	printf("esp32_halt_poll\n");

	sendReason();

	return TARGET_HALT_BREAKPOINT;
}

void esp32_halt_resume(target *t, bool step)
{
	printf("esp32_halt_resume\n");
}

static int esp32_fault_unwind(target *t)
{
	printf("esp32_fault_unwind\n");	
	return 0;
}

int esp32_run_stub(target *t, uint32_t loadaddr,
                     uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3)
{
	printf("esp32_run_stub\n");	

	return 0;
}

/* The following routines implement hardware breakpoints and watchpoints.
 * The Flash Patch and Breakpoint (FPB) and Data Watch and Trace (DWT)
 * systems are used. */

static uint32_t dwt_mask(size_t len)
{
	printf("esp32_dwt_mask\n");	
#if 0

   // This disables all the watchdogs 

	TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
    TIMERG0.wdt_config0.en = 0;
    TIMERG0.wdt_wprotect = 0;
    TIMERG1.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
    TIMERG1.wdt_config0.en = 0;
    TIMERG1.wdt_wprotect = 0;
#endif
	return 0;
}

static uint32_t dwt_func(target *t, enum target_breakwatch type)
{
	printf("esp32_dwt_func\n");	
	return 0;
}

static void setFirstBreakpoint(uint32_t pc)
{
    asm(
        "wsr.ibreaka0 %0\n" \
        "rsr.ibreakenable a3\n" \
        "movi a4,1\n" \
        "or a4, a4, a3\n" \
        "wsr.ibreakenable a4\n" \
        ::"r"(pc):"a3", "a4");
}

#define ESP_OK 0
#define ESP_FAIL        -1
#define ESP_ERR_NO_MEM          0x101
#define ESP_ERR_INVALID_ARG     0x102

int esp_original_set_watchpoint(int no, void *adr, int size, int flags)
{
    int x;
    if (no<0 || no>1) return ESP_ERR_INVALID_ARG;
    if (flags&(~0xC0000000)) return ESP_ERR_INVALID_ARG;
    int dbreakc=0x3F;
    //We support watching 2^n byte values, from 1 to 64. Calculate the mask for that.
    for (x=0; x<7; x++) {
        if (size==(1<<x)) break;
        dbreakc<<=1;
    }
    if (x==7) return ESP_ERR_INVALID_ARG;
    //Mask mask and add in flags.
    dbreakc=(dbreakc&0x3f)|flags;

    if (no==0) {
        asm volatile(
            "wsr.dbreaka0 %0\n" \
            "wsr.dbreakc0 %1\n" \
            ::"r"(adr),"r"(dbreakc));
    } else {
        asm volatile(
            "wsr.dbreaka1 %0\n" \
            "wsr.dbreakc1 %1\n" \
            ::"r"(adr),"r"(dbreakc));
    }
    return ESP_OK;
}

void esp_original_clear_watchpoint(int no)
{
    //Setting a dbreakc register to 0 makes it trigger on neither load nor store, effectively disabling it.
    int dbreakc=0;
    if (no==0) {
        asm volatile(
            "wsr.dbreakc0 %0\n" \
            ::"r"(dbreakc));
    } else {
        asm volatile(
            "wsr.dbreakc1 %0\n" \
            ::"r"(dbreakc));
    }
}


static int esp32_breakwatch_set(target *t, struct breakwatch *bw)
{
	struct esp32_priv *priv = t->priv;
	unsigned i;
	uint32_t val = bw->addr;

	printf("esp32_breakwatch_set\n");	


	switch (bw->type) {
	case TARGET_BREAK_HARD:
 	    setFirstBreakpoint(bw->addr);
		return 0;

	case TARGET_WATCH_WRITE:
	case TARGET_WATCH_READ:
	case TARGET_WATCH_ACCESS:
		return 0;
	default:
		return 1;
	}
}

static int esp32_breakwatch_clear(target *t, struct breakwatch *bw)
{
	printf("esp32_breakwatch_clear\n");	

	struct esp32_priv *priv = t->priv;
	unsigned i = bw->reserved[0];
	switch (bw->type) {
	case TARGET_BREAK_HARD:
		priv->hw_breakpoint[i] = false;
		return 0;
	case TARGET_WATCH_WRITE:
	case TARGET_WATCH_READ:
	case TARGET_WATCH_ACCESS:
		priv->hw_watchpoint[i] = false;
		return 0;
	default:
		return 1;
	}
}




target_addr esp32_check_watch(target *t)
{

	printf("esp32_breakwatch_clear\n");	

	return 0;
}

static void dumpExceptionToRegfile(XtExcFrame *frame) {
	int i;
	long *frameAregs=&frame->a0;
	gdbRegFile.pc=frame->pc;
	for (i=0; i<16; i++) gdbRegFile.a[i]=frameAregs[i];
	for (i=16; i<64; i++) gdbRegFile.a[i]=0xDEADBEEF;
	gdbRegFile.lbeg=frame->lbeg;
	gdbRegFile.lend=frame->lend;
	gdbRegFile.lcount=frame->lcount;
	gdbRegFile.sar=frame->sar;
	//All windows have been spilled to the stack by the ISR routines. The following values should indicate that.
	gdbRegFile.sar=frame->sar;
	gdbRegFile.windowbase=0; //0
	gdbRegFile.windowstart=0x1; //1
	gdbRegFile.configid0=0xdeadbeef; //ToDo
	gdbRegFile.configid1=0xdeadbeef; //ToDo
	gdbRegFile.ps=frame->ps-PS_EXCM_MASK;
	gdbRegFile.threadptr=0xdeadbeef; //ToDo
	gdbRegFile.br=0xdeadbeef; //ToDo
	gdbRegFile.scompare1=0xdeadbeef; //ToDo
	gdbRegFile.acclo=0xdeadbeef; //ToDo
	gdbRegFile.acchi=0xdeadbeef; //ToDo
	gdbRegFile.m0=0xdeadbeef; //ToDo
	gdbRegFile.m1=0xdeadbeef; //ToDo
	gdbRegFile.m2=0xdeadbeef; //ToDo
	gdbRegFile.m3=0xdeadbeef; //ToDo
	gdbRegFile.expstate=frame->exccause; //ToDo
}


// This will probably not work, I assume processes switchin will stop on exception 
void wifi_panic_handler(XtExcFrame *frame) {
	dumpExceptionToRegfile(frame);
	sendReason();
	while(1) {
		gdb_main();
	}
}

void no_panic_handler(XtExcFrame *frame) {
	printf("DONT PANIC\n");
	// This does not work, we must also clear PS.EX...
	// The idea was to be able to catch memory reads from faulty adresses

    //asm volatile("movi    a0, PS_INTLEVEL(5) | PS_UM | PS_WOE");
	//asm volatile("movi    a0,0x60020");
	 // Illegal instruction
    //asm volatile("wsr     a0, PS");

	//asm volatile("rsr     a0, ps");
	//asm volatile("rsr     a0, excsave1");
	//asm volatile("addi.n	a0, a0, 3");
	//asm volatile("rfe");
}

void set_exception_handler(int i) {
       xt_set_exception_handler(i, no_panic_handler);	
}


void set_all_exception_handlers(void) {
	printf("set_all_exception_handlers\n");	

	int i=0;
	for (i=0;i<64;i++) {
       xt_set_exception_handler(i, wifi_panic_handler);
	}
}



static bool esp32_vector_catch(target *t, int argc, char *argv[])
{
	struct esp32_priv *priv = t->priv;
	const char *vectors[] = {"IllegalInstruction", "Syscall", "InstructionFetchError", "LoadStoreError",
    "Level1Interrupt", "Alloca", "IntegerDivideByZero", "PCValue",
    "Privileged", "LoadStoreAlignment", "res", "res",
    "InstrPDAddrError", "LoadStorePIFDataError", "InstrPIFAddrError", "LoadStorePIFAddrError",
    "InstTLBMiss", "InstTLBMultiHit", "InstFetchPrivilege", "res",
    "InstrFetchProhibited", "res", "res", "res",
    "LoadStoreTLBMiss", "LoadStoreTLBMultihit", "LoadStorePrivilege", "res",
    "LoadProhibited", "StoreProhibited", "res", "res",
    "Cp0Dis", "Cp1Dis", "Cp2Dis", "Cp3Dis",
    "Cp4Dis", "Cp5Dis", "Cp6Dis", "Cp7Dis"};

	uint32_t tmp = 0;
	unsigned i;

    //  || ((argv[1][0] != 'e') && (argv[1][0] != 'd'))
	if ((argc < 2)) {
		tc_printf(t, "usage: monitor vector_catch"
			     "(all|");

			for (i = 0; i < sizeof(vectors) / sizeof(char*); i++) {
				tc_printf(t,"|%s",vectors[i]);
			}

			tc_printf(t,")\n");
	} else {
		for (int j = 0; j < argc; j++) {
			if (!strcmp("all", argv[j])) {
			     set_all_exception_handlers();
		    }

			for (i = 0; i < sizeof(vectors) / sizeof(char*); i++) {
				if (vectors[i] && !strcmp(vectors[i], argv[j]))
				{
					set_exception_handler(i);
				}	
			}
		}
	}

	tc_printf(t, "Catching vectors: ");
	for (i = 0; i < sizeof(vectors) / sizeof(char*); i++) {
		if (!vectors[i])
			continue;
		if (priv->demcr & (1 << i))
			tc_printf(t, "%s ", vectors[i]);
	}
	tc_printf(t, "\n");
	return true;
}

static bool esp32_scan_i2c(target *t, int argc, char *argv[])
{
	struct esp32_priv *priv = t->priv;
	const char *vectors[] = {"reset", NULL, NULL, NULL, "mm", "nocp",
				"chk", "stat", "bus", "int", "hard"};
	uint32_t tmp = 0;
	unsigned i;

	if ((argc < 2) || ((argv[1][0] != 'e') && (argv[1][0] != 'd'))) {
		tc_printf(t, "usage: monitor scan_i2c (null|div) "
			     "null gives you a StoreProhibited exception \n");
	} else {
		// ATTACH
	}

	tc_printf(t, "Start scanning  i2c: ");
	for (int j = 0; j < argc; j++) {
		for (i = 0; i < sizeof(vectors) / sizeof(char*); i++) {
			if (!strcmp("null", argv[j])) {
				int *ptr=0;
				*ptr=0xff;
			}
			if (!strcmp("div", argv[j])) {
				int test=0;
				int kalle=12;
				int nisse;
				nisse=kalle/test;
			}
		}
    }

	tc_printf(t, "\n");
	return true;
}



/* Windows defines this with some other meaning... */
#ifdef SYS_OPEN
#	undef SYS_OPEN
#endif

static int esp32_hostio_request(target *t)
{
	return 0;
}

