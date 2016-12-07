/*
 * MaiKe Labs (2016 - 2026)
 *
 * Written by Jack Tan <jiankemeng@gmail.com>
 *
 * This example code is in the Public Domain (or CC0 licensed, at your option.)
 * Unless required by applicable law or agreed to in writing, this
 * software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
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

uint8_t temprature_sens_read();	
uint32_t hall_sens_read();

void read_sens_task(void *pvParameters)
{
	while (1) {
		printf("ESP32 onchip Temperature = %d\n", temprature_sens_read());
		printf("ESP32 onchip Hall sensor = %d\n", hall_sens_read());
		vTaskDelay(4000 / portTICK_PERIOD_MS);
	}
}

void app_main()
{
	nvs_flash_init();
	system_init();
	printf("Welcome to Noduino Quantum\r\n");
	printf("Try to read the sensors inside the ESP32 chip ... \r\n");
	xTaskCreatePinnedToCore(&read_sens_task, "read_sens_task", 1024, NULL, 5,
				NULL, 0);
}
