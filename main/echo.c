/*
 * Copyright (c) 2007-2009 Xilinx, Inc.  All rights reserved.
 *
 * Xilinx, Inc.
 * XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS" AS A
 * COURTESY TO YOU.  BY PROVIDING THIS DESIGN, CODE, OR INFORMATION AS
 * ONE POSSIBLE   IMPLEMENTATION OF THIS FEATURE, APPLICATION OR
 * STANDARD, XILINX IS MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION
 * IS FREE FROM ANY CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE
 * FOR OBTAINING ANY RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.
 * XILINX EXPRESSLY DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO
 * THE ADEQUACY OF THE IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO
 * ANY WARRANTIES OR REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE
 * FROM CLAIMS OF INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <lwip/sockets.h>
#include <lwip/sys.h>

#include <string.h>

#include "echo.h"

#define OS_IS_FREERTOS 1

unsigned short echo_port = 7;

typedef struct
{
    int new_sd;
    SemaphoreHandle_t StatSem;
}echo_param;

void print_echo_app_header()
{
    printf("%13s %4d %s\r\n", "echo server",echo_port,"$ telnet <board_ip> 7");
}

/* thread spawned for each connection */
void process_echo_request(void *p)
{
    echo_param* echo_param1 = (echo_param*)p;
	int sd = (int)echo_param1->new_sd;
	int RECV_BUF_SIZE = 50;
	char recv_buf[RECV_BUF_SIZE];
	int n, nwrote;
	//xTaskHandle taskHello;
	//xTaskHandle taskBye;

	while (1) {
		/* read a max of RECV_BUF_SIZE bytes from socket */
		if ((n = read(sd, recv_buf, RECV_BUF_SIZE)) < 0) {
			printf("%s: error reading from socket %d, closing socket\r\n", __FUNCTION__, sd);
#ifdef OS_IS_FREERTOS
			break;
#else
    #ifdef APISSYS_RTOS
			break;
    #else
			close(sd);
			return;
    #endif
#endif
		}

		/* break if the recved message = "quit" */
		if (!strncmp(recv_buf, "quit", 4))
			break;

		/* break if client closed connection */
		if (n <= 0)
			break;

		/* handle request */
		if ((nwrote = write(sd, recv_buf, n)) < 0) {
			printf("%s: ERROR responding to client echo request. received = %d, written = %d\r\n",__FUNCTION__, n, nwrote);
			printf("Closing socket %d\r\n", sd);
#ifdef OS_IS_FREERTOS
			break;
#else
    #ifdef APISSYS_RTOS
			break;
    #else
			close(sd);
			return;
    #endif
#endif
		}
	}

	/* close connection */
	close(sd);
#ifdef OS_IS_FREERTOS
	vTaskDelete(NULL);
#endif
#ifdef APISSYS_RTOS
	TaskDelete(NULL);
#endif
}

void echo_application_thread(void *pvParameters)
{
	int sock, new_sd;
	struct sockaddr_in address, remote;
	int size;
        echo_param* echo_param1 = malloc(sizeof(echo_param));

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return;

	address.sin_family = AF_INET;
	address.sin_port = htons(echo_port);
	address.sin_addr.s_addr = INADDR_ANY;

	if (bind(sock, (struct sockaddr *)&address, sizeof (address)) < 0)
		return;

	listen(sock, 0);

	size = sizeof(remote);
        echo_param1->StatSem = pvParameters;

	while (1) {
		if ((new_sd = accept(sock, (struct sockaddr *)&remote, (socklen_t *)&size)) > 0) {
                        echo_param1->new_sd = new_sd;
			sys_thread_new("echo connexion", process_echo_request,(void*)echo_param1,THREAD_STACKSIZE,DEFAULT_THREAD_PRIO);
		}
	}
}
