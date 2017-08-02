/*
 * gdbstub-freertos.h
 *
 *  Created on: Sep 29, 2016
 *      Author: vlad
 */

#ifndef GDBSTUB_FREERTOS_H_
#define GDBSTUB_FREERTOS_H_

#include <stdlib.h>
#include <stdbool.h>

void fill_task_array();
void gdbstub_freertos_task_list(char *query);
void gdbstub_freertos_task_select(size_t gdb_task_index);
bool gdbstub_freertos_task_selected();
void gdbstub_freertos_regs_read();
void gdbstub_freertos_report_thread();

typedef struct  {
    uint32_t exit;
	uint32_t pc;
	uint32_t ps;
	uint32_t a[16]; //a0..a15
	// These are added manually by the exception code; the HAL doesn't set these on an exception.
    uint32_t sar;
	uint32_t exccause;
    uint32_t excvaddr;

	uint32_t lbeg;
	uint32_t lend;
	uint32_t lcount;

    // coprocessor?
	uint32_t tmp0;
	uint32_t tmp1;
	uint32_t tmp2;

// Floating pont registers???

} xtensa_exception_frame_t;

#pragma pack(push, 4)
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


#pragma pack(pop) 

void gdbstub_freertos_set_reg_data(GdbRegFile *data);


#endif /* GDBSTUB_FREERTOS_H_ */
