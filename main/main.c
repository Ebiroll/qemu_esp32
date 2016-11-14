/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "soc/rtc_cntl_reg.h" 
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
// Wifi/net related
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include <string.h>
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netif.h"
#include "netif/etharp.h"
// #include "ne2kif.h"
extern err_t ne2k_init(struct netif *netif);

// Extern echo function
extern void echo_application_thread(void *pvParameters);


/* For testing of how code is generated  */
#if 0
/*         "wsr 	a3, ps\n"\ */
         "movi.n	a2, 0x3ffe0400\n"
         "movi.n	a3,0x400807BC\n"
        "movi.n	a2, 0x3ffe0400\n"\
        "movi.n	a3, 0x400807BC\n"\
        "s32i.n        a2, a3 , 0\n"\

#endif

	 // Load a9 with adress to user_entr  
	 // Load a8 with adress to elf_entry  
void patch() {
    asm volatile (\
    "j      0x40007023\n"\
	"nop.n\n"\
	"nop\n"\
	"l32r     a9,0x3ffe040\n"\
	"l32r     a8,0x40\n"\
    "s32i.n   a9,a8,0\n"\
	"movi.n	a2, 0\n");
}

void retint() {
      unsigned int*mem_location=(unsigned int*)0x40000000;
      *mem_location=0x3ffe0400;
}

int test(int in) {
    return (in & 0xfffffff7);
}

void jump() {
    asm volatile (\
         "RFE\n"\
         "jx a0\n");
    retint();
}


/* produce simple hex dump output */
void simpleHex(int toOutout)
{

    printf( "%08X", toOutout);
}



void dump_task(void *pvParameter)
{
    unsigned char*mem_location=(unsigned char*)0x40000000;
    unsigned int*simple_mem_location=(unsigned int*)mem_location;
    unsigned int* end=(unsigned int*)0x400C1FFF;

    printf("\n");

    int *GPIO_STRAP_TEST=(int *)0x3ff44038;
    printf( "GPIO STRAP REG=%08X\n", *GPIO_STRAP_TEST);

    //RTC_CNTL_RESET_STATE_REG  3ff48034

    int *test=(int *)RTC_CNTL_RESET_STATE_REG;
    printf( "RTC_CNTL_RESET_STATE_REG,%08X=%08X\n",RTC_CNTL_RESET_STATE_REG, *test);


    test=(int *)RTC_CNTL_INT_ST_REG;
    printf( "RTC_CNTL_INT_ST_REG; %08X=%08X\n",RTC_CNTL_INT_ST_REG, *test);

    // RTCIO_RTC_GPIO_PIN3_REG 

    //test=(int *)RTCIO_RTC_GPIO_PIN3_REG;
    //printf( "RTCIO_RTC_GPIO_PIN3_REG; %08X=%08X\n",RTCIO_RTC_GPIO_PIN3_REG, *test);


    test=(int *)0x3FF00044;
    printf( "GPIO_STATUS_REG ; %08X=%08X\n",0x3FF00044, *test);

    test=(int *)0x3FF00040;
    printf( "GPIO_IN1_REG 32-39; %08X=%08X\n",0x3FF00040, *test);


    test=(int *)0x3FF003f0;
    printf( "DPORT DPORT_PRO_DCACHE_DBUG0_REG REG; %08X=%08X\n",0x3FF003f0, *test);


    test=(int *)0x3FF00038;
    printf( "DPORT 38 REG; %08X=%08X\n",0x3FF00038, *test);

    test=(int *)0x3FF4800c;
    printf( "DPORT RTC_CNTL_TIME_UPDATE_REG REG; %08X=%08X\n",0x3FF4800c, *test);

    printf("ROM DUMP\n");
    printf("\n");
    printf("\n");
    fflush(stdout);

    unsigned int simple_crc=0;
    unsigned int pos=0;

    while(simple_mem_location<end) {
        simpleHex(*simple_mem_location);
        simple_crc=simple_crc ^ (*simple_mem_location);
        printf(",");
        if (pos++==7) {
            printf( ":%08X:\n", simple_crc);
            fflush(stdout);
            simple_crc=0;
            pos=0;
            //vTaskDelay(300 / portTICK_PERIOD_MS);
        }
        simple_mem_location++;
    }
    fflush(stdout);


    printf("\n");
    printf("\n");
    printf("ROM1 Dump.\n");
    fflush(stdout);

    simple_crc=0;
    pos=0;
    // rom1 3FF9_0000 0x3FF9_FFFF
    mem_location=(unsigned char*)0x3FF90000;
    simple_mem_location=(unsigned int*)mem_location;
    end=(unsigned int*)0x3FF9FFFF;

    while(simple_mem_location<end) {
        simpleHex(*simple_mem_location);
        simple_crc=simple_crc ^ (*simple_mem_location);
        printf(",");
        if (pos++==7) {
            printf( ":%08X:\n", simple_crc);
            fflush(stdout);
            simple_crc=0;
            pos=0;
            //vTaskDelay(300 / portTICK_PERIOD_MS);
        }
        simple_mem_location++;
    }
    fflush(stdout);



    printf("\n");
    printf("Done\n");
    printf("\n");


    int times=0;
    while (true) {
	    printf("Done %d\n",times++);
        vTaskDelay(300 / portTICK_PERIOD_MS);
    }

#if 0

    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
    int level = 0;
    while (true) {
        gpio_set_level(GPIO_NUM_4, level);
        level = !level;
	    printf("blink\n");
        printf( "DPORT RTC_CNTL_TIME_UPDATE_REG REG; %08X=%08X\n",0x3FF00038, *test);
        vTaskDelay(300 / portTICK_PERIOD_MS);
    }
#endif
    //system_restart();
}

/*
typedef struct {
    system_event_id_t     event_id;     
    system_event_info_t   event_info;   
    } system_event_t;
*/

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_GOT_IP:
        {
            struct netif *netif;
            // List all available interfaces
            for (netif = netif_list; netif != NULL; netif = netif->next) {
                printf("%c%c%d\n",netif->name[0],netif->name[1],netif->num);
                fflush(stdout);
            }
        }
        default:
          break;
    }
    return ESP_OK;
}


void wifi_task(void *pvParameter) {
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    wifi_config_t sta_config = {
        .sta = {
	 #include "secret.h"
	     //.ssid = "ssid",
	     //.password = "password",
	     //.bssid_set = false
        }
    };
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &sta_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    ESP_ERROR_CHECK( esp_wifi_connect() );
    printf("Connected\n");

    xTaskCreate(&echo_application_thread, "echo_thread", 2048, NULL, 5, NULL);


    //gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
    int level = 0;
    // Send arp request

    struct netif *netif;

    for (netif = netif_list; netif != NULL; netif = netif->next) {
        printf("%c%c%d\n",netif->name[0],netif->name[1],netif->num);
        fflush(stdout);
    }


    ip4_addr_t scanaddr;

    netif=netif_find("en0");
    if (!netif) {
        printf("No en0");
    }

    unsigned char hostnum=1;
    char tmpBuff[20];
    // Arpscan network
    while (true) {

        sprintf(tmpBuff,"192.168.1.%d",hostnum);
        IP4_ADDR(&scanaddr, 192, 168 , 1, hostnum);


        //gpio_set_level(GPIO_NUM_4, level);
        if (netif)
        {
            //printf("ARP request %s\n",tmpBuff);
            //err_t ret=etharp_request(netif, &scanaddr);
            //if (ret<0) {
            //    printf("Failed request %s\n",tmpBuff);
            //}
        }

        level = !level;
	//printf(".");
        vTaskDelay(300 / portTICK_PERIOD_MS);
        hostnum++;
    }

}

extern void Task_lwip_init(void * pParam);

void emulated_net(void *pvParameter) {

    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );

    Task_lwip_init(NULL);

    //gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
    int level = 0;
    // Send arp request

    struct netif *netif;

    for (netif = netif_list; netif != NULL; netif = netif->next) {
        printf("%c%c%d\n",netif->name[0],netif->name[1],netif->num);
        fflush(stdout);
    }


    ip4_addr_t scanaddr;

    netif=netif_find("et0");
    if (!netif) {
        printf("No et0");
    }

    xTaskCreate(&echo_application_thread, "echo_thread", 2048, NULL, 5, NULL);

    unsigned char hostnum=1;
    char tmpBuff[20];
    // Arpscan network
    while (true) {

        sprintf(tmpBuff,"192.168.1.%d",hostnum);
        IP4_ADDR(&scanaddr, 192, 168 , 1, hostnum);


        //gpio_set_level(GPIO_NUM_4, level);
        if (netif)
        {
            printf("ARP request %s\n",tmpBuff);
            err_t ret=etharp_request(netif, &scanaddr);
            if (ret<0) {
                printf("Failed request %s\n",tmpBuff);
            }
        }

        level = !level;
	    //printf(".");
        vTaskDelay(300 / portTICK_PERIOD_MS);
        hostnum++;
    }
}


void app_main()
{

    esp_log_level_set("*", ESP_LOG_INFO);
    nvs_flash_init();
    // deprecated init
    //system_init();
    //xTaskCreate(&wifi_task,"wifi_task",2048, NULL, 5, NULL);
    //xTaskCreate(&emulated_net, "emulated_net", 2048, NULL, 5, NULL);
    //emulated_net(NULL);
    //wifi_task(NULL);
    dump_task(NULL);

    // Dumping rom is best done with the esptool.py
    //xTaskCreate(&dump_task, "dump_task", 2048, NULL, 5, NULL);
}
