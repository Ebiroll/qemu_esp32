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


static const char *TAG = "gdp_ota";

typedef struct
{
    int new_sd;
    SemaphoreHandle_t StatSem;
} gdb_param;


/* thread spawned for each connection */
void process_gdb_request(void *p)
{
    gdb_param* gdb_param1 = (gdb_param*)p;
	int sd = (int)gdb_param1->new_sd;
	int RECV_BUF_SIZE = 100;
	char recv_buf[RECV_BUF_SIZE];
	int n;
    int received=0;

    int32_t lReceivedValue;
    BaseType_t xStatus=pdFAIL;

    int32_t lValueToSend; 
    //  const TickType_t xTicksToWait = pdMS_TO_TICKS(1100);


	while (1) {
		/* read a max of RECV_BUF_SIZE bytes from socket */
		if ((n = read(sd, recv_buf, RECV_BUF_SIZE)) < 0) {
			printf("%s: error reading from socket %d, closing socket\r\n", __FUNCTION__, sd);
            close(sd);
			break;
        }
        received=0;
        while (received<n) {
            lValueToSend=recv_buf[received];
            printf("%c",lValueToSend);
            xStatus = xQueueSendToBack(receive_queue, &lValueToSend, 0);
            if( xStatus != pdPASS )
            {
                printf("Could not send to the queue.");
            }
            received++;
        }

        //close(sd);
        //return;
 		
		printf("\nread %d bytes\n",n);

		/* break if the recved message = "quit" */
		//if (!strncmp(recv_buf, "quit", 4))
		//	break;

		/* break if client closed connection */
		if (n <= 0)
			break;

		/* handle request */

#if 0
        // Wait fore reply
        while( uxQueueMessagesWaiting( send_queue ) == 0 ) {
           vTaskDelay(pdMS_TO_TICKS(500));
        }
        printf("send:");

        while( uxQueueMessagesWaiting( send_queue ) > 0 ) {
            xStatus = xQueueReceive ( send_queue, &lReceivedValue, xTicksToWait );

            if( xStatus == pdPASS ) {
                printf("%c",lReceivedValue);
                if ((nwrote = write(sd, &lReceivedValue, 1)) < 0) {
                    printf("%s: ERROR responding to gdb request. received = %d, written = %d\r\n",__FUNCTION__, n, nwrote);
                    printf("Closing socket %d\r\n", sd);
                    return;
                }
            }
            //if (nwrote<0) {
			//  close(sd);
            //}
		}
#endif

        



        #if 0


        xStatus = xQueueReceive ( my_queue, &lReceivedValue, xTicksToWait );
        if( xStatus == pdPASS ) {
            ESP_LOGI(TAG, "Received = %d", lReceivedValue);
        } else {
            ESP_LOGI(TAG,  "Could not receive from the queue.");
        }
        //////////////////

		if ((nwrote = write(sd, recv_buf, n)) < 0) {
			printf("%s: ERROR responding to client echo request. received = %d, written = %d\r\n",__FUNCTION__, n, nwrote);
			printf("Closing socket %d\r\n", sd);
			break;
			//close(sd);
			//return;
		}
        #endif
	}

	/* close connection */
	close(sd);
	vTaskDelete(NULL);
}


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
    gdb_param* gdb_param1 = malloc(sizeof(gdb_param));

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
        xTaskCreate(&gdb_main, "gdb_main",2*4096, (void*)gdb_param1, 20, NULL);
		if ((new_sd = accept(sock, (struct sockaddr *)&remote, (socklen_t *)&size)) > 0) {
			printf("accepted new gdb connection\n");
                        gdb_param1->new_sd = new_sd;
	                    xTaskCreate(&process_gdb_request, "gdb_connection",2*4096, (void*)gdb_param1, 20, NULL);
                        set_gdb_socket(new_sd);
                        //wifi_gdb_handler(new_sd);  
	        }
	}
}

#if 0
char recv_buf[64];


static void http_get_task(void *pvParameters)
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;

    while(1) {
        /* Wait for the callback to set the CONNECTED_BIT in the
           event group.
        */
        xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                            false, true, portMAX_DELAY);
        ESP_LOGI(TAG, "Connected to AP");

        int err = getaddrinfo(WEB_SERVER, "80", &hints, &res);

        if(err != 0 || res == NULL) {
            ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        /* Code to print the resolved IP.

           Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

        s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            ESP_LOGE(TAG, "... Failed to allocate socket.");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... allocated socket\r\n");

        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
            close(s);
            freeaddrinfo(res);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(TAG, "... connected");
        freeaddrinfo(res);

        if (write(s, REQUEST, strlen(REQUEST)) < 0) {
            ESP_LOGE(TAG, "... socket send failed");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... socket send success");

        /* Read HTTP response */
        do {
            bzero(recv_buf, sizeof(recv_buf));
            r = read(s, recv_buf, sizeof(recv_buf)-1);
            for(int i = 0; i < r; i++) {
                putchar(recv_buf[i]);
            }
        } while(r > 0);

        ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d\r\n", r, errno);
        close(s);
        for(int countdown = 10; countdown >= 0; countdown--) {
            ESP_LOGI(TAG, "%d... ", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "Starting again!");
    }
}
#endif



void app_main()
{
    ESP_ERROR_CHECK( nvs_flash_init() );

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

    // Wait for this...
     set_all_exception_handlers();

    xTaskCreate(&gdb_application_thread, "gdb_thread", 4*4096, NULL, 17, NULL);
}
