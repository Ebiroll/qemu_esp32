/*
 * Copyright (c) 2009 Xilinx, Inc.  All rights reserved.
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

#include <stdio.h>
#include <string.h>

#include "lwip/err.h"
#include "lwip/udp.h"


int transfer_data() {
	return 0;
}

struct ip_addr forward_ip;
//#define fwd_port 1234
#define fwd_port 5001
#define transmit_port 5001

void udp_echo_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, struct ip_addr *addr, u16_t port){
	int i;

	xil_printf("received at %d, echoing to the same port\n",pcb->local_port);
	//dst_ip = &(pcb->remote_ip); // this is zero always
      if (p != NULL) {
//    	  printf("UDP rcv %d bytes: ", (*p).len);
//    	  for (i = 0; i < (*p).len; ++i)
//			printf("%c",((char*)(*p).payload)[i]);
//    	printf("\n");
            //udp_sendto(pcb, p, IP_ADDR_BROADCAST, 1234); //dest port

    	udp_sendto(pcb, p, &forward_ip, fwd_port); //dest port
            pbuf_free(p);
      }
}

struct udp_pcb *broadcast_pcb;

#define out_buf_size 50
int out_buf_i = 0;
int buf[out_buf_size];

void udp_sample(int sample) {

	buf[out_buf_i] = sample;
	out_buf_i++;
	if (out_buf_i == out_buf_size) {

        //XTmrCtr_Reset(&Sampling_Timer, timer);

			struct pbuf * p = pbuf_alloc(PBUF_TRANSPORT, out_buf_size * sizeof(int), PBUF_REF);
			p->payload = buf;

	//		xil_printf("transmitting audio to to port=%d at " , transmit_port);
	//		xil_printf("sending %d bytes (printing in words) from buf @ %X:", p->len, p->payload);
	//		for (out_buf_i = 0 ; out_buf_i != out_buf_size ; out_buf_i++)
	//			xil_printf("%d,", buf[out_buf_i]);
	//		xil_printf("\n");

			xil_printf("udp sampes to port %d\n", fwd_port);
			udp_sendto(broadcast_pcb, p, &forward_ip, fwd_port); //dest port
			pbuf_free(p);

			out_buf_i = 0;

//		int cycles = XTmrCtr_GetValue(&Sampling_Timer, timer);
//        xil_printf("udp: %d timer ticks\n\r", cycles);

	}
}


///////////

static unsigned txperf_client_connected = 0;

static struct udp_pcb *connected_pcb = NULL;
static struct pbuf *pbuf_to_be_sent = NULL;

#define SEND_BUFSIZE (1424)
static char send_buf[SEND_BUFSIZE];

int
transfer_utxperf_data()
{
	int copy = 1;
	err_t err;
	struct udp_pcb *pcb = connected_pcb;
	static int id;
	int *payload;

	if (!connected_pcb) {
                xil_printf("!connected_pcb\r\n");
		return -1;
	}

#define ALLOC_PBUF_ON_TRANSFER 0
#if ALLOC_PBUF_ON_TRANSFER
	pbuf_to_be_sent = pbuf_alloc(PBUF_RAW, 1400, PBUF_POOL);
	if (!pbuf_to_be_sent)
		//xil_printf("error allocating pbuf to send\r\n");
		return -1;
	else {
		//memcpy(pbuf_to_be_sent->payload, send_buf, 1024);
		pbuf_to_be_sent->len = 1400;
	}
#endif

	if (!pbuf_to_be_sent) {
		return -1;
	}

	/* always increment the id */
	payload = (int*)(pbuf_to_be_sent->payload);
	*payload = id++;

	err = udp_send(pcb, pbuf_to_be_sent);
	if (err != ERR_OK) {
		xil_printf("Error on udp_send: %d\r\n", err);
		return -1;
	}

#if ALLOC_PBUF_ON_TRANSFER
	pbuf_free(pbuf_to_be_sent);
#endif

	return 0;
}

int
start_utxperf_application()
{
	struct udp_pcb *pcb;
	struct ip_addr ipaddr;
	err_t err;
	int i;

	/* create a udp socket */
	pcb = udp_new();
	if (!pcb) {
		xil_printf("Error creating PCB. Out of Memory\r\n");
		return -1;
	}

	/* bind local address */
	if ((err = udp_bind(pcb, IP_ADDR_ANY, 0)) != ERR_OK) {
		xil_printf("error on udp_bind: %x\n\r", err);
	}

	err = udp_connect(pcb, &forward_ip, fwd_port);

	/* initialize data buffer being sent */
	for (i = 0; i < SEND_BUFSIZE; i++)
		send_buf[i] = (i % 10) + '0';

	connected_pcb = pcb;

#if !ALLOC_PBUF_ON_TRANSFER
	pbuf_to_be_sent = pbuf_alloc(PBUF_RAW, 1450, PBUF_POOL);
	if (!pbuf_to_be_sent)
		xil_printf("error allocating pbuf to send\r\n");
	else {
		memcpy(pbuf_to_be_sent->payload, send_buf, 1024);
		pbuf_to_be_sent->len = 1450;
	}
#endif

	return 0;
}


//////////////////////////////////////////////////////

void start_udp( vBasicTFTPServer, pvParameters ){
	struct udp_pcb *ptel_pcb = udp_new();
	int port = fwd_port;

	udp_bind(ptel_pcb, IP_ADDR_ANY, port);
	IP4_ADDR(&forward_ip, 192, 168,   1, 1);
	//IP4_ADDR(&forward_ip, 255, 255,   255,  255);
	xil_printf("Listening UDP datagrams at %d, forwarding to port=%d at " , port, fwd_port);
	print_ip("", &forward_ip);

	udp_recv(ptel_pcb, udp_echo_recv, NULL);

//	broadcast_pcb = udp_new(); // AUDIO
//	int i;
//	for (i = 0 ; i != out_buf_size ; i++)
//		udp_sample(i);
	start_utxperf_application();

}


