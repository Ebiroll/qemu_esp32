#include "freertos/FreeRTOS.h"
#include <freertos/task.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include <driver/spi_master.h>
#include <esp_log.h>

static char tag[] = "mpcd";

extern void task_test_SSD1306(void *ignore);


void app_main(void)
{

	xTaskCreatePinnedToCore(&task_test_SSD1306, "task_test_SSD1306", 8048, NULL, 5, NULL, 0);
}

