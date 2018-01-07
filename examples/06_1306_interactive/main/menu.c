/*
MIT License

Copyright (c) 2017 Olof Astrand (Ebiroll)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <lwip/sockets.h>
#include <lwip/sys.h>
#include "esp_system.h"
#include <nvs_flash.h>

#include <string.h>
#include "esp32_i2c.h"
#include "1306.h"
#include "menu.h"

#define OS_IS_FREERTOS 1

extern unsigned char ACROBOT[];

unsigned short echo_port = 10023;

typedef struct
{
    int new_sd;
    SemaphoreHandle_t StatSem;
} echo_param;

void print_echo_app_header()
{
    printf("%13s %4d %s\r\n", "echo server",echo_port,"$ telnet <board_ip> 10023");
}

int printMenu(char *data,int maxlen) {

    sprintf(data,"0. i2c_scan\n\
1. clear display.\n\
2. Set page and col to 0.\n\
3. Set page to 4.\n\
4. Set column to 4.\n\
5. Send 8*0xff.\n\
6. Send 8*0x0f.\n\
7. Incremet number and draw it.\n\
8. draw dot.\n\
9. draw image.\n\n");

return (strlen(data));
}





/* thread spawned for each connection */
void process_menu_request(void *p)
{
    echo_param* echo_param1 = (echo_param*)p;
	int sd = (int)echo_param1->new_sd;
	int RECV_BUF_SIZE = 50;
	char recv_buf[RECV_BUF_SIZE];
	int n, nwrote;
	int display_number=0;
	unsigned char display_data[8];
	char *send_buf=malloc(1024*2);

	while (1) {
		send_buf[0]=0;
		int len=printMenu(send_buf,1024*2);

		if ((nwrote = write(sd, send_buf, len)) < 0) {
			printf("%s: ERROR Sending menu. , towrite=%d written = %d\r\n",__FUNCTION__,len, nwrote);
			printf("Closing socket %d\r\n", sd);
			break;
		}


		/* read a max of RECV_BUF_SIZE bytes from socket */
		if ((n = read(sd, recv_buf, RECV_BUF_SIZE)) < 0) {
			printf("%s: error reading from socket %d, closing socket\r\n", __FUNCTION__, sd);
			break;
		}
		printf("read %d menusocket bytes\n",n);
		if (n>2) {
  		   printf("%c%c\n",recv_buf[0],recv_buf[1]);
		}
		recv_buf[n+1]=0;

		/* break if the recved message = "quit" */
		if (!strncmp(recv_buf, "quit", 4))
			break;

		if (!strncmp(recv_buf, "0", 1))
		{
			printf("i2c_scan\n");
			i2c_scan();
		}

		if (!strncmp(recv_buf, "1", 1))
		{
			clear_display();
		}

		if (!strncmp(recv_buf, "2", 1))
		{
			Set_Page_Address(0);
			Set_Column_Address(0);
		}

		if (!strncmp(recv_buf, "3", 1))
		{
			Set_Page_Address(4);
		}

		if (!strncmp(recv_buf, "4", 1))
		{
			Set_Column_Address(4);
		}

		if (!strncmp(recv_buf, "5", 1))
		{
			int j=0;
			for (j=0;j<8;j++) {
				display_data[j]=0xff;
			}
			Write_data(display_data,8);

		}

		if (!strncmp(recv_buf, "6", 1))
		{
			int j=0;
			for (j=0;j<8;j++) {
				display_data[j]=0x0f;
			}
			Write_data(display_data,8);
		}

		if (!strncmp(recv_buf, "7", 1))
		{
			display_number++;
			display_three_numbers(display_number,0,0);
		}

		if (!strncmp(recv_buf, "8", 1))
		{
			display_dot(4);
		}
		if (!strncmp(recv_buf, "9", 1))
		{
		        int dataLen=1024;
		        unsigned char*data=ACROBOT;
			Set_Page_Address(0);
			Set_Column_Address(0);
			while(dataLen>0) {
			  Write_data(data,128);
			  data+=128;
			  dataLen-=128;
			}
		}
		
		/* break if client closed connection */
		if (n <= 0)
			break;

		/* handle request */
		if ((nwrote = write(sd, recv_buf, n)) < 0) {
			printf("%s: ERROR responding to client echo request. received = %d, written = %d\r\n",__FUNCTION__, n, nwrote);
			printf("Closing socket %d\r\n", sd);
			break;
		}
	}

	/* close connection */
	close(sd);
	vTaskDelete(NULL);
}

void menu_application_thread(void *pvParameters)
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
			printf("accepted new menu connection\n");
                        echo_param1->new_sd = new_sd;
			sys_thread_new("menu connection", process_menu_request,(void*)echo_param1,2*THREAD_STACKSIZE,DEFAULT_THREAD_PRIO);
		}
	}
}
