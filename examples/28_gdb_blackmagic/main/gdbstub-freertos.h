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

void gdbstub_freertos_task_list();
void gdbstub_freertos_task_select(size_t gdb_task_index);
bool gdbstub_freertos_task_selected();
void gdbstub_freertos_regs_read();
void gdbstub_freertos_report_thread();

#endif /* GDBSTUB_FREERTOS_H_ */
