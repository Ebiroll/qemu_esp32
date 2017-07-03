#include <stdlib.h>
#include "JSON.h"
#include "WiFi.h"
#include <System.h>
#include <GeneralUtils.h>
#include <esp_partition.h>
#include <stdio.h>
#include <sys/time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


static JsonObject fromPartition(esp_partition_t *pPartition) {
	JsonObject obj = JSON::createObject();
	obj.setInt("type",    pPartition->type);
	obj.setInt("subType", pPartition->subtype);
	obj.setInt("size",    pPartition->size);
	obj.setInt("address", pPartition->address);
	obj.setBoolean("encrypted", pPartition->encrypted);
	obj.setString("label", std::string(pPartition->label));
	return obj;
} // fromPartition


JsonObject SYSTEM_JSON() {
	JsonObject obj = JSON::createObject();
	obj.setInt("freeHeap", System::getFreeHeapSize());
	struct timeval tv;
	gettimeofday(&tv, nullptr);
	obj.setDouble("time", tv.tv_sec + tv.tv_usec / 1000000.0);
	int taskCount = uxTaskGetNumberOfTasks();
	obj.setInt("taskCount", taskCount);

	/*
	TaskStatus_t *pTaskStatusArray = (TaskStatus_t *)malloc(sizeof(TaskStatus_t) * taskCount);
	taskCount = uxTaskGetSystemState(pTaskStatusArray, taskCount, nullptr);
	free(pTaskStatusArray);
	*/

	// Get a list of all partitions ...
	JsonArray arr = JSON::createArray();
	esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);
	esp_partition_t *pPartition;
	while(it != NULL) {
		pPartition = (esp_partition_t *)esp_partition_get(it);
		arr.addObject(fromPartition(pPartition));
		it = esp_partition_next(it);
	}
	esp_partition_iterator_release(it);

	it = esp_partition_find(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, NULL);
	while(it != NULL) {
		pPartition = (esp_partition_t *)esp_partition_get(it);
		arr.addObject(fromPartition(pPartition));
		it = esp_partition_next(it);
	}
	esp_partition_iterator_release(it);
	obj.setArray("partitions", arr);
	return obj;
} // SYSTEM_JSON
