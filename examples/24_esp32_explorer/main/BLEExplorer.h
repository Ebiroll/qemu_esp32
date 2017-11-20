/*
 * BLEExplorer.h
 *
 *  Created on: Sep 27, 2017
 *      Author: kolban
 */

#ifndef MAIN_BLEEXPLORER_H_
#define MAIN_BLEEXPLORER_H_
#include <freertos/FreeRTOS.h>
#include "JSON.h"
#include "BLEAdvertisedDevice.h"

class BLEExplorer {
public:
	BLEExplorer();
	virtual ~BLEExplorer();
	JsonArray scan();
	JsonArray connect(std::string addr);
	JsonObject enumerateDevices(BLEAdvertisedDevice device);
	JsonObject enumerateCharacteristics(BLERemoteService *p, std::map<uint16_t, BLERemoteCharacteristic*> *charact, std::string _addr);

	// --- SERVER --- //
	void createServer(std::string name);
	void deleteServer();
	JsonObject addService(BLEUUID uuid);
	void startService(uint16_t handle);
	//void stopServer();
	void startAdvertising();
	void stopAdvertising();
	JsonObject addCharacteristic(BLEUUID uuid, uint16_t service);
	JsonObject addDescriptor(BLEUUID uuid, uint16_t characteristic);
	void setCharacteristicValue(BLEUUID uuid, std::string value);

private:
};

#endif /* MAIN_BLEEXPLORER_H_ */

