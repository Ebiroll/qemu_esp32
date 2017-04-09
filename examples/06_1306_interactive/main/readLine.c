#include "driver/uart.h"
#include <stdio.h>

#define ESP_REG(addr) *((volatile uint32_t *)(addr))


char *readLine(uart_port_t uart,char *line,int len) {
	int size;
	char *ptr = line;
	while(1) {
		size = uart_read_bytes(uart, (unsigned char *)ptr, 1, portMAX_DELAY);
		if (size == 1) {
			if (*ptr == '\n') {
				*ptr = 0;
				return line;
			}
			ptr++;
		} // End of read a character
	} // End of loop
} // End of readLine


// This function polls uart data 
char *pollLine(uart_port_t uart,char *line,int len) {
	int size;
	volatile uint32_t* FIFO=0;
	volatile uint32_t* RX_STATUS=0;

	char *ptr = line;
	switch (uart) {
		case UART_NUM_0:
		    FIFO=0x3ff40000;
			RX_STATUS=0x3ff4001c;
		break;
		case UART_NUM_1:
			FIFO=0x3ff50000;
			RX_STATUS=0x3ff5001c;
		break;
		case UART_NUM_2:
			FIFO=0x3ff6e000;	
			RX_STATUS=0x3ff6e01c;			
		break;		
		default:
		break;
	}

	while(1) {
		if (ESP_REG(RX_STATUS)>0) {
			*ptr=(ESP_REG(FIFO) & 0x000000ff);
			printf("GOT:%c\n",(char)*ptr);
			if (*ptr == '\n') {
				*ptr = 0;
				return line;
			}
			ptr++;
		} 
		vTaskDelay(100 / portTICK_PERIOD_MS);
	} 
} 

