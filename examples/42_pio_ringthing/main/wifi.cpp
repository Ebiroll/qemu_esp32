#include "./wifi.h"

static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;
static const char * TAG  = "WiFi";

static std::unique_ptr<std::function<onConnectedCallback>> onConnected;

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    std::function<onConnectedCallback> *onConnectedFn = onConnected.get();
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "got ip:%s",
                 ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);

        if (onConnectedFn) {
            ESP_LOGI(TAG, "calling onConnected callback");
            (*onConnectedFn)();
        }
        break;
    case SYSTEM_EVENT_AP_STACONNECTED:
        ESP_LOGI(TAG,
                 "station: " MACSTR " join, AID=%d",
                 MAC2STR(event->event_info.sta_connected.mac),
                 event->event_info.sta_connected.aid);
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        ESP_LOGI(TAG,
                 "station:" MACSTR "leave, AID=%d",
                 MAC2STR(event->event_info.sta_disconnected.mac),
                 event->event_info.sta_disconnected.aid);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        break;
    default:
        ESP_LOGI(TAG, "Unknown Event: %d!!!!!", event->event_id);
        break;
    }
    return ESP_OK;
}

void connect_wifi(std::function<onConnectedCallback> userOnConnected) {
    onConnected = std::unique_ptr<std::function<onConnectedCallback>>(&userOnConnected);

    ESP_LOGI("WIFI", "initialization begin");
    wifi_event_group = xEventGroupCreate();

    nvs_flash_init();
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_sta_config_t));
    strcpy((char*)(wifi_config.sta.ssid), "NETWORKNAME");
    strcpy((char*)(wifi_config.sta.password), "NETWORKPASS");

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI("WIFI", "initialization exiting, now");
}