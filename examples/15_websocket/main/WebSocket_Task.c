/**
 * @section License
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017, Thomas Barth, barth-dev.de
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "WebSocket_Task.h"

#include "freertos/FreeRTOS.h"
#include "esp_heap_alloc_caps.h"
#include "hwcrypto/sha.h"
#include "esp_system.h"
#include "wpa2/utils/base64.h"
#include <string.h>
#include <stdlib.h>

#define WS_PORT				9998	/**< \brief TCP Port for the Server*/
#define WS_CLIENT_KEY_L		24		/**< \brief Length of the Client Key*/
#define SHA1_RES_L			20		/**< \brief SHA1 result*/
#define WS_STD_LEN			125		/**< \brief Maximum Length of standard length frames*/
#define WS_SPRINTF_ARG_L	4		/**< \brief Length of sprintf argument for string (%.*s)*/

/** \brief Opcode according to RFC 6455*/
typedef enum {
	WS_OP_CON = 0x0, 				/*!< Continuation Frame*/
	WS_OP_TXT = 0x1, 				/*!< Text Frame*/
	WS_OP_BIN = 0x2, 				/*!< Binary Frame*/
	WS_OP_CLS = 0x8, 				/*!< Connection Close Frame*/
	WS_OP_PIN = 0x9, 				/*!< Ping Frame*/
	WS_OP_PON = 0xa 				/*!< Pong Frame*/
} WS_OPCODES;

//reference to the RX queue
extern QueueHandle_t WebSocket_rx_queue;

//Reference to open websocket connection
static struct netconn* WS_conn = NULL;

const char WS_sec_WS_keys[] = "Sec-WebSocket-Key:";
const char WS_sec_conKey[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
const char WS_srv_hs[] ="HTTP/1.1 101 Switching Protocols \r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %.*s\r\n\r\n";


err_t WS_write_data(char* p_data, size_t length) {

	//check if we have an open connection
	if (WS_conn == NULL)
		return ERR_CONN;

	//currently only frames with a payload length <WS_STD_LEN are supported
	if (length > WS_STD_LEN)
		return ERR_VAL;

	//netconn_write result buffer
	err_t result;

	//prepare header
	WS_frame_header_t hdr;
	hdr.FIN = 0x1;
	hdr.payload_length = length;
	hdr.mask = 0;
	hdr.reserved = 0;
	hdr.opcode = WS_OP_TXT;

	//send header
	result = netconn_write(WS_conn, &hdr, sizeof(WS_frame_header_t), NETCONN_COPY);

	//check if header was send
	if (result != ERR_OK)
		return result;

	//send payload
	return netconn_write(WS_conn, p_data, length, NETCONN_COPY);
}

static void ws_server_netconn_serve(struct netconn *conn) {

	//Netbuf
	struct netbuf *inbuf;

	//message buffer
	char *buf;

	//pointer to buffer (multi purpose)
	char* p_buf;

	//Pointer to SHA1 input
	char* p_SHA1_Inp;

	//Pointer to SHA1 result
	char* p_SHA1_result;

	//multi purpose number buffer
	uint16_t i;

	//will point to payload (send and receive
	char* p_payload;

	//Frame header pointer
	WS_frame_header_t* p_frame_hdr;

	//allocate memory for SHA1 input
	p_SHA1_Inp = pvPortMallocCaps(WS_CLIENT_KEY_L + sizeof(WS_sec_conKey),
			MALLOC_CAP_8BIT);

	//allocate memory for SHA1 result
	p_SHA1_result = pvPortMallocCaps(SHA1_RES_L, MALLOC_CAP_8BIT);

	//Check if malloc suceeded
	if ((p_SHA1_Inp != NULL) && (p_SHA1_result != NULL)) {

		//receive handshake request
		if (netconn_recv(conn, &inbuf) == ERR_OK) {

			//read buffer
			netbuf_data(inbuf, (void**) &buf, &i);

			//write static key into SHA1 Input
			for (i = 0; i < sizeof(WS_sec_conKey); i++)
				p_SHA1_Inp[i + WS_CLIENT_KEY_L] = WS_sec_conKey[i];

			//find Client Sec-WebSocket-Key:
			p_buf = strstr(buf, WS_sec_WS_keys);

			//check if needle "Sec-WebSocket-Key:" was found
			if (p_buf != NULL) {

				//get Client Key
				for (i = 0; i < WS_CLIENT_KEY_L; i++)
					p_SHA1_Inp[i] = *(p_buf + sizeof(WS_sec_WS_keys) + i);

				// calculate hash
				esp_sha(SHA1, (unsigned char*) p_SHA1_Inp, strlen(p_SHA1_Inp),
						(unsigned char*) p_SHA1_result);

				//hex to base64
				p_buf = (char*) _base64_encode((unsigned char*) p_SHA1_result,
						SHA1_RES_L, (size_t*) &i);

				//free SHA1 input
				free(p_SHA1_Inp);

				//free SHA1 result
				free(p_SHA1_result);

				//allocate memory for handshake
				p_payload = pvPortMallocCaps(
						sizeof(WS_srv_hs) + i - WS_SPRINTF_ARG_L,
						MALLOC_CAP_8BIT);

				//check if malloc suceeded
				if (p_payload != NULL) {

					//prepare handshake
					sprintf(p_payload, WS_srv_hs, i - 1, p_buf);

					//send handshake
					netconn_write(conn, p_payload, strlen(p_payload),
							NETCONN_COPY);

					//free base 64 encoded sec key
					free(p_buf);

					//free handshake memory
					free(p_payload);

					//set pointer to open WebSocket connection
					WS_conn = conn;

					//Wait for new data
					while (netconn_recv(conn, &inbuf) == ERR_OK) {

						//read data from inbuf
						netbuf_data(inbuf, (void**) &buf, &i);

						//get pointer to header
						p_frame_hdr = (WS_frame_header_t*) buf;

						//check if clients wants to close the connection
						if (p_frame_hdr->opcode == WS_OP_CLS)
							break;

						//get payload length
						if (p_frame_hdr->payload_length <= WS_STD_LEN) {

							//get beginning of mask or payload
							p_buf = (char*) &buf[sizeof(WS_frame_header_t)];

							//check if content is masked
							if (p_frame_hdr->mask) {

								//allocate memory for decoded message
								p_payload = pvPortMallocCaps(
										p_frame_hdr->payload_length + 1,
										MALLOC_CAP_8BIT);

								//check if malloc succeeded
								if (p_payload != NULL) {

									//decode playload
									for (i = 0; i < p_frame_hdr->payload_length;
											i++)
										p_payload[i] = (p_buf + WS_MASK_L)[i]
												^ p_buf[i % WS_MASK_L];
												
									//add 0 terminator
									p_payload[p_frame_hdr->payload_length] = 0;
								}
							} else
								//content is not masked
								p_payload = p_buf;

							//do stuff
							if ((p_payload != NULL)	&& (p_frame_hdr->opcode == WS_OP_TXT)) {

								//prepare FreeRTOS message
								WebSocket_frame_t __ws_frame;
								__ws_frame.conenction=conn;
								__ws_frame.frame_header=*p_frame_hdr;
								__ws_frame.payload_length=p_frame_hdr->payload_length;
								__ws_frame.payload=p_payload;

								//send message
								xQueueSendFromISR(WebSocket_rx_queue,&__ws_frame,0);
							}

							//free payload buffer (in this demo done by the receive task)
//							if (p_frame_hdr->mask && p_payload != NULL)
//								free(p_payload);

						} //p_frame_hdr->payload_length<126

						//free input buffer
						netbuf_delete(inbuf);

					} //while(netconn_recv(conn, &inbuf)==ERR_OK)
				} //p_payload!=NULL
			} //check if needle "Sec-WebSocket-Key:" was found
		} //receive handshake
	} //p_SHA1_Inp!=NULL&p_SHA1_result!=NULL

	//release pointer to open WebSocket connection
	WS_conn = NULL;

	//delete buffer
	netbuf_delete(inbuf);

	// Close the connection
	netconn_close(conn);

	//Delete connection
	netconn_delete(conn);

}

void ws_server(void *pvParameters) {
	//connection references
	struct netconn *conn, *newconn;

	//set up new TCP listener
	conn = netconn_new(NETCONN_TCP);
	netconn_bind(conn, NULL, WS_PORT);
	netconn_listen(conn);

	//wait for connections
	while (netconn_accept(conn, &newconn) == ERR_OK)
		ws_server_netconn_serve(newconn);

	//close connection
	netconn_close(conn);
	netconn_delete(conn);
}
