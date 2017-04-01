#include "esp_log.h"
#include "fonts.h"
#include "ssd1306.hpp"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
using namespace std;

OLED oled = OLED(GPIO_NUM_19, GPIO_NUM_22, SSD1306_128x64);

void myTask(void *pvParameters) {
	adc1_config_width(ADC_WIDTH_12Bit);
	adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_0db);

	ostringstream os;
//	char *data = (char*) malloc(32);
	uint16_t graph[128];
	memset(graph, 0, sizeof(graph));
//	for(uint8_t i=0;i<64;i++){
//		oled.clear();
//		oled.draw_hline(0, i, 100, WHITE);
//		sprintf(data,"%d",i);
//		oled.draw_string(105, 10, string(data), BLACK, WHITE);
//		oled.refresh(false);
//		vTaskDelay(1000 / portTICK_PERIOD_MS);
//	}
	while (1) {
		os.str("");
		os << "ADC_CH4(GPIO32):" << adc1_get_voltage(ADC1_CHANNEL_4);
		for (int i = 0; i < 127; i++) {
			graph[i] = graph[i + 1];
		}
		graph[127] = adc1_get_voltage(ADC1_CHANNEL_4);
		oled.clear();
//		sprintf(data, "01");
//		oled.select_font(2);
//		oled.draw_string(0, 0, string(data), WHITE, BLACK);
//		sprintf(data, ":%d", graph[127]);
		oled.select_font(1);
		oled.draw_string(0, 0, os.str(), WHITE, BLACK);
//		oled.draw_string(33, 4, string(data), WHITE, BLACK);
		graph[127] = graph[127] * 48 / 4096;
		for (uint8_t i = 0; i < 128; i++) {
			oled.draw_pixel(i, 63 - graph[i], WHITE);
		}
		oled.draw_pixel(127, 63 - graph[127], WHITE);
		oled.refresh(true);
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}

}

#ifdef __cplusplus
extern "C" {
#endif
void app_main() {

	oled = OLED(GPIO_NUM_19, GPIO_NUM_22, SSD1306_128x64);
	if (oled.init()) {
		ESP_LOGI("OLED", "oled inited");
//		oled.draw_rectangle(10, 30, 20, 20, WHITE);
//		oled.select_font(0);
//		//deprecated conversion from string constant to 'char*'
//		oled.draw_string(0, 0, "glcd_5x7_font_info", WHITE, BLACK);
//		ESP_LOGI("OLED", "String length:%d",
//				oled.measure_string("glcd_5x7_font_info"));
//		oled.select_font(1);
//		oled.draw_string(0, 18, "tahoma_8pt_font_info", WHITE, BLACK);
//		ESP_LOGI("OLED", "String length:%d",
//				oled.measure_string("tahoma_8pt_font_info"));
//		oled.draw_string(55, 30, "Hello ESP32!", WHITE, BLACK);
//		oled.refresh(true);
//		vTaskDelay(3000 / portTICK_PERIOD_MS);
		xTaskCreatePinnedToCore(&myTask, "adctask", 2048, NULL, 5, NULL, 1);
	} else {
		ESP_LOGE("OLED", "oled init failed");
	}
}
#ifdef __cplusplus
}
#endif
