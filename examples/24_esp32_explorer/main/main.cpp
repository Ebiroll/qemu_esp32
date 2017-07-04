
#include "sdkconfig.h"
#include <esp_log.h>
#include <FATFS_VFS.h>
#include <FileSystem.h>
#include <FreeRTOS.h>
#include <Task.h>
#include <WebServer.h>
#include <WiFiEventHandler.h>
#include <WiFi.h>
#include <stdio.h>
#include <string>
#include "ESP32Explorer.h"
extern "C" {
#include "emul_ip.h"
}

static char tag[] = "ESP32_Explorer_MAIN";

class ESP32_ExplorerTask: public Task {
	void run(void *data) override {
		ESP32_Explorer *pESP32_Explorer = new ESP32_Explorer();
		pESP32_Explorer->start();
	}
};

class TargetWiFiEventHandler: public WiFiEventHandler {
	 esp_err_t staGotIp(system_event_sta_got_ip_t event_sta_got_ip) {
			ESP_LOGD(tag, "MyWiFiEventHandler(Class): staGotIp");
			ESP32_ExplorerTask *pESP32_ExplorerTask = new ESP32_ExplorerTask();
			pESP32_ExplorerTask->setStackSize(8000);
			pESP32_ExplorerTask->start();
			return ESP_OK;
	 }
};

class WiFiTask: public Task {
	void run(void *data) override {
		if (is_running_qemu()) {
			printf("Running in qemu\n");
			xTaskCreatePinnedToCore(task_lwip_init, "loop", 4096, NULL, 14, NULL, 0);
			//task_lwip_init(NULL);
			ESP32_ExplorerTask *pESP32_ExplorerTask = new ESP32_ExplorerTask();
			pESP32_ExplorerTask->setStackSize(8000);
			pESP32_ExplorerTask->start();

		}
		else {
			WiFi wifi;
			TargetWiFiEventHandler *eventHandler = new TargetWiFiEventHandler();
			wifi.setWifiEventHandler(eventHandler);
			wifi.setIPInfo("192.168.1.99", "192.168.1.1", "255.255.255.0");
			wifi.connectAP("ssid", "password");
		}
	} // End run
};


void task_webserver(void *ignore) {
	WiFiTask *pMyTask = new WiFiTask();
	pMyTask->setStackSize(8000);
	pMyTask->start();
	FreeRTOS::deleteTask();
}

extern "C" {
	int app_main(void);
}
int app_main(void) {
	task_webserver(nullptr);
	return 0;
}
