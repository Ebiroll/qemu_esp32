#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "esp_log.h"
static const char *TAG = "test queues";

QueueHandle_t my_queue;

static void vSenderTask( void *pvParameters )
{
    int32_t lValueToSend; BaseType_t xStatus;
    lValueToSend = (int32_t) pvParameters;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(500));
        xStatus = xQueueSendToBack(my_queue, &lValueToSend, 0);
        if( xStatus != pdPASS )
        {
            ESP_LOGI(TAG, "Could not send to the queue.");
        }
    }
}

static void vReceiverTask( void *pvParameters )
{
    int32_t lReceivedValue;
    BaseType_t xStatus;
    const TickType_t xTicksToWait = pdMS_TO_TICKS(1100);
    while (1) {
        if( uxQueueMessagesWaiting( my_queue ) != 0 ) {
            ESP_LOGI(TAG, "Queue should have been empty!");
        }

        xStatus = xQueueReceive ( my_queue, &lReceivedValue, xTicksToWait );
        if( xStatus == pdPASS ) {
            ESP_LOGI(TAG, "Received = %d", lReceivedValue);
        } else {
            ESP_LOGI(TAG,  "Could not receive from the queue.");
        }
    }
}

void app_main( void )
{
    nvs_flash_init();
    my_queue = xQueueCreate( 5, sizeof( int32_t ) );
    if( my_queue != NULL ) {
        xTaskCreate( vSenderTask, "Sender1", 3000, ( void * ) 100, 1, NULL );
        xTaskCreate( vSenderTask, "Sender2", 3000, ( void * ) 200, 1, NULL );
        xTaskCreate( vReceiverTask, "Receiver", 3000, NULL, 2, NULL );
    } else {
        ESP_LOGI(TAG, "queue not created");
    }
}
