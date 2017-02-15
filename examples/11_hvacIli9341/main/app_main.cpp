#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/spi_reg.h"
#include "driver/hspi.h"


#include "soc/gpio_reg.h"
#include "driver/gpio.h"
#include "esp_attr.h"                                  
#include "Adafruit_ILI9341_fast_as.h"

float target_room_temperature = 23.5;
float RW_temperature = 65;
float target_RW_temperature = 70;
float room1_temperature = 23.4;
float room2_temperature = 23.3;
float outside_temperature = 2.4;
float min_target_temp = 18;
float max_target_temp = 26;
bool heater_enabled = false;
unsigned long room1_updated = -1;
unsigned long room2_updated = -1;
unsigned long outside_updated = -1;
unsigned long heater_state_changed_time = 0;
time_t total_on_time = 1;
time_t total_off_time = 1;
time_t last24h_on_time = 1;
time_t last24h_off_time = 1;
time_t cursecs = 0;

Adafruit_ILI9341 tft;

extern void setupUI();
extern void updateScreen(bool);

//void lcd_debug(void *pvParameters)
//{
//	tft.begin();
//	tft.fillScreen(ILI9341_GREEN);
//	setupUI();
////	vTaskDelete(NULL);
//}

extern "C" void app_main(void* arg) 
{
	tft.begin();
	tft.fillScreen(ILI9341_WHITE);
	vTaskDelay(250 / portTICK_RATE_MS);
	tft.fillScreen(ILI9341_RED);
	vTaskDelay(250 / portTICK_RATE_MS);
	tft.fillScreen(ILI9341_GREEN);
	vTaskDelay(250 / portTICK_RATE_MS);
	tft.fillScreen(ILI9341_BLUE);
	vTaskDelay(250 / portTICK_RATE_MS);
	tft.fillScreen(ILI9341_BLACK);
	vTaskDelay(250 / portTICK_RATE_MS);
	setupUI();
	ets_printf("LCD_init done....\n");	
	while (1) {
	    //xTaskCreate(lcd_debug,"lcd_debug", 4096, NULL, 3, NULL);
	    //vTaskDelete(NULL);
		ets_printf("Pause\n");	
	    vTaskDelay(250 / portTICK_RATE_MS);
		if (heater_enabled) {
	    	target_room_temperature += 0.1;
	    	if (target_room_temperature >= 30.0) {
	    		heater_enabled = false;
	    		heater_state_changed_time = cursecs;
	    	}
	    }
	    else {
	    	target_room_temperature -= 0.1;		    	
	    	if (target_room_temperature <= 20.0) {
	    		heater_enabled = true;
	    		heater_state_changed_time = cursecs;
	    	}
		}
	    updateScreen(false);
    }
}
