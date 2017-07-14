
#include "sdkconfig.h"
#include <esp_log.h>
#include "esp_vfs.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <WebServer.h>
#include <stdio.h>
#include <string>

extern "C" {
#include "emul_ip.h"
}

static char tag[] = "ESP32_Webspiff_MAIN";


extern "C" {
	int app_main(void);
}

int app_main(void) {
		if (is_running_qemu()) {
			printf("Running in qemu\n");
			xTaskCreatePinnedToCore(task_lwip_init, "loop", 4096, NULL, 14, NULL, 0);

		}
		else {
		}
	return 0;
}
