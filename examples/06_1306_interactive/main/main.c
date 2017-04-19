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
#include <sys/time.h>
#include "1306.h"
#include "esp32_i2c.h"
#include "menu.h"
#include "emul_ip.h"

#include <string.h>

static const char *tag = "1306";



// readline from readLine.c
extern char *readLine(uart_port_t uart,char *line,int len);
extern char *pollLine(uart_port_t uart,char *line,int len);



#define BUF_SIZE 512

char echoLine[512];

extern unsigned short echo_port;

/**
 * An ESP32 WiFi event handler.
 * The types of events that can be received here are:
 *
 * SYSTEM_EVENT_AP_PROBEREQRECVED
 * SYSTEM_EVENT_AP_STACONNECTED
 * SYSTEM_EVENT_AP_STADISCONNECTED
 * SYSTEM_EVENT_AP_START
 * SYSTEM_EVENT_AP_STOP
 * SYSTEM_EVENT_SCAN_DONE
 * SYSTEM_EVENT_STA_AUTHMODE_CHANGE
 * SYSTEM_EVENT_STA_CONNECTED
 * SYSTEM_EVENT_STA_DISCONNECTED
 * SYSTEM_EVENT_STA_GOT_IP
 * SYSTEM_EVENT_STA_START
 * SYSTEM_EVENT_STA_STOP
 * SYSTEM_EVENT_WIFI_READY
 */
static esp_err_t esp32_wifi_eventHandler(void *ctx, system_event_t *event) {
	// Your event handling code here...
	switch(event->event_id) {
		// When we have started being an access point, then start being a web server.
		case SYSTEM_EVENT_AP_START: 
			break;

		// If we fail to connect to an access point as a station, become an access point.
		case SYSTEM_EVENT_STA_DISCONNECTED: 
			ESP_LOGD(tag, "Station disconnected started");
			// We think we tried to connect as a station and failed! ... become
			// an access point.
			break;

		// If we connected as a station then we are done and we can stop being a
		// web server.
		case SYSTEM_EVENT_STA_GOT_IP: 
			ESP_LOGD(tag, "********************************************");
			ESP_LOGD(tag, "* We are now connected to AP")
			ESP_LOGD(tag, "* - Our IP address is: " IPSTR, IP2STR(&event->event_info.got_ip.ip_info.ip));
			ESP_LOGD(tag, "********************************************");
            {
                u32_t *my_ip=(u32_t *)&event->event_info.got_ip.ip_info.ip;
                ssd1306_128x64_noname_powersave_off();

                unsigned char a=(*my_ip >> 24) & 0xff;
                display_three_numbers(a,12,0);
                a=(*my_ip >> 16) & 0xff;
                display_dot(3);
                display_three_numbers(a,8,0);
                a=(*my_ip >> 8) & 0xff;
                display_dot(7);
                display_three_numbers(a,4,0);
                a=(*my_ip) & 0xff;
                display_dot(11);
                display_three_numbers(a,0,0);                

		Set_Page_Address(4);
		Set_Column_Address(2);

		unsigned char port_hi=echo_port/1000;
		unsigned char port_lo=echo_port%100;

                display_three_numbers(port_hi,0,3);                
                display_three_numbers(port_lo,3,3);                

            }

            xTaskCreate(&menu_application_thread, "menu thread", 2048, NULL, 5, NULL);

			break;

		default: // Ignore the other event types
			break;
	} // Switch event

	return ESP_OK;
} // esp32_wifi_eventHandler



static void initialise_wifi(void)
{
    tcpip_adapter_init();
    //wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(esp32_wifi_eventHandler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
	        //#include "secret.i"
	        .ssid = "ssid",
            .password = "password",
        },
    };
    ESP_LOGI(tag, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    ESP_ERROR_CHECK( esp_wifi_connect() );

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


//
// Menu from uart 1
//
static void uartLoopTask(void *inpar)
{
    QueueHandle_t uart_queue;
    char* line=malloc(1024*2);
    int display_number=123;
    unsigned char display_data[16];

    // Could not get to work with UART0 
    uart_port_t uart_num = UART_NUM_1; // uart port number
    uart_config_t uart_config = {
        .baud_rate = 115200,                   //baudrate
        .data_bits = UART_DATA_8_BITS,         //data bit mode
        .parity = UART_PARITY_DISABLE,         //parity mode
        .stop_bits = UART_STOP_BITS_1,         //stop bit mode
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, //hardware flow control(cts/rts)
        .rx_flow_ctrl_thresh = 122,            //flow control threshold
    };
    ESP_LOGI(tag, "Setting UART configuration number %d...", uart_num);
    ESP_LOGI(tag, "Setting UART configuration number %d...", uart_num);
    ESP_ERROR_CHECK( uart_param_config(uart_num, &uart_config));

    // UART pins
    ESP_ERROR_CHECK(uart_set_pin(uart_num, 23, 19, -1, -1));

    // driver_install, otherwise E (20206) uart: uart_read_bytes(841): uart driver error
    ESP_ERROR_CHECK(uart_driver_install(uart_num, 512 * 2, 512 * 2, 10, &uart_queue, 0));
    sprintf(line,"Select 1,2,3,4 or 5\n");
    ssd1306_128x64_noname_powersave_off();
    //display_three_numbers(display_number,0);


    printMenu(line,2*1024);
    uart_tx_chars(UART_NUM_1, (const char *)line, strlen(line));
    
    printf("Uart Menu\n");

    //#if 0
    char *data;

    while (true)
    {
        line[0]=0;
        int to_send=printMenu(line,2*1024);
        uart_tx_chars(UART_NUM_1, (const char *)line, to_send);


        data = readLine(uart_num, line, 256);
		/* break if the recved message = "quit" */
		if (!strncmp(line, "quit", 4))
			break;

		if (!strncmp(line, "0", 1))
		{
			printf("i2c_scan\n");
			i2c_scan();
		}

		if (!strncmp(line, "1", 1))
		{
			clear_display();
		}

		if (!strncmp(line, "2", 1))
		{
			Set_Page_Address(0);
			Set_Column_Address(0);
		}

		if (!strncmp(line, "3", 1))
		{
			Set_Page_Address(4);
		}

		if (!strncmp(line, "4", 1))
		{
			Set_Column_Address(4);
		}

		if (!strncmp(line, "5", 1))
		{
			int j=0;
			for (j=0;j<8;j++) {
				display_data[j]=0xff;
			}
			Write_data(display_data,8);

		}

		if (!strncmp(line, "6", 1))
		{
			int j=0;
			for (j=0;j<8;j++) {
				display_data[j]=0x0f;
			}
			Write_data(display_data,8);
		}

		if (!strncmp(line, "7", 1))
		{
			display_number++;
			display_three_numbers(display_number,0,0);
		}

		if (!strncmp(line, "8", 1))
		{
			display_dot(4);
		}
    }
    //#endif
}


//
//
#define BLINK_GPIO 5

void blink_task(void *pvParameters)
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
	}
}



void app_main(void)
{
    // Before calling nvs_flash:init(),
    //(gdb) p/x GPIO_PIN_MUX_REG[0]
    //$3 = 0x3ff49044
    //(gdb) p/x GPIO_PIN_MUX_REG[21]
    //$4 = 0x3ff4907c
    //(gdb) p/x &GPIO_PIN_MUX_REG[21]
    //$5 = 0x3f408448

    //nvs_flash_init();
    i2c_init(0,0x3C);
    ssd1306_128x64_noname_init();
    //printf("t=0x%x\n",*quemu_test);
    //initialise_wifi();

    //xTaskCreatePinnedToCore(&blink_task, "blink", 4096, NULL, 20, NULL, 0);


    if (is_running_qemu()) {
        printf("Running in qemu\n");

       // Initialize event loop
       ESP_ERROR_CHECK( esp_event_loop_init(esp32_wifi_eventHandler, NULL) );
       xTaskCreatePinnedToCore(task_lwip_init, "loop", 4096, NULL, 14, NULL, 0);


    } else {
        initialise_wifi();
    }

    // Uart
    xTaskCreatePinnedToCore(&uartLoopTask, "loop", 4096, NULL, 20, NULL, 0);

    // wifi socket with 


#if 0
#endif


// 




}
