#ifndef GDBSTUB_CFG_H
#define GDBSTUB_CFG_H

/*
 * Enable thread support. gdbstub will be able to list FreeRTOS tasks as threads
 * at the expense of code size.
 *
 * By default, ESP8266 SDK starts a lot of tasks and transferring all their
 * data through UART will take some time, so there is a performance impact too.
 *
 * This option is set in the premake script.
 */
#ifndef GDBSTUB_THREAD_AWARE
#define GDBSTUB_THREAD_AWARE 1
#endif

/*
 * Max number of tasks gdbstub will be able to handle. Each task descriptor
 * takes 8 bytes of memory. The task descriptor array is allocated
 * statically.
 */
#ifndef GDBSTUB_THREADS_MAX
#define GDBSTUB_THREADS_MAX 10
#endif

#define ATTR_GDBINIT
#ifndef ATTR_GDBFN
#define ATTR_GDBFN		
#endif

#endif

