/*
 *  Copyright (c) 2016 - 2026 MaiKe Labs
 *
 *  Written by Jack Tan <jiankemeng@gmail.com>
 *
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
*/
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "esp_event_loop.h"
#include "esp_wifi.h"
#include "esp_log.h"

#define MAX_APs 20
#define	WIFI_CHANNEL_MAX		(13)

unsigned int chanel=1;

// From auth_mode code to string
static char* getAuthModeName(wifi_auth_mode_t auth_mode) {
	
	char *names[] = {"OPEN", "WEP", "WPA PSK", "WPA2 PSK", "WPA WPA2 PSK", "MAX"};
	return names[auth_mode];
}

wifi_ap_record_t ap_records[MAX_APs];


void scan_ap_task(void *pvParameters)
{
	uint8_t channel = 1;
	uint16_t ap_num = MAX_APs;
	uint16_t n = 0;
	wifi_scan_config_t scan_config = {
			.ssid = 0,
			.bssid = 0,
			.channel = 0,
			.show_hidden = true
	};

    while (1) {

		// Setting this has no effect
		//scan_config.channel = chanel;		


		ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
		printf(" Completed scan!\n");
 
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, ap_records));
        printf("Found %d access points:\n", ap_num);

			for (uint16_t i = 0; i < ap_num; i++) {
				uint8_t *bi = ap_records[i].bssid;
				printf("%32s %2d%s (%02x:%02x:%02x:%02x:%02x:%02x) rssi: %02d %s auth: %12s\r\n",
						ap_records[i].ssid,
						ap_records[i].primary,(ap_records[i].second>0) ? "(HT40)" : "      ",
						MAC2STR(bi),
						ap_records[i].rssi,
						(/*ap_records[i].low_rate_enable==1*/ false) ? " LR " : "    " ,
						getAuthModeName(ap_records[i].authmode)
					);
			}

		printf("------------ delay 6s to start a new scanning ... --------------\r\n");
		channel = (channel % WIFI_CHANNEL_MAX) + 1;
		//wifi_set_channel(channel);
		printf(" Channel %d\n",channel);

        vTaskDelay(6000 / portTICK_PERIOD_MS);
    }
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
	return 0;
}

static void wifi_init(void)
{
	const uint8_t protocol = WIFI_PROTOCOL_LR;

	tcpip_adapter_init();
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_protocol(WIFI_IF_STA,protocol));

	ESP_ERROR_CHECK(esp_wifi_start());
}

void app_main()
{
	nvs_flash_init();
	//system_init();
	wifi_init();
	printf("WiFi AP SSID Scanning... \r\n");
    xTaskCreatePinnedToCore(&scan_ap_task, "scan_ap_task", 8*1024, NULL, 5, NULL, 0);
}
