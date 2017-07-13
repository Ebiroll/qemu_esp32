/*
 * This file is part of the Black Magic Debug project.
 *
 * Copyright (C) 2011  Black Sphere Technologies Ltd.
 * Written by Gareth McMullin <gareth@blacksphere.co.nz>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "general.h"
#include "gdb_if.h"
//#include "version.h"

#include "gdb_packet.h"
#include "gdb_main.h"
#include "target.h"
//#include "exception.h"
#include "gdb_packet.h"
//#include "morse.h"

#include <assert.h>
#include <sys/time.h>
#include <sys/unistd.h>

//#include "esp/uart.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "espressif/esp_wifi.h"
//#include "ssid_config.h"

//#include <dhcpserver.h>

#define ACCESS_POINT_MODE
#define AP_SSID	 "blackmagic"
#define AP_PSK	 "blackmagic"

void platform_init(int argc, char **argv)
{
	(void) argc;
	(void) argv;
	/*
	gpio_set_iomux_function(3, IOMUX_GPIO3_FUNC_GPIO);
	gpio_set_iomux_function(2, IOMUX_GPIO2_FUNC_GPIO);

	gpio_clear(_, SWCLK_PIN);
	gpio_clear(_, SWDIO_PIN);

	gpio_enable(SWCLK_PIN, GPIO_OUTPUT);
	gpio_enable(SWDIO_PIN, GPIO_OUTPUT);
	*/

	assert(gdb_if_init() == 0);
}

void platform_srst_set_val(bool assert)
{
	(void)assert;
}

bool platform_srst_get_val(void) { return false; }

const char *platform_target_voltage(void)
{
	return "not supported";
}

uint32_t platform_time_ms(void)
{
	return xTaskGetTickCount() / portTICK_RATE_MS;
}

#define vTaskDelayMs(ms)	vTaskDelay((ms)/portTICK_RATE_MS)

void platform_delay(uint32_t ms)
{
	vTaskDelayMs(ms);
}

int platform_hwversion(void)
{
	return 0;
}

/* This is a transplanted main() from main.c */
void main_task(void *parameters)
{
	(void) parameters;

	platform_init(0, NULL);

	while (true) {

	               //volatile struct exception e;
		       //TRY_CATCH(e, EXCEPTION_ALL) {
		       gdb_main();
		       //}
		       //if (e.type) {
		       //gdb_putpacketz("EFF");
		       //target_list_free();
		       //morse("TARGET LOST.", 1);
		       //}
	}

	/* Should never get here */
}

void user_init(void)
{
#if 0
	uart_set_baud(0, 460800);
	printf("SDK version:%s\n", sdk_system_get_sdk_version());

#ifndef ACCESS_POINT_MODE
	struct sdk_station_config config = {
		.ssid = WIFI_SSID,
		.password = WIFI_PASS,
	};

	sdk_wifi_set_opmode(STATION_MODE);
	sdk_wifi_station_set_config(&config);
#else

	/* required to call wifi_set_opmode before station_set_config */
	sdk_wifi_set_opmode(SOFTAP_MODE);

	struct ip_info ap_ip;
	IP4_ADDR(&ap_ip.ip, 172, 16, 0, 1);
	IP4_ADDR(&ap_ip.gw, 0, 0, 0, 0);
	IP4_ADDR(&ap_ip.netmask, 255, 255, 0, 0);
	sdk_wifi_set_ip_info(1, &ap_ip);

	struct sdk_softap_config ap_config = {
		.ssid = AP_SSID,
		.ssid_hidden = 0,
		.channel = 3,
		.ssid_len = strlen(AP_SSID),
		.authmode = AUTH_OPEN, //AUTH_WPA_WPA2_PSK,
		.password = AP_PSK,
		.max_connection = 3,
		.beacon_interval = 100,
	};
	sdk_wifi_softap_set_config(&ap_config);

	ip_addr_t first_client_ip;
	IP4_ADDR(&first_client_ip, 172, 16, 0, 2);
	dhcpserver_start(&first_client_ip, 4);

#endif
#endif
	xTaskCreate(&main_task, (const char *)"main", 4*256, NULL, 2, NULL);
}
