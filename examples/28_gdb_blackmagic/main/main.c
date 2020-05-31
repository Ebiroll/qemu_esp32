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
//#include "esp_event_loop.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "freertos/queue.h"
#include "gdb_main.h"
#include "gdbstub-freertos.h"
#include <xtensa/corebits.h>
#include <soc/cpu.h>
//#define USE_QEMU CONFIG_ETH_USE_OPENETH

#ifdef USE_QEMU
#include "esp_eth.h"
#include "esp_log.h"
#include "esp_netif.h"
#endif

extern void set_all_exception_handlers(void);
extern void wifi_gdb_handler(int socket);

unsigned short gdb_port = 2345;


/* The examples use simple WiFi configuration that you can set via
   'make menuconfig'.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_WIFI_SSID CONFIG_WIFI_SSID
#define EXAMPLE_WIFI_PASS CONFIG_WIFI_PASSWORD
//#define USE_SERIAL CONFIG_USE_SERIAL


/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;


static const char *TAG = "blackmagic";


#if !defined(USE_SERIAL) 
    #pragma GCC diagnostic warning "Configured to USE WIFI"
#endif

#if defined(USE_SERIAL) 
    #pragma GCC diagnostic warning "Configured to USE SERIAL"
#endif

#if !defined(USE_SERIAL) && !defined (USE_QEMU)

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

#endif

#ifdef USE_QEMU

static esp_eth_handle_t s_eth_handle = NULL;
static esp_eth_mac_t *s_mac = NULL;
static esp_eth_phy_t *s_phy = NULL;
static void *s_eth_glue = NULL;
static esp_ip4_addr_t s_ip_addr;
static xSemaphoreHandle s_semph_get_ip_addrs;
static esp_netif_t *s_example_esp_netif = NULL;


/**
 * @brief Checks the netif description if it contains specified prefix.
 * All netifs created withing common connect component are prefixed with the module TAG,
 * so it returns true if the specified netif is owned by this module
 */
static bool is_our_netif(const char *prefix, esp_netif_t *netif)
{
    return strncmp(prefix, esp_netif_get_desc(netif), strlen(prefix)-1) == 0;
}


static void on_got_ip(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    if (!is_our_netif(TAG, event->esp_netif)) {
        ESP_LOGW(TAG, "Got IPv4 from another interface \"%s\": ignored", esp_netif_get_desc(event->esp_netif));
        return;
    }
    ESP_LOGI(TAG, "Got IPv4 event: Interface \"%s\" address: " IPSTR, esp_netif_get_desc(event->esp_netif), IP2STR(&event->ip_info.ip));
}


/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");

    memcpy(&s_ip_addr, &event->ip_info.ip, sizeof(s_ip_addr));
    xSemaphoreGive(s_semph_get_ip_addrs);
}

static esp_netif_t* eth_start(void)
{
    char *desc;

    esp_netif_ip_info_t ip_info;
    esp_netif_inherent_config_t netif_common_config = {
            .flags = ESP_NETIF_FLAG_AUTOUP,
            .ip_info = (esp_netif_ip_info_t*)&ip_info,
            .if_key = "TEST",
            .if_desc = "net_test_if"
    };
    esp_netif_set_ip4_addr(&ip_info.ip, 192, 168 , 3, 40);

    esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_ETH();
    // Prefix the interface description with the module TAG
    // Warning: the interface desc is used in tests to capture actual connection details (IP, gw, mask)
    asprintf(&desc, "%s: %s", TAG, esp_netif_config.if_desc);
    esp_netif_config.if_desc = desc;
    esp_netif_config.route_prio = 64;
    esp_netif_config_t netif_config = {
            .base = &esp_netif_config,
            .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH
    };
    esp_netif_t *netif = esp_netif_new(&netif_config);
    assert(netif);
 
    free(desc);

    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    // the numbers are not so important for simulated network
    phy_config.phy_addr = 1;
    phy_config.reset_gpio_num = 17;
    phy_config.autonego_timeout_ms = 100;
    s_mac = esp_eth_mac_new_openeth(&mac_config);
    s_phy = esp_eth_phy_new_dp83848(&phy_config);


    // Register user defined event handers
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));


    // Install Ethernet driver
    esp_eth_config_t config = ETH_DEFAULT_CONFIG(s_mac, s_phy);
    ESP_ERROR_CHECK(esp_eth_driver_install(&config, &s_eth_handle));
    // combine driver with netif
    s_eth_glue = esp_eth_new_netif_glue(s_eth_handle);
    esp_netif_attach(netif, s_eth_glue);
    esp_eth_start(s_eth_handle);
    return netif;
}

static void eth_stop(void)
{
    esp_netif_t *eth_netif = s_example_esp_netif; //get_example_netif_from_desc("eth");
    ESP_ERROR_CHECK(esp_eth_stop(s_eth_handle));
    ESP_ERROR_CHECK(esp_eth_del_netif_glue(s_eth_glue));
    ESP_ERROR_CHECK(esp_eth_clear_default_handlers(eth_netif));
    ESP_ERROR_CHECK(esp_eth_driver_uninstall(s_eth_handle));
    ESP_ERROR_CHECK(s_phy->del(s_phy));
    ESP_ERROR_CHECK(s_mac->del(s_mac));

    esp_netif_destroy(eth_netif);
    s_example_esp_netif = NULL;
}


/* tear down connection, release resources */
static void stop(void)
{
    eth_stop();
}



void initialize_qemu_net() {

    ESP_ERROR_CHECK(esp_netif_init());
    // Create default event loop that running in background
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *eth_netif = eth_start();

}

#endif

void set_gdb_socket(int socket);

#define MYRSR(reg, curval)  asm volatile ("rsr %0, " #reg : "=r" (curval));

void silly_stack_test(int input)
{
    int count=0;
    MYRSR(SAR,count);
    printf("--- SAR ---0x%X\n",(unsigned int)count);

    MYRSR(LCOUNT,count);
    printf("--- LCOUNT ---0x%X\n",(unsigned int)count);

    MYRSR(PS,count);
    printf("--- PS ---0x%X\n",(unsigned int)count);

    MYRSR(LBEG,count);
    printf("--- LBEG ---0x%X\n",(unsigned int)count);


    while(1) {
        for (;;) {
            vTaskDelay(9000);
            count++;
        }   
    }
}

void demo_thread(void *pvParameters)
{
    char test[10];
    int stack;

    printf("--- test --0x%X\n",(unsigned int)test);
    strcpy(test,"hello");
    silly_stack_test(2);
}


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

void set_exception_handler(int i);

void app_main()
{
    ESP_ERROR_CHECK( nvs_flash_init() );

    //xTaskCreate(&demo_thread, "demo", 4*1024, NULL, 10, NULL);

    //gdbstub_freertos_task_list("qXfer:threads:read::0,18");
#if !defined(USE_SERIAL)
#if defined(USE_QEMU)
     initialize_qemu_net();
#else
    //initialise_wifi();
#endif
#else

    initialise_wifi();


    printf("CONNECT DEBUGGER!\n\n");
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    xTaskCreate(&gdb_application_thread, "gdb_thread", 4*4096, NULL, 17, NULL);


    printf("5\n");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    printf("4\n");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    printf("3\n");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    printf("2\n");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    printf("1\n");
    vTaskDelay(2000 / portTICK_PERIOD_MS);
 
    //gdb_main();
#endif

    // Dont yet do this...
    // set_all_exception_handlers();



}


