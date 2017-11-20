/*
 * BLEExplorer.cpp
 *
 *  Created on: Sep 27, 2017
 *      Author: kolban
 */

#include "BLEExplorer.h"
#include "sdkconfig.h"
#include <esp_log.h>
#include <BLEDevice.h>
#include "Task.h"
#include "Memory.h"
#include <sstream>

#include "BLEClient.h"
static const char* LOG_TAG = "BLEExplorer";
static bool isRunning = false;
static BLEServer *pServer;
static BLEServiceMap *mServices;
static BLECharacteristicMap *mCharacteristics;

BLEExplorer::BLEExplorer() {
	// TODO Auto-generated constructor stub
}

BLEExplorer::~BLEExplorer() {
	// TODO Auto-generated destructor stub
}
/**
 * @brief Perform a BLE Scan and return the results.
 */
JsonArray BLEExplorer::scan() {
	ESP_LOGD(LOG_TAG, ">> scan");
	if(!isRunning){
		BLEDevice::init("");
		isRunning = true;
	}
	BLEScan* pBLEScan = BLEDevice::getScan();
	pBLEScan->setActiveScan(true);
	BLEScanResults scanResults = pBLEScan->start(1);
	JsonObject obj = JSON::createObject();
	JsonArray arr = JSON::createArray();
	arr.addString("BLE devices");
	for (int c = 0; c < scanResults.getCount(); ++c) {
		BLEAdvertisedDevice dev =  scanResults.getDevice(c);
		arr.addObject(enumerateDevices(dev));
	}
	obj.setInt("deviceCount", scanResults.getCount());
	ESP_LOGD(LOG_TAG, "<< scan");
	return arr;
} // scan
/*
 * @brief Connect to BLE server to get and enumerate services and characteristics
 */
JsonArray BLEExplorer::connect(std::string _addr){
	BLEAddress *pAddress = new BLEAddress(_addr);
	BLEClient*  pClient  = BLEDevice::createClient();

	// Connect to the remove BLE Server.
	pClient->connect(*pAddress);
	std::map<std::string, BLERemoteService*> *pRemoteServices = pClient->getServices();
	if (pRemoteServices == nullptr) {
		ESP_LOGD(LOG_TAG, "Failed to find services");
		return JSON::createArray();
	}
//	Memory::startTraceAll();
	JsonArray arr = JSON::createArray();
	for (auto it=pRemoteServices->begin(); it!=pRemoteServices->end(); ++it) {
		std::map<uint16_t, BLERemoteCharacteristic*> pRemoteCharacteristics;
		it->second->getCharacteristics(&pRemoteCharacteristics);
		arr.addObject(enumerateCharacteristics(it->second, &pRemoteCharacteristics, _addr));
	}

	pClient->disconnect();
/*
	free(pClient);
	free(pAddress);   //FIXME using it here causing multi heap crash
	free(pRemoteServices);*/
	return arr;
}
/*
 * @brief Enumerate devices
 */
JsonObject BLEExplorer::enumerateDevices(BLEAdvertisedDevice device){
	JsonArray arr = JSON::createArray();
	JsonObject obj = JSON::createObject();
	obj.setString("id", device.getAddress().toString());
	obj.setString("parent", "#");
	obj.setString("text", device.getName() == ""? "N/A":device.getName());
	JsonObject state = JSON::createObject();
	state.setBoolean("opened", true);
	obj.setObject("state", state);
	JsonObject obj2 = JSON::createObject();
	//obj2.setString("id", device.getAddress().toString());
	//obj2.setString("parent", device.getAddress().toString());
	obj2.setString("text", "Address - " + device.getAddress().toString());
	//obj2.setBoolean("children", false);
	arr.addObject(obj2);
	if(device.haveManufacturerData()){
		obj2 = JSON::createObject();
		obj2.setString("text", "Manufacturer - " + device.getManufacturerData());
		arr.addObject(obj2);
	}
	std::stringstream ss;
	if(device.haveAppearance()){
		ss << "Appearance - " << device.getAppearance();
		obj2 = JSON::createObject();
		obj2.setString("text", ss.str());
		arr.addObject(obj2);
	}
	if(device.haveRSSI()){
		ss.str("");
		ss << "RSSI - " <<  device.getRSSI();
		obj2 = JSON::createObject();
		obj2.setString("text", ss.str());
		arr.addObject(obj2);
	}
	if(device.haveTXPower()){
		ss.str("");
		ss << "TX power - " << device.getTXPower();
		obj2 = JSON::createObject();
		obj2.setString("text", ss.str());
		arr.addObject(obj2);
	}
	if(device.haveServiceUUID()){
		ss.str("");
		ss << "Services - " << device.getAddress().toString();
		obj2 = JSON::createObject();
		obj2.setString("text", ss.str());
		arr.addObject(obj2);
	}


	obj.setArray("children", arr);

	return obj;
}
/*
 * @brief Enumerate characteristics from given service
 */
JsonObject BLEExplorer::enumerateCharacteristics(BLERemoteService *pService, std::map<uint16_t, BLERemoteCharacteristic*> *characteristicMap, std::string _addr){
	JsonObject obj = JSON::createObject();
	JsonArray arr = JSON::createArray();
	char tmp;
	itoa(pService->getHandle(), &tmp, 16);

	obj.setString("id", &tmp);  // todo service's handle
	obj.setString("text", BLEUtils::gattServiceToString(pService->getUUID().getNative()->uuid.uuid32) + " Service: " + pService->getUUID().toString());
	obj.setString("icon", "service");
	JsonObject state = JSON::createObject();
	state.setBoolean("opened", true);
	obj.setObject("state", state);
	obj.setString("parent", _addr);

	for (auto it=characteristicMap->begin(); it!=characteristicMap->end(); ++it) {  // TODO add descriptors enumerator, add short uuid if characteristic !UNKNOWN
		JsonObject ch = JSON::createObject();
		itoa(it->second->getHandle(), &tmp, 16);
		ch.setString("id", &tmp);  // characteristic's handle
		ch.setString("text", BLEUtils::gattCharacteristicUUIDToString(it->second->getUUID().getNative()->uuid.uuid16) + " Characteristic: " + it->second->getUUID().toString());
		ch.setString("icon", "characteristic");
		arr.addObject(ch);
	}

	obj.setArray("children", arr);
	return obj;
}

void BLEExplorer::createServer(std::string name){
	BLEDevice::init(name);
	pServer = BLEDevice::createServer();
	mCharacteristics = new BLECharacteristicMap();
	mServices = new BLEServiceMap();
}

void BLEExplorer::startService(uint16_t handle){
	BLEService *pservice = mServices->getByHandle(handle);
	pservice->start();
}

JsonObject BLEExplorer::addService(BLEUUID _uuid){
	JsonObject obj = JSON::createObject();

	char ptr[33];
	BLEService *pservice = pServer->createService(_uuid);
	if(pservice != nullptr){
		mServices->setByHandle(pservice->getHandle(), pservice);
		pservice->start();
		obj.setString("icon", "service");
		itoa(pservice->getHandle(), ptr, 16);
		obj.setString("handle", ptr);
		obj.setString("parent", "#");
		obj.setString("text", BLEUtils::gattServiceToString(pservice->getUUID().getNative()->uuid.uuid16) + " Service: " + pservice->getUUID().toString());
	}
	return obj;
}

JsonObject BLEExplorer::addCharacteristic(BLEUUID uuid, uint16_t service){
	char ptr[33];
	BLEService *pservice = mServices->getByHandle(service);
	ESP_LOGE(LOG_TAG, "ADD: %d", pservice->getHandle());
	BLECharacteristic *charact = pservice->createCharacteristic(BLEUUID(uuid), {BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE});
	charact->setValue("Private name");
	JsonObject obj = JSON::createObject();
	obj.setString("icon", "characteristic");
	itoa(charact->getHandle(), ptr, 16);
	obj.setString("handle", ptr);
	itoa(service, ptr, 16);
	obj.setString("parent", ptr);
	obj.setString("text", BLEUtils::gattCharacteristicUUIDToString(charact->getUUID().getNative()->uuid.uuid16) + " Characteristic: " + charact->getUUID().toString());
	return obj;
}

JsonObject BLEExplorer::addDescriptor(BLEUUID uuid, uint16_t charact){
	char ptr[33];
	BLECharacteristic *pcharact = mCharacteristics->getByHandle(charact);
	BLEDescriptor descr = BLEDescriptor(uuid);
	pcharact->addDescriptor(&descr);
	JsonObject obj = JSON::createObject();
	obj.setString("icon", "descriptor");
	itoa(descr.getHandle(), ptr, 16);
	obj.setString("handle", ptr);
	obj.setString("parent", pcharact->getUUID().toString());
	obj.setString("text", BLEUtils::gattDescriptorUUIDToString(uuid.getNative()->uuid.uuid32) + " Descriptor: " + uuid.toString());
	return obj;
}

void BLEExplorer::startAdvertising(){
	pServer->getAdvertising()->start();
}

void BLEExplorer::stopAdvertising(){
	pServer->getAdvertising()->stop();
}

