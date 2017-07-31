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

#include <unistd.h>

static const char esp32_driver_str[] = "esp32";

static bool esp32_vector_catch(target *t, int argc, char *argv[]);

const struct command_s esp32_cmd_list[] = {
	{"vector_catch", (cmd_handler)esp32_vector_catch, "Catch exception vectors"},
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


struct xtensa_exception_frame_t {
	uint32_t pc;
	uint32_t ps;
	uint32_t sar;
	uint32_t vpri;
	uint32_t a0;
	uint32_t a[14]; //a2..a15
	// These are added manually by the exception code; the HAL doesn't set these on an exception.
	uint32_t exccause;

//  HAVE_LOOPS
	//uint32_t lcount;
	//uint32_t lbeg;
	//uint32_t lend;

// Floating pont registers???

}

typedef struct  {
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
} GdbRegFile;

GdbRegFile gdbRegFile;


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
#if 0
	int * i = (int *) (p & (~3));

	// TODO: better address range check?
	if (p < 0x20000000 || p >= 0x60000000) {
		return -1;
	}

	return * i >> ((p & 3) * 8);
#endif
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

bool esp32_probe(struct target_controller *controller)
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
	t->regs_size = 104*4;

	t->breakwatch_set = esp32_breakwatch_set;
	t->breakwatch_clear = esp32_breakwatch_clear;

	target_add_commands(t, esp32_cmd_list, esp32_driver_str);

    target_check_error(t);


    target_attach(t,controller);

	return true;
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



	return true;
}

void esp32_detach(target *t)
{
}

enum { DB_DHCSR, DB_DCRSR, DB_DCRDR, DB_DEMCR };

static void esp32_regs_read(target *t, void *data)
{
	printf("esp32_regs_read\n");

}

static void esp32_regs_write(target *t, const void *data)
{
		printf("esp32_regs_write\n");
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
}

static enum target_halt_reason esp32_halt_poll(target *t, target_addr *watch)
{
	return TARGET_HALT_BREAKPOINT;
}

void esp32_halt_resume(target *t, bool step)
{
}

static int esp32_fault_unwind(target *t)
{
	return 0;
}

int esp32_run_stub(target *t, uint32_t loadaddr,
                     uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3)
{

	return 0;
}

/* The following routines implement hardware breakpoints and watchpoints.
 * The Flash Patch and Breakpoint (FPB) and Data Watch and Trace (DWT)
 * systems are used. */

static uint32_t dwt_mask(size_t len)
{
	return 0;
}

static uint32_t dwt_func(target *t, enum target_breakwatch type)
{
	return 0;
}

static int esp32_breakwatch_set(target *t, struct breakwatch *bw)
{
	struct esp32_priv *priv = t->priv;
	unsigned i;
	uint32_t val = bw->addr;

	switch (bw->type) {
	case TARGET_BREAK_HARD:
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

	return 0;
}

static bool esp32_vector_catch(target *t, int argc, char *argv[])
{
	struct esp32_priv *priv = t->priv;
	const char *vectors[] = {"reset", NULL, NULL, NULL, "mm", "nocp",
				"chk", "stat", "bus", "int", "hard"};
	uint32_t tmp = 0;
	unsigned i;

	if ((argc < 3) || ((argv[1][0] != 'e') && (argv[1][0] != 'd'))) {
		tc_printf(t, "usage: monitor vector_catch (enable|disable) "
			     "(hard|int|bus|stat|chk|nocp|mm|reset)\n");
	} else {
		// ATTACH
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

/* Windows defines this with some other meaning... */
#ifdef SYS_OPEN
#	undef SYS_OPEN
#endif

static int esp32_hostio_request(target *t)
{
	return 0;
}

