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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "driver/gpio.h"
#include "sdkconfig.h"

#define BLINK_GPIO 27

void study_task(void *pvParameters)
{
    gpio_pad_select_gpio(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

	while (1) {
        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(1000 / portTICK_RATE_MS);
        gpio_set_level(BLINK_GPIO, 1);
        vTaskDelay(1000 / portTICK_RATE_MS);
        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(1000 / portTICK_RATE_MS);
        gpio_set_level(BLINK_GPIO, 1);
        vTaskDelay(1000 / portTICK_RATE_MS);

		/* wakeup from deep sleep after 8 seconds */
		system_deep_sleep(1000*1000*8);
	}
}

void app_main()
{
	printf("Welcome to Noduino Quantum\r\n");
	printf("Try to investigate the ULP/RTC of ESP32 ... \r\n");
	xTaskCreatePinnedToCore(&study_task, "study_task", 1024, NULL, 5,
				NULL, 0);
}
