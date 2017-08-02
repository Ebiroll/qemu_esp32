#ifndef GDBSTUB_H
#define GDBSTUB_H

#ifdef __cplusplus
extern "C" {
#endif

void gdbstub_init();
void gdbstub_do_break();

#define gdbstub_do_break() { __asm volatile ("break 0,0"); }

#ifdef __cplusplus
}
#endif

#endif

