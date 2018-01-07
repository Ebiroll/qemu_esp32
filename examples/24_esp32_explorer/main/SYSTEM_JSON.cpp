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

const static char LOG_TAG[] = "SYSTEM_JSON";
/**
 * @brief Obtain a JSON object describing a partition.
 *
 * {
 *   "type":
 *   "subType":
 *   "size":
 *   "address":
 *   "encrypted":
 *   "label":
 * }
 */
static JsonObject fromPartition(esp_partition_t *pPartition) {
	JsonObject obj = JSON::createObject();
	obj.setInt("type",          pPartition->type);                // type
	obj.setInt("subType",       pPartition->subtype);             // subType
	obj.setInt("size",          pPartition->size);                // size
	obj.setInt("address",       pPartition->address);             // address
	obj.setBoolean("encrypted", pPartition->encrypted);           // encrypted
	obj.setString("label",      std::string(pPartition->label));  // label
	return obj;
} // fromPartition


/**
 * @brief Create a JSON object from a TaskStatus_t.
 *
 * {
 *    "name":
 *    "stackHighWater":
 *    "priority":
 *    "taskNumber":
 *    "state":
 * }
 * @param [in] pTaskStatus The task status to turn into a JSON object.
 * @return A JSON object from a TaskStatus_t.
 */
static JsonObject fromTaskStatus(TaskStatus_t *pTaskStatus) {
	JsonObject obj = JSON::createObject();
	obj.setString("name",        pTaskStatus->pcTaskName);             // name
	obj.setInt("stackHighWater", pTaskStatus->usStackHighWaterMark);   // stackHighWater
	obj.setInt("priority",       pTaskStatus->uxCurrentPriority);      // priority
	obj.setInt("taskNumber",     pTaskStatus->xTaskNumber);            // taskNumber
	std::string state = "";
	switch(pTaskStatus->eCurrentState) {
		case eReady: {
			state = "Ready";
			break;
		}
		case eRunning: {
			state = "Running";
			break;
		}
		case eBlocked: {
			state = "Blocked";
			break;
		}
		case eSuspended: {
			state = "Suspended";
			break;
		}
		case eDeleted: {
			state = "Deleted";
			break;
		}
	}
	obj.setString("state", state);             // state
	return obj;
} // fromTaskStatus


/**
 * @brief Create the JSON object to return to the caller that provides us information
 * about the system as a whole.
 *
 * The returned object contains:
 * {
 *   "model":          // ESP32 model number
 *   "cores":          // Number of ESP32 cores
 *   "revision":       // ESP32 revision
 *   "hasEmbeddedFlash":
 *   "hasWiFi":        // Is WiFi present?
 *   "hasBLE":         // Is BLE present?
 *   "hasBT":          // Is BT classic present?
 *   "espVersion":     // Version of ESP-IDF
 *   "freeHeap":       // Amount of heap free
 *   "time":           // Time since boot
 *   "taskCount":      // Number of FreeRTOS tasks
 *   "partitions":     // Partitions structure
 *   [
 *     {
 *       "type":
 *       "subType":
 *       "size":
 *       "address":
 *       "encrypted":
 *       "label":
 *     },
 *     ...
 *   ]
 *   "taskStatus": [
 *     {
 *       "name":
 *       "stackHighWater":
 *       "priority":
 *       "taskNumber":
 *       "state":
 *     },
 *     ...
 *   ]
 * }
 *
 * @return A JSON Object describing the data.
 */
JsonObject SYSTEM_JSON() {
	JsonObject obj = JSON::createObject();
	esp_chip_info_t chipInfo;
	esp_chip_info(&chipInfo);
	obj.setInt("model", chipInfo.model);
	obj.setInt("cores", chipInfo.cores);
	obj.setInt("revision", chipInfo.revision);
	obj.setBoolean("hasEmbeddedFlash", chipInfo.features & CHIP_FEATURE_EMB_FLASH);
	obj.setBoolean("hasWifi", chipInfo.features & CHIP_FEATURE_WIFI_BGN);
	obj.setBoolean("hasBLE", chipInfo.features & CHIP_FEATURE_BLE);
	obj.setBoolean("hasBT", chipInfo.features & CHIP_FEATURE_BT);
	obj.setString("espVersion", System::getIDFVersion());
	obj.setInt("freeHeap", System::getFreeHeapSize());
	//obj.setInt("minimumFreeHeap", System::getMinimumFreeHeapSize());
	struct timeval tv;
	gettimeofday(&tv, nullptr);
	obj.setDouble("time", tv.tv_sec + tv.tv_usec / 1000000.0);
	int taskCount = uxTaskGetNumberOfTasks();
	obj.setInt("taskCount", taskCount);

#if( configUSE_TRACE_FACILITY == 1 )
	TaskStatus_t *pTaskStatusArray = new TaskStatus_t[taskCount];
	assert(pTaskStatusArray != nullptr);
	taskCount = ::uxTaskGetSystemState(pTaskStatusArray, taskCount, nullptr);
	std::sort(pTaskStatusArray, pTaskStatusArray+taskCount, [](const TaskStatus_t& a, const TaskStatus_t& b) {
		return strcasecmp(a.pcTaskName, b.pcTaskName) < 0;
	});

	JsonArray arr2 = JSON::createArray();
	for (int i=0; i<taskCount; i++) {
		/*
		ESP_LOGD(LOG_TAG, "Task name: %s, stack high water: %d, runtime counter: %d, current priority: %d, task number: %d", pTaskStatusArray[i].pcTaskName,
			pTaskStatusArray[i].usStackHighWaterMark,
			pTaskStatusArray[i].ulRunTimeCounter,
			pTaskStatusArray[i].uxCurrentPriority,
			pTaskStatusArray[i].xTaskNumber
		);
		*/
		arr2.addObject(fromTaskStatus(&pTaskStatusArray[i]));
	}
	obj.setArray("taskStatus", arr2);
	delete[] pTaskStatusArray;
#endif


	// Get a list of all partitions ...
	JsonArray arr = JSON::createArray();

	// Find all the app partitions.
	esp_partition_iterator_t it = ::esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);
	esp_partition_t *pPartition;
	while(it != NULL) {
		pPartition = (esp_partition_t *)::esp_partition_get(it);
		arr.addObject(fromPartition(pPartition));
		it = esp_partition_next(it);
	}
	esp_partition_iterator_release(it);

	// Find all the data partitions ...
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
