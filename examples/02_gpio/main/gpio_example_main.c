/* GPIO Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

/**
 * Brief:
 * This test code shows how to configure gpio and how to use gpio interrupt.
 *
 * GPIO status:
 * GPIO15: output
 * GPIO17: output
 *
 * Test:
 * Generate pulses on GPIO15/17, *
 */

#define GPIO_OUTPUT_IO_0    15
#define GPIO_OUTPUT_IO_1    17
#define GPIO_OUTPUT_PIN_SEL  ((1<<GPIO_OUTPUT_IO_0) | (1<<GPIO_OUTPUT_IO_1))

static xQueueHandle gpio_evt_queue = NULL;


static void gpio_task_example(void* arg)
{
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
        }
    }
}

void app_main()
{

    int level=gpio_get_level(0);
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO15/17
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    //start gpio task
    //xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    gpio_set_level(GPIO_OUTPUT_IO_0, 0);
    gpio_set_level(GPIO_OUTPUT_IO_1, 1);


    int cnt = 0;
    while(1) {
        printf("cnt: %d\n", cnt++);
        //vTaskDelay(1000 / portTICK_RATE_MS);
        //gpio_set_level(GPIO_OUTPUT_IO_0, (1+cnt) % 2);
        //gpio_set_level(GPIO_OUTPUT_IO_1, cnt % 2);
        //vTaskDelay(1000 / portTICK_RATE_MS);
        while(gpio_get_level(0)==level) {
            vTaskDelay(10 / portTICK_RATE_MS);
        }
        level=gpio_get_level(0);

        while(gpio_get_level(0)==level) {
            vTaskDelay(10 / portTICK_RATE_MS);
        }
        level=gpio_get_level(0);
        gpio_set_level(GPIO_OUTPUT_IO_0, 1);
        gpio_set_level(GPIO_OUTPUT_IO_1, 0);
        //vTaskDelay(1000 / portTICK_RATE_MS);
        // Wait until release
        while(gpio_get_level(0)==level) {
            vTaskDelay(10 / portTICK_RATE_MS);
        }
        level=gpio_get_level(0);
        while(gpio_get_level(0)==level) {
            vTaskDelay(10 / portTICK_RATE_MS);
        }
        level=gpio_get_level(0);

        gpio_set_level(GPIO_OUTPUT_IO_0, 0);
        gpio_set_level(GPIO_OUTPUT_IO_1, 1);


    }
}

