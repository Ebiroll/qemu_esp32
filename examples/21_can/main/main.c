/**
 * @section License
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017, Thomas Barth, barth-dev.de
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * \version 0.1
 * \brief A basic CAN driver for the Espressif ESP32
 *
 * Within this demo, the CAN module of the ESP32 is configured to accept any CAN message and loop it back to the bus.
 * CAN RX frames are processed by an FreeRTOS task.
 * The CAN Module is assigned to pin 4 for RX and pin 5 for TX in this demo. Please note that not all pins can be used for CAN.
 * First try to set up the system with the given pins, then change to your configuration. Also note that you will need a CAN transceiver (like the MCP2551) to connect the chip to the bus.
 * \see http://www.barth-dev.de/can-driver-esp32/
 */

#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"

#include "CAN.h"

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}


void task_CAN( void *pvParameters ){
    (void)pvParameters;

    //frame buffer
    CAN_frame_t __RX_frame;

    //create CAN RX Queue
    CAN_cfg.rx_queue = xQueueCreate(10,sizeof(CAN_frame_t));

    //start CAN Module
    CAN_init();

    while (1){
        //receive next CAN frame from queue
        if(xQueueReceive(CAN_cfg.rx_queue,&__RX_frame, 3*portTICK_PERIOD_MS)==pdTRUE){

        	//do stuff!
        	printf("New Frame form 0x%08x, DLC %d, dataL: 0x%08x, dataH: 0x%08x \r\n",__RX_frame.MsgID,  __RX_frame.DLC, __RX_frame.data.u32[0],__RX_frame.data.u32[1]);

        	//loop back frame
        	CAN_write_frame(&__RX_frame);
        }
    }
}


void app_main(void)
{
    nvs_flash_init();
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

    //Create CAN receive task
    xTaskCreate(&task_CAN, "CAN", 2048, NULL, 5, NULL);

}

