/* Gdb over wifi example
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "emul_ip.h"
#include "freertos/queue.h"
#include "gdb_main.h"
#include "gdbstub-freertos.h"

extern void set_all_exception_handlers(void);
extern void wifi_gdb_handler(int socket);

unsigned short gdb_port = 2345;

QueueHandle_t receive_queue;
QueueHandle_t send_queue;

/* The examples use simple WiFi configuration that you can set via
   'make menuconfig'.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_WIFI_SSID CONFIG_WIFI_SSID
#define EXAMPLE_WIFI_PASS CONFIG_WIFI_PASSWORD

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;


static const char *TAG = "blackmagic";




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

static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
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
}

void set_gdb_socket(int socket);


void gdb_application_thread(void *pvParameters)
{
	int sock, new_sd;
	struct sockaddr_in address, remote;
	int size;

        xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                            false, true, portMAX_DELAY);
        ESP_LOGI(TAG, "Connected to AP");



	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return;

	address.sin_family = AF_INET;
	address.sin_port = htons(gdb_port);
	address.sin_addr.s_addr = INADDR_ANY;

	if (bind(sock, (struct sockaddr *)&address, sizeof (address)) < 0)
		return;

	listen(sock, 0);

	size = sizeof(remote);

	while (1) {
		if ((new_sd = accept(sock, (struct sockaddr *)&remote, (socklen_t *)&size)) > 0) {
			printf("accepted new gdb connection\n");
                        set_gdb_socket(new_sd);
                        gdb_main();
	        }
	}
}



void app_main()
{
    ESP_ERROR_CHECK( nvs_flash_init() );

    //gdbstub_freertos_task_list("qXfer:threads:read::0,18");

    if (is_running_qemu()) {
     tcpip_adapter_init();
     wifi_event_group = xEventGroupCreate();
     ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
     xTaskCreate(&task_lwip_init, "task_lwip_init",2*4096, NULL, 22, NULL); 
    }
    else
    {
      initialise_wifi();
    }

    receive_queue = xQueueCreate( 100, sizeof( int32_t ) );
    //send_queue = xQueueCreate( 100, sizeof( int32_t ) );

    // Dont yet do this...
    // set_all_exception_handlers();

    xTaskCreate(&gdb_application_thread, "gdb_thread", 4*4096, NULL, 17, NULL);
}
