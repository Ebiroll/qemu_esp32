//file: main.cpp
#include "Arduino.h"

extern "C" {
#include "ssd1366.h"
}

extern "C" void app_main()
{
  initArduino();
  i2c_master_init();
	ssd1306_init();

	//xTaskCreate(&task_ssd1306_display_pattern, "ssd1306_display_pattern",  2048, NULL, 6, NULL);
	xTaskCreate(&task_ssd1306_display_clear, "ssd1306_display_clear",  2048, NULL, 6, NULL);
	vTaskDelay(100/portTICK_PERIOD_MS);
	xTaskCreate(&task_ssd1306_display_text, "ssd1306_display_text",  2048,
    (void *)"Hello world!\nMulitine is OK!\nAnother line", 6, NULL);
    
  //xTaskCreate(&task_ssd1306_scroll, "ssid1306_scroll", 2048, NULL, 6, NULL);
    
  setup();
  for(;;) {
    loop();
  }
}
