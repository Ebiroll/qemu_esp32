#include "sdkconfig.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <WebServer.h>
#include <stdio.h>
#include <string>
#include "RequestManager.h"

#include <freertos/event_groups.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_err.h>

extern "C" {
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_log.h"
#include "spiffs_vfs.h"
#include "emul_ip.h"
}


#define EXAMPLE_WIFI_SSID CONFIG_WIFI_SSID
#define EXAMPLE_WIFI_PASS CONFIG_WIFI_PASSWORD

static char tag[] = "Webspiff_MAIN";

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = 0x00000001;

//------------------------------------------------------------
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

//-------------------------------
static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config;
    strcpy(( char *)wifi_config.sta.ssid,( char *)EXAMPLE_WIFI_SSID);
    strcpy(( char *)wifi_config.sta.password , ( char *)EXAMPLE_WIFI_PASS);
/*    
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
            .password = EXAMPLE_WIFI_PASS,
        },
*/

    ESP_LOGI(tag, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

/*
//-------------------------------
static void initialize_sntp(void)
{
    ESP_LOGI(tag, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}
*/

void webserver_task(void* param) {


}

extern "C" {
	int app_main(void);
}

#if 0
#define SPIFFS_BASE_PATH "/spiffs"


extern int spiffs_is_registered;
extern int spiffs_is_mounted;

void vfs_spiffs_register();
int spiffs_mount();
int spiffs_unmount(int unreg);
void spiffs_fs_stat(uint32_t *total, uint32_t *used);
#endif


int app_main(void) {
   RequestManager *manager=new RequestManager();

   //int test_buff[4];
#if 0
   if (spi_flash_read(0x1800fc, (void *)test_buff, 4) != 0) { 
         printf("Failed spi_flash_read\r\n\n");
   }

   printf("%08X\n\n",(unsigned int)test_buff[0]);

   //int write_buff[4];
#endif

   //test_buff[0]=4711;
   //printf("%08X\n\n",(unsigned int)test_buff[0]);

    printf("\r\n\n");
    ESP_LOGI(tag, "==== STARTING SPIFFS TEST ====\r\n");

    vfs_spiffs_register();
    printf("\r\n\n");

    spiffs_mount();

    printf("Mounted \r\n");

    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
 

    if (is_running_qemu()) {
        printf("Running in qemu\n");
        xTaskCreatePinnedToCore(task_lwip_init, "loop", 4096, NULL, 14, NULL, 0);

    }
    else {
       initialise_wifi();
    }

    printf("waiting \r\n");


    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);

    printf("Starting manager\r\n");

    manager->start();

        //xTaskCreatePinnedToCore(task_lwip_init, "loop", 4096, NULL, 14, NULL, 0);


    while (1) {
		vTaskDelay(1000 / portTICK_RATE_MS);
    }
	return 0;
}
