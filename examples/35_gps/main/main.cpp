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
#include <string.h>
#include <sys/time.h>


static const char *TAG = "uart";


// readline from readLine.c
extern "C" char *readLine(uart_port_t uart,char *line,int len);
extern "C" char *pollLine(uart_port_t uart,char *line,int len);

#define BUF_SIZE 512

char echoLine[512];



// This task only prints what is received on UART1
static void uartECHOTask(void *inpar) {
//        rxPin = 13;
//        txPin = 35;
  char* data;

  uart_port_t uart_num = UART_NUM_1;                                     //uart port number
  uart_config_t uart_config = {
      .baud_rate = 9600,                     //baudrate
      .data_bits = UART_DATA_8_BITS,          //data bit mode
      .parity = UART_PARITY_DISABLE,          //parity mode
      .stop_bits = UART_STOP_BITS_1,          //stop bit mode
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,  //hardware flow control(cts/rts)
      .rx_flow_ctrl_thresh = 122,             //flow control threshold
  };
  ESP_LOGI(TAG, "Setting UART configuration number %d...", uart_num);
  ESP_ERROR_CHECK( uart_param_config(uart_num, &uart_config));
  QueueHandle_t uart_queue;
  ESP_ERROR_CHECK( uart_set_pin(uart_num, 12, 13, -1, -1));
  ESP_ERROR_CHECK( uart_driver_install(uart_num, 512 * 2, 512 * 2, 10,  &uart_queue,0));

  printf("ESP32 uart echo\n");

  while(1) {
     //data=readLine(uart_num,echoLine,256);
     //vTaskDelay(100 / portTICK_PERIOD_MS);
     int size = uart_read_bytes(uart_num, (unsigned char *)echoLine, 1, portMAX_DELAY);
     printf("%c",echoLine[0]);
  }
}

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}

// Similar to uint32_t system_get_time(void)
uint32_t get_usec() {

 struct timeval tv;
 gettimeofday(&tv,NULL);
 return (tv.tv_sec*1000000 + tv.tv_usec);
 //uint64_t tmp=get_time_since_boot();
 //uint32_t ret=(uint32_t)tmp;
 //return ret;
}


extern "C" void app_main(void)
{
    nvs_flash_init();
    xTaskCreatePinnedToCore(&uartECHOTask, "gps", 4096, NULL, 20, NULL, 0);


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
