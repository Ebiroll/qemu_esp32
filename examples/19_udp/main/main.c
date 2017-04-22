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

#include <lwip/sockets.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_err.h>
#include <nvs_flash.h>

#include <lwip/err.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/netdb.h>
#include <lwip/dns.h>
#include "emul_ip.h"


#define EXAMPLE_WIFI_PORT CONFIG_WIFI_PORT
#define EXAMPLE_WIFI_SSID CONFIG_WIFI_SSID
#define EXAMPLE_WIFI_PASS CONFIG_WIFI_PASSWORD


static const char *TAG = "UDP";

static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;
const int STARTED_BIT = BIT1;


#define RECEIVER_PORT_NUM 6001


#define SENDER_PORT_NUM 6000
#define SENDER_IP_ADDR "192.168.1.131"
#define RECEIVER_IP_ADDR "255.255.255.255"


// Similar to uint32_t system_get_time(void)
uint32_t get_usec() {

  struct timeval tv;
   gettimeofday(&tv,NULL);
   return (tv.tv_sec*1000000 + tv.tv_usec);


  //uint64_t tmp=get_time_since_boot();
  //uint32_t ret=(uint32_t)tmp;
  //return ret;
}




void send_thread(void *pvParameters)
{

    int socket_fd;
    struct sockaddr_in sa,ra;

    int sent_data; char data_buffer[80];
    /* Creates an UDP socket (SOCK_DGRAM) with Internet Protocol Family (PF_INET).
     * Protocol family and Address family related. For example PF_INET Protocol Family and AF_INET family are coupled.
    */
    socket_fd = socket(PF_INET, SOCK_DGRAM, 0);

    if ( socket_fd < 0 )
    {

        printf("socket call failed");
        exit(0);

    }

    memset(&sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr(SENDER_IP_ADDR);
    sa.sin_port = htons(SENDER_PORT_NUM);


    /* Bind the TCP socket to the port SENDER_PORT_NUM and to the current
    * machines IP address (Its defined by SENDER_IP_ADDR).
    * Once bind is successful for UDP sockets application can operate
    * on the socket descriptor for sending or receiving data.
    */
    if (bind(socket_fd, (struct sockaddr *)&sa, sizeof(struct sockaddr_in)) == -1)
    {
      printf("Bind to Port Number %d ,IP address %s failed\n",SENDER_PORT_NUM,SENDER_IP_ADDR);
      close(socket_fd);
      exit(1);
    }
    printf("Bind to Port Number %d ,IP address %s SUCCESS!!!\n",SENDER_PORT_NUM,SENDER_IP_ADDR);



    memset(&ra, 0, sizeof(struct sockaddr_in));
    ra.sin_family = AF_INET;
    ra.sin_addr.s_addr = inet_addr(RECEIVER_IP_ADDR);
    ra.sin_port = htons(RECEIVER_PORT_NUM);


    strcpy(data_buffer,"Hello World");
    for (;;) {
        //#define sendto(s,dataptr,size,flags,to,tolen)     lwip_sendto_r(s,dataptr,size,flags,to,tolen)
        sent_data = sendto(socket_fd, data_buffer,sizeof("Hello World"),0,(struct sockaddr*)&ra,sizeof(ra));
        if(sent_data < 0)
        {
            printf("send failed\n");
            close(socket_fd);
            exit(2); 
        }
        vTaskDelay(4000 / portTICK_PERIOD_MS);

    }
    close(socket_fd); 
}


void receive_thread(void *pvParameters)
{

    int socket_fd;
    struct sockaddr_in sa,ra;

    int recv_data; char data_buffer[80];
    /* Creates an UDP socket (SOCK_DGRAM) with Internet Protocol Family (PF_INET).
     * Protocol family and Address family related. For example PF_INET Protocol Family and AF_INET family are coupled.
    */

    socket_fd = socket(PF_INET, SOCK_DGRAM, 0);

    if ( socket_fd < 0 )
    {

        printf("socket call failed");
        exit(0);

    }

    memset(&sa, 0, sizeof(struct sockaddr_in));
    ra.sin_family = AF_INET;
    ra.sin_addr.s_addr = inet_addr(RECEIVER_IP_ADDR);
    ra.sin_port = htons(RECEIVER_PORT_NUM);


    /* Bind the UDP socket to the port RECEIVER_PORT_NUM and to the current
    * machines IP address (Its defined by RECEIVER_PORT_NUM).
    * Once bind is successful for UDP sockets application can operate
    * on the socket descriptor for sending or receiving data.
    */
    if (bind(socket_fd, (struct sockaddr *)&ra, sizeof(struct sockaddr_in)) == -1)
    {

    printf("Bind to Port Number %d ,IP address %s failed\n",RECEIVER_PORT_NUM,RECEIVER_IP_ADDR);
    close(socket_fd);
    exit(1);
    }
    /* RECEIVER_PORT_NUM is port on which Server waits for data to
    * come in. It copies the received data into receive buffer and
    * prints the received data as string. If no data is available it
    * blocks.recv calls typically return any availbale data on the socket instead of waiting for the entire data to come.
    */

    recv_data = recv(socket_fd,data_buffer,sizeof(data_buffer),0);
    if(recv_data > 0)
    {

        data_buffer[recv_data] = '\0';
        printf("%s\n",data_buffer);

    }
    close(socket_fd); 

}

static esp_err_t esp32_wifi_eventHandler(void *ctx, system_event_t *event) {

	switch(event->event_id) {
        case SYSTEM_EVENT_WIFI_READY:
        	ESP_LOGD(TAG, "EVENT_WIFI_READY");

            break;

        case SYSTEM_EVENT_AP_STACONNECTED:
        	ESP_LOGD(TAG, "EVENT_AP_START");
            break;

		// When we have started being an access point
		case SYSTEM_EVENT_AP_START: 
        	ESP_LOGD(TAG, "EVENT_START");
            xEventGroupSetBits(wifi_event_group, STARTED_BIT);            

			break;
        case SYSTEM_EVENT_SCAN_DONE:
        	ESP_LOGD(TAG, "EVENT_SCAN_DONE");
			break;

		case SYSTEM_EVENT_STA_CONNECTED: 
       		ESP_LOGD(TAG, "EVENT_STA_CONNECTED");
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
            break;

		// If we fail to connect to an access point as a station, become an access point.
		case SYSTEM_EVENT_STA_DISCONNECTED:
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);

			ESP_LOGD(TAG, "EVENT_STA_DISCONNECTED");
			// We think we tried to connect as a station and failed! ... become
			// an access point.
			break;

		// If we connected as a station then we are done and we can stop being a
		// web server.
		case SYSTEM_EVENT_STA_GOT_IP: 
			ESP_LOGD(TAG, "********************************************");
			ESP_LOGD(TAG, "* We are now connected to AP")
			ESP_LOGD(TAG, "* - Our IP address is: " IPSTR, IP2STR(&event->event_info.got_ip.ip_info.ip));
			ESP_LOGD(TAG, "********************************************");
            {
                //u32_t *my_ip=(u32_t *)&event->event_info.got_ip.ip_info.ip;
            }
            //printf("Startinng send\n");
            xTaskCreate(&send_thread, "send_thread", 2048, NULL, 5, NULL);

			break;

		default: // Ignore the other event types
			break;
	} // Switch event

	return ESP_OK;
} // esp32_wifi_eventHandler



static void initialise_wifi(void)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
	           .ssid = EXAMPLE_WIFI_SSID,
               .password = EXAMPLE_WIFI_PASS,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    ESP_ERROR_CHECK( esp_wifi_connect() );

}

void app_main()
{
    ESP_ERROR_CHECK( nvs_flash_init() );

    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(esp32_wifi_eventHandler, NULL) );

    // Is this required for udp?
    tcpip_adapter_init();

    if (is_running_qemu()) {
      xTaskCreate(&task_lwip_init, "task_lwip_init",2*4096, NULL, 20, NULL); 
    }
    else
    {
      initialise_wifi();
    }

}
