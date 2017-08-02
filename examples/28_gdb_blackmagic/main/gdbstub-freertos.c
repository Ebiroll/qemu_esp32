/*
 * gdbstub-freertos.c
 *
 *  Created on: Sep 29, 2016
 *      Author: vlad
 */

//#include "gdbstub.h"
#include "gdbstub-cfg.h"
#include "gdbstub-internal.h"

#include <sys/types.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "gdbstub-freertos.h"

#include <stdint.h>
#include <stdio.h>

static size_t task_count = 0;
static size_t task_selected = 0;
static int task_array_filled=0;

static struct {
	uint32_t * stack;
	TaskHandle_t handle;
	xtensa_exception_frame_t* test;
} task_list[GDBSTUB_THREADS_MAX] = {{ 0 }};

/*
 * tskTCB is private, so, unfortunately, we have
 * to redefine it here in order to save some memory
 * by not calling uxTaskGetSystemState()
 *
 * Shouldn't be a big problem since pxTopOfStack
 * is always first.
 */
typedef struct tskTaskControlBlock {
	volatile portSTACK_TYPE * pxTopOfStack;
} tskTCB;

static char gdbstub_packet_crc;			// Checksum of the output packet


/*
 * Private FreeRTOS stuff shared using portREMOVE_STATIC_QUALIFIER
 */
extern List_t * volatile pxDelayedTaskList;
extern List_t * volatile pxOverflowDelayedTaskList;
extern List_t pxReadyTasksLists[configMAX_PRIORITIES];

#if INCLUDE_vTaskDelete
extern List_t xTasksWaitingTermination;
#endif

#if INCLUDE_vTaskSuspend
extern List_t xSuspendedTaskList;
#endif

extern tskTCB * volatile pxCurrentTCB;

char thread_entry[250] = { 0 };

void ATTR_GDBFN gdbSendChar(char c);

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
		gdbSendChar(hexChars[(val >> (i - 4)) & 0xf]);
	}
}

void gdbPacketFlush();

// Finish sending a packet.
void gdb_packet_end() {
	gdbSendChar('#');
	gdb_packet_hex(gdbstub_packet_crc, 8);
	gdbPacketFlush();
}

static void putEntry(uint32_t pc, uint32_t sp)
{
    if (pc & 0x80000000) {
        pc = (pc & 0x3fffffff) | 0x40000000;
    }
    printf(" 0x%X",pc);
    printf(":0x%X",sp);
}

bool esp_stack_ptr_is_sane(uint32_t sp)
{
	return !(sp < 0x3ffae010UL || sp > 0x3ffffff0UL || ((sp & 0xf) != 0));
}

static void doBacktrace(uint32_t s_pc,uint32_t s_a0,uint32_t s_a1)
{
    uint32_t i = 0, pc = s_pc, sp =s_a1;
    printf("\r\nBacktrace:");
    /* Do not check sanity on first entry, PC could be smashed. */
    putEntry(pc, sp);
    pc = s_a0;
    while (i++ < 100) {
        uint32_t psp = sp;
        if (!esp_stack_ptr_is_sane(sp) || i++ > 100) {
            break;
        }
        sp = *((uint32_t *) (sp - 0x10 + 4));
        putEntry(pc - 3, sp); // stack frame addresses are return addresses, so subtract 3 to get the CALL address
        pc = *((uint32_t *) (psp - 0x10));
        if (pc < 0x40000000) {
            break;
        }
    }
    printf("\r\n\r\n");
}


static void gdbstub_send_task(size_t id, char * name) {

	const char * thread_template = "<thread id=\"%x\" core=\"0\" name=\"%s\">%s</thread>";
	thread_entry[0]=0;

	if (name) {
	  snprintf(thread_entry, sizeof(thread_entry), thread_template, id, name,name);
	  gdb_packet_str(thread_entry);
	}
}

static void process_task_list(List_t * list) {
	volatile tskTCB * next_tcb, * first_tcb;

	if (list->uxNumberOfItems > 0) {
		listGET_OWNER_OF_NEXT_ENTRY(first_tcb, list);

		do {
			if (task_count > GDBSTUB_THREADS_MAX) {
				break;
			}

			listGET_OWNER_OF_NEXT_ENTRY(next_tcb, list);

			task_list[task_count].stack = (uint32_t *) next_tcb->pxTopOfStack;
			task_list[task_count].test = (xtensa_exception_frame_t *) next_tcb->pxTopOfStack;
			task_list[task_count].handle = (TaskHandle_t) next_tcb;

			task_count++;
		} while (next_tcb != first_tcb);
	}
}

void fill_task_array() {
	int32_t queue = configMAX_PRIORITIES;

	task_count = 0;

	do {
		queue--;
		process_task_list(&pxReadyTasksLists[queue]);
	} while (queue > tskIDLE_PRIORITY);

	process_task_list((List_t *) pxDelayedTaskList);
	process_task_list((List_t *) pxOverflowDelayedTaskList);

#if INCLUDE_vTaskDelete
	process_task_list(&xTasksWaitingTermination);
#endif

#if INCLUDE_vTaskSuspend
	process_task_list(&xSuspendedTaskList);
#endif

  task_array_filled=1;
}



void gdbstub_freertos_task_list(char *query) {
	if (task_array_filled==0) {
		fill_task_array();
	}


	gdb_packet_start();
	gdb_packet_str("l");
	gdb_packet_str("<?xml version=\"1.0\" ?>");
	gdbPacketFlush();
	gdb_packet_str("<threads>");
	gdbPacketFlush();

	/*
	 * It is assumed that the task array doesn't need
	 * to be updated because it already was when the
	 * stop reason was reported.
	 *
	 * Thread ID 0 has a special meaning in the remote
	 * protocol and so is avoided.
	 */


	for (size_t i = 0; i < task_count - 1; i++) {
		gdbstub_send_task(i + 1, (char *) pcTaskGetTaskName(task_list[i].handle));
		gdbPacketFlush();
	}

	gdb_packet_str("</threads>");
	gdb_packet_end();
}

void gdbstub_freertos_task_select(size_t gdb_task_index) {
	task_selected = gdb_task_index - 1;
}

bool gdbstub_freertos_task_selected() {
	if (task_selected > task_count - 1) {
		return false;
	}

	return task_list[task_selected].handle == (TaskHandle_t) pxCurrentTCB;
}

void gdbstub_freertos_regs_read() {
	if (task_array_filled==0) {
		fill_task_array();
	}

    printf("**********************************\n");
/*
	uint32_t pc;
	uint32_t ps;
	uint32_t sar;
	uint32_t vpri;
	uint32_t a0;
	uint32_t a[14]; //a2..a15
	// These are added manually by the exception code; the HAL doesn't set these on an exception.
	uint32_t exccause;
*/

	// task_selected was checked before
	gdb_packet_start();

	uint32_t * task_regs = task_list[task_selected].stack;

	// pc
	gdb_packet_hex(bswap32(task_regs[1]), 32);

	for (size_t i = 4; i <= 18; i++) {
		gdb_packet_hex(bswap32(task_regs[i]), 32);
	}

	// Skip special registers
	gdb_packet_hex(0, 32);
	gdb_packet_hex(0, 32);
	gdb_packet_hex(0, 32);
	gdb_packet_hex(0, 32);

	// ps
	gdb_packet_hex(task_regs[2], 32);

	gdb_packet_end();
}

void gdbstub_freertos_set_reg_data(GdbRegFile *data) {
	if (task_array_filled==0) {
		fill_task_array();
	}

    printf("**********************************\n");
/*
    uint32_t exit;
	uint32_t pc;
	uint32_t ps;
	uint32_t a[16]; //a0..a15
	uint32_t sar;
	uint32_t exccause;
    uint32_t excvaddr;

*/

	// task_selected was checked before

	uint32_t * task_regs = task_list[task_selected].stack;

	// pc
	data->pc=task_regs[1];
	data->ps=task_regs[2];
	data->a[0]=task_regs[3];

	for (size_t i = 4; i < 16+4; i++) {
		//gdb_packet_hex(bswap32(task_regs[i]), 32);
		data->a[i-3]=task_regs[i];
	}

    data->sar=task_regs[20];
	data->expstate=task_regs[21];

	doBacktrace(data->pc,data->a[0],data->a[1]);

}


void gdbstub_freertos_report_thread() {
	fill_task_array();

	size_t task_index = 0;

	for (size_t i = 0; i < task_count - 1; i++) {
		if (task_list[i].handle == (TaskHandle_t) pxCurrentTCB) {
			task_index = i;
			break;
		}
	}

	gdb_packet_str("thread:");
	gdb_packet_hex(task_index + 1, 8);
	gdb_packet_str(";");
}
