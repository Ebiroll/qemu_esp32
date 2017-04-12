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

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE


#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include <sys/time.h>
#include "1306.h"
#include "esp32_i2c.h"
#include "menu.h"
#include <lwip/netif.h>
#include "esp_wifi.h"
#include "esp_wifi_types.h"

#include <string.h>

static const char *TAG = "LR";

static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;
const int STARTED_BIT = BIT1;

extern char *readLine(uart_port_t uart,char *line,int len);

typedef struct {
	unsigned frame_ctrl:16;
	unsigned duration_id:16;
	uint8_t addr1[6]; /* receiver address */
	uint8_t addr2[6]; /* sender address */
	uint8_t addr3[6]; /* filtering address */
	unsigned sequence_ctrl:16;
	uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;

typedef struct {
	wifi_ieee80211_mac_hdr_t hdr;
	uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;


/**
 * An ESP32 WiFi event handler.
 * The types of events that can be received here are:
 *
 * SYSTEM_EVENT_AP_PROBEREQRECVED
 * SYSTEM_EVENT_AP_STACONNECTED
 * SYSTEM_EVENT_AP_STADISCONNECTED
 * SYSTEM_EVENT_AP_START
 * SYSTEM_EVENT_AP_STOP
 * SYSTEM_EVENT_SCAN_DONE
 * SYSTEM_EVENT_STA_AUTHMODE_CHANGE
 * SYSTEM_EVENT_STA_CONNECTED
 * SYSTEM_EVENT_STA_DISCONNECTED
 * SYSTEM_EVENT_STA_GOT_IP
 * SYSTEM_EVENT_STA_START
 * SYSTEM_EVENT_STA_STOP
 * SYSTEM_EVENT_WIFI_READY
 */
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
            xTaskCreate(&menu_application_thread, "menu thread", 2048, NULL, 5, NULL);

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
                u32_t *my_ip=(u32_t *)&event->event_info.got_ip.ip_info.ip;
                ssd1306_128x64_noname_powersave_off();

                unsigned char a=(*my_ip >> 24) & 0xff;
                display_three_numbers(a,12);
                a=(*my_ip >> 16) & 0xff;
                display_dot(3);
                display_three_numbers(a,8);
                a=(*my_ip >> 8) & 0xff;
                display_dot(7);
                display_three_numbers(a,4);
                a=(*my_ip) & 0xff;
                display_dot(11);
                display_three_numbers(a,0);                
            }

            xTaskCreate(&menu_application_thread, "menu thread", 2048, NULL, 5, NULL);

			break;

		default: // Ignore the other event types
			break;
	} // Switch event

	return ESP_OK;
} // esp32_wifi_eventHandler



static void init_wifi(wifi_mode_t mode)
{

    const uint8_t protocol = WIFI_PROTOCOL_LR;

    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(esp32_wifi_eventHandler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    ESP_ERROR_CHECK(esp_wifi_set_mode(mode));
    wifi_event_group=xEventGroupCreate();

    if ( mode == WIFI_MODE_STA ) {
        ESP_ERROR_CHECK(esp_wifi_set_protocol(WIFI_IF_STA,protocol));
        wifi_config_t config = {
            .sta = {
  			   .ssid = "lr_ssid",
               .password = "passthelr",
               .bssid_set = false
            }
        };
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config));
	    ESP_ERROR_CHECK(esp_wifi_start());
	    ESP_ERROR_CHECK(esp_wifi_connect());
        xEventGroupWaitBits(wifi_event_group,CONNECTED_BIT,
        false,true,portMAX_DELAY);
        ESP_LOGI(TAG,"Connected to AP");
    } else {
        ESP_ERROR_CHECK(esp_wifi_set_protocol(WIFI_IF_AP,protocol));
        wifi_config_t ap_config = {
            .ap = {
                .ssid = "lr_ssid",
                .password = "passthelr",
                .ssid_len = 7,
                //.channel = 1,
                .authmode = WIFI_AUTH_WPA2_PSK,
                .ssid_hidden = false,
                .max_connection = 3,
                .beacon_interval = 100
            }
        };

        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
        ESP_ERROR_CHECK(esp_wifi_start());
        //ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    }

}



// Similar to uint32_t system_get_time(void)
uint32_t get_usec() {

  struct timeval tv;
   gettimeofday(&tv,NULL);
   return (tv.tv_sec*1000000 + tv.tv_usec);


  //uint64_t tmp=get_time_since_boot();
  //uint32_t ret=(uint32_t)tmp;
  //return ret;
}

const char *
wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type)
{
	switch(type) {
	case WIFI_PKT_CTRL: return "CTRL";
	case WIFI_PKT_MGMT: return "MGMT";
	case WIFI_PKT_DATA: return "DATA";
	default:	
	case WIFI_PKT_MISC: return "MISC";
	}
}

unsigned char bar_data[256];

unsigned char last_pack=0;

unsigned char progress_data[4];


void showRSS(int rss) {
    int ix=0;
    if (rss<0) {
        rss=-1*rss;
    }

    Set_Page_Address(0);
    Set_Column_Address(0);
    display_three_numbers( rss,0);

    Set_Page_Address(2);
    Set_Column_Address(0);

    for (ix=0;ix<rss && ix<200;ix++) {
        bar_data[ix]=0xff;
        if (ix%10==0) {
           bar_data[ix]=0x0f;           
        }
    }
    while (ix<128) {
        bar_data[ix]=0x0;
        ix++;
    }

    Write_data(progress_data,4);

    Set_Page_Address(2);
    Set_Column_Address(0);

    last_pack=(last_pack+1)%100;

    Write_data(bar_data,200);

    Set_Page_Address(4);
    Set_Column_Address(last_pack);

    Write_data(progress_data,4);


}

void
wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type)
{

	if (type != WIFI_PKT_MGMT)
		return;

	const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
	const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
	const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

    // Only log espresstif AP:s
    if ((hdr->addr2[0]==0x24) && (hdr->addr2[1]==0x0a)) {
        showRSS(ppkt->rx_ctrl.rssi);
    }

	printf("PACKET TYPE=%s, CHAN=%02d, RSSI=%02d, TS=%d"
		" ADDR1=%02x:%02x:%02x:%02x:%02x:%02x,"
		" ADDR2=%02x:%02x:%02x:%02x:%02x:%02x,"
		" ADDR3=%02x:%02x:%02x:%02x:%02x:%02x\n",
		wifi_sniffer_packet_type2str(type),
		ppkt->rx_ctrl.channel,
		ppkt->rx_ctrl.rssi,
        ppkt->rx_ctrl.timestamp,
		/* ADDR1 */
		hdr->addr1[0],hdr->addr1[1],hdr->addr1[2],
		hdr->addr1[3],hdr->addr1[4],hdr->addr1[5],
		/* ADDR2 */
		hdr->addr2[0],hdr->addr2[1],hdr->addr2[2],
		hdr->addr2[3],hdr->addr2[4],hdr->addr2[5],
		/* ADDR3 */
		hdr->addr3[0],hdr->addr3[1],hdr->addr3[2],
		hdr->addr3[3],hdr->addr3[4],hdr->addr3[5]
	);
}


void app_main(void)
{
    char *line=malloc(256);
    char *data;

    uart_port_t uart_num = UART_NUM_0; // uart port number

    // Small moving bar updated for each new value
    progress_data[0]=0;
    progress_data[1]=0;
    progress_data[2]=0xff;
    progress_data[3]=0xff;

    //int *quemu_test=(int *)  0x3ff005f0;
    nvs_flash_init();
    i2c_init(0,0x3C);
    ssd1306_128x64_noname_init();
    //printf("t=0x%x\n",*quemu_test);
    init_wifi(WIFI_MODE_AP);
    // WIFI_MODE_STA
    clear_display();
    ssd1306_128x64_noname_powersave_off();
    display_three_numbers( 0,0);


    xEventGroupWaitBits(wifi_event_group,STARTED_BIT,
    false,true,portMAX_DELAY);

    printf("List all networks\n");
    ip4_addr_t *addr;
    struct netif *netif;
    for (netif = netif_list; netif != NULL; netif = netif->next) {
        printf("%c%c%d\n",netif->name[0],netif->name[1],netif->num);
        addr=(ip4_addr_t *) &netif->ip_addr;
        printf("ip_addr %d.%d.%d.%d\n",IP2STR((addr)));
        fflush(stdout);
    }
    // Assume we have adress 192.168.4.1 for AP 
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);

#if 0

    while (true)
    {
        data = readLine(uart_num, line, 256);
		/* break if the recved message = "quit" */
		if (!strncmp(line, "quit", 4))
			break;

		if (!strncmp(line, "0", 1))
		{
            printf("0. Entering promisc mode\n");

            esp_wifi_set_promiscuous(true);
	        esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);

		} else if (!strncmp(line, "1", 1))
		{
			printf("1\n");            
		} else {
			printf("what?\n 0. enters promisc mode\n");                        
        }
    }
#endif

#if 0
    if (*quemu_test==0x42) {
        printf("Running in qemu\n");
       // Uart
       xTaskCreatePinnedToCore(&uartLoopTask, "loop", 4096, NULL, 20, NULL, 0);

    } else {
        initialise_wifi();
    }
#endif



}
