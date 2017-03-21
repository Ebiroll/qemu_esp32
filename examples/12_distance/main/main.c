//
// Lots of this based on ncolbans work
// https://github.com/nkolban/esp32-snippets
/*
MIT License

Copyright (c) 2017 Olof Astrand (Ebiroll)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
//#include "driver/adc.h"
#include <sys/time.h>
#include <u8g2.h>

#include "sdkconfig.h"
#include "u8g2_esp32_hal.h"


#define ECHO_PIN GPIO_NUM_4
#define TRIG_PIN GPIO_NUM_15

static const char *TAG = "distance";

// SDA - GPIO21
#define PIN_SDA 21

// SCL - GPIO22
#define PIN_SCL 22


esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}

// Similar to uint32_t system_get_time(void)
uint32_t get_usec() {

 struct timeval tv;

 //              struct timeval {
 //                time_t      tv_sec;     // seconds
 //                suseconds_t tv_usec;    // microseconds
 //              };

 gettimeofday(&tv,NULL);
 return (tv.tv_sec*1000000 + tv.tv_usec);


  //uint64_t tmp=get_time_since_boot();
  //uint32_t ret=(uint32_t)tmp;
  //return ret;
}

double avg_distance;
double all_distance[10];


//
// Toggle trig pin and wait for input on echo pin 
//
static void measure_Distance() {

    gpio_pad_select_gpio(TRIG_PIN);
    gpio_pad_select_gpio(ECHO_PIN);

    gpio_set_direction(TRIG_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(ECHO_PIN, GPIO_MODE_INPUT);

    int i=0;
    while(i<10) {
        // HC-SR04P
        gpio_set_level(TRIG_PIN, 1);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        gpio_set_level(TRIG_PIN, 0);
        uint32_t startTime=get_usec();

        while (gpio_get_level(ECHO_PIN)==0 && get_usec()-startTime < 500*1000)
        {
            // Wait until echo goes high
        }

        startTime=get_usec();

        while (gpio_get_level(ECHO_PIN)==1 && get_usec()-startTime < 500*1000)
        {
            // Wait until echo goes low again
        }

        if (gpio_get_level(ECHO_PIN) == 0)
        {
            uint32_t diff = get_usec() - startTime; // Diff time in uSecs
            // Distance is TimeEchoInSeconds * SpeeOfSound / 2
            double distance = 340.29 * diff / (1000 * 1000 * 2); // Distance in meters
            printf("Distance is %f cm\n", distance * 100);
            all_distance[i%10]=distance;
            i++;
            double tot=0.0;
            for (int j=0;j<10;j++) {
                tot=tot+all_distance[j];
            }
            avg_distance=tot/10.0;
        }
        else
        {
            // No value
            printf("Did not receive a response!\n");
        }
        // Delay and re run.
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }


}

u8g2_t u8g2; // a structure which will contain all the data for one display


void init_SSD1306i2c() {
	u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
	u8g2_esp32_hal.sda   = (gpio_num_t) PIN_SDA;
	u8g2_esp32_hal.scl  = (gpio_num_t) PIN_SCL;
	u8g2_esp32_hal_init(u8g2_esp32_hal);


	u8g2_Setup_ssd1306_128x32_univision_f(
		&u8g2,
		U8G2_R0,
		//u8x8_byte_sw_i2c,
		u8g2_esp32_msg_i2c_cb,
		u8g2_esp32_msg_i2c_and_delay_cb);  // init u8g2 structure
	    u8x8_SetI2CAddress(&u8g2.u8x8,0x78);

	ESP_LOGI(TAG, "u8g2_InitDisplay");
	u8g2_InitDisplay(&u8g2); // send init sequence to the display, display is in sleep mode after this,

	ESP_LOGI(TAG, "u8g2_SetPowerSave");
	u8g2_SetPowerSave(&u8g2, 0); // wake up display
	ESP_LOGI(TAG, "u8g2_ClearBuffer");
	u8g2_ClearBuffer(&u8g2);
	ESP_LOGI(TAG, "u8g2_DrawBox");
	u8g2_DrawBox(&u8g2, 0, 2, 80,6);
	u8g2_DrawFrame(&u8g2, 0,2,100,6);

	ESP_LOGI(TAG, "u8g2_SetFont");
    u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);
	ESP_LOGI(TAG, "u8g2_DrawStr");
    u8g2_DrawStr(&u8g2, 2,26,"Hej Alla!");
	ESP_LOGI(TAG, "u8g2_SendBuffer");
	u8g2_SendBuffer(&u8g2);

	ESP_LOGI(TAG, "All Init!");

}

void display_Avg() {
    char Buff[128];

	ESP_LOGI(TAG, "u8g2_ClearBuffer");
	u8g2_ClearBuffer(&u8g2);
	ESP_LOGI(TAG, "u8g2_DrawBox");
	u8g2_DrawBox(&u8g2, 0, 2, avg_distance * 100,6);
	u8g2_DrawFrame(&u8g2, 0,2,100,6);

    sprintf(Buff,"%.2f cm", avg_distance * 100);

	ESP_LOGI(TAG, "u8g2_SetFont");
    u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);
	ESP_LOGI(TAG, "u8g2_DrawStr");
    u8g2_DrawStr(&u8g2, 2,26,Buff);
	ESP_LOGI(TAG, "u8g2_SendBuffer");
	u8g2_SendBuffer(&u8g2);


}



void app_main(void)
{
    nvs_flash_init();
    init_SSD1306i2c();

    while (true) {
        measure_Distance();
        display_Avg();
    }


#if 0
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    wifi_config_t sta_config = {
        .sta = {
            .ssid = "access_point_name",
            .password = "password",
            .bssid_set = false
        }
    };
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &sta_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    ESP_ERROR_CHECK( esp_wifi_connect() );
#endif


}
