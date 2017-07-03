/*
 * ESP32Explorer.cpp
 *
 *  Created on: May 22, 2017
 *      Author: kolban
 */
#include <string>
#include <iostream>
#include <GPIO.h>
#include "ESP32Explorer.h"
#include "sdkconfig.h"
#include <esp_log.h>
#include <FATFS_VFS.h>
#include <FileSystem.h>
#include <FreeRTOS.h>
#include <Task.h>
#include <WebServer.h>
#include <TFTP.h>
#include <JSON.h>
#include <WiFi.h>
#include <stdio.h>

#include <GeneralUtils.h>

#include <esp_wifi.h>

static char tag[] = "ESP32Explorer";

extern JsonObject I2S_JSON();
extern JsonObject GPIO_JSON();
extern JsonObject WIFI_JSON();
extern JsonObject SYSTEM_JSON();
extern JsonObject FILESYSTEM_GET_JSON(std::string path);

static void handleTest(WebServer::HTTPRequest *pRequest, WebServer::HTTPResponse *pResponse) {
	ESP_LOGD(tag, "handleTest called");
	ESP_LOGD(tag, "Path: %s" ,pRequest->getPath().c_str());
	ESP_LOGD(tag, "Method: %s", pRequest->getMethod().c_str());
	ESP_LOGD(tag, "Query:");
	std::map<std::string, std::string> queryMap = pRequest->getQuery();
	std::map<std::string, std::string>::iterator itr;
	for (itr=queryMap.begin(); itr != queryMap.end(); itr++) {
		ESP_LOGD(tag, "name: %s, value: %s", itr->first.c_str(), itr->second.c_str());
	}
	ESP_LOGD(tag, "Body: %s", pRequest->getBody().c_str());
	pResponse->sendData("hello!");
} // handleTest


static void handle_REST_SYSTEM(WebServer::HTTPRequest *pRequest, WebServer::HTTPResponse *pResponse) {
	ESP_LOGD(tag, "handle_REST_SYSTEM");
	pResponse->addHeader("Content-Type", "application/json");
	JsonObject obj = SYSTEM_JSON();
	pResponse->sendData(obj.toString());
	JSON::deleteObject(obj);
} // handle_REST_GPIO


static void handle_REST_I2S(WebServer::HTTPRequest *pRequest, WebServer::HTTPResponse *pResponse) {
	ESP_LOGD(tag, "handle_REST_I2S");
	pResponse->addHeader("Content-Type", "application/json");
	JsonObject obj = I2S_JSON();
	pResponse->sendData(obj.toString());
	JSON::deleteObject(obj);
} // handle_REST_I2S


static void handle_REST_GPIO(WebServer::HTTPRequest *pRequest, WebServer::HTTPResponse *pResponse) {
	ESP_LOGD(tag, "handle_REST_GPIO");
	pResponse->addHeader("Content-Type", "application/json");
	JsonObject obj = GPIO_JSON();
	pResponse->sendData(obj.toString());
	JSON::deleteObject(obj);
} // handle_REST_GPIO


static void handle_REST_GPIO_SET(WebServer::HTTPRequest *pRequest, WebServer::HTTPResponse *pResponse) {
	ESP_LOGD(tag, "handle_REST_GPIO_SET");
	std::vector<std::string> parts = pRequest->pathSplit();
	std::stringstream stream(parts[4]);
	int gpio;
	stream >> gpio;
	gpio_set_level((gpio_num_t)gpio, 1);
	JsonObject obj = GPIO_JSON();
	pResponse->sendData(obj.toString());
	JSON::deleteObject(obj);
} // handle_REST_GPIO_SET


static void handle_REST_GPIO_CLEAR(WebServer::HTTPRequest *pRequest, WebServer::HTTPResponse *pResponse) {
	ESP_LOGD(tag, "handle_REST_GPIO_CLEAR");
	std::vector<std::string> parts = pRequest->pathSplit();
	std::stringstream stream(parts[4]);
	int gpio;
	stream >> gpio;
	gpio_set_level((gpio_num_t)gpio, 0);
	JsonObject obj = GPIO_JSON();
	pResponse->sendData(obj.toString());
	JSON::deleteObject(obj);
} // handle_REST_GPIO_CLEAR


static void handle_REST_GPIO_DIRECTION_INPUT(WebServer::HTTPRequest *pRequest, WebServer::HTTPResponse *pResponse) {
	ESP_LOGD(tag, "handle_REST_GPIO_DIRECTION_INPUT");
	std::vector<std::string> parts = pRequest->pathSplit();
	std::stringstream stream(parts[5]);
	int gpio;
	stream >> gpio;
	gpio_set_direction((gpio_num_t)gpio, GPIO_MODE_INPUT);
	JsonObject obj = GPIO_JSON();
	pResponse->sendData(obj.toString());
	JSON::deleteObject(obj);
} // handle_REST_GPIO_DIRECTION_INPUT


static void handle_REST_GPIO_DIRECTION_OUTPUT(WebServer::HTTPRequest *pRequest, WebServer::HTTPResponse *pResponse) {
	ESP_LOGD(tag, "handle_REST_GPIO_DIRECTION_OUTPUT");
	std::vector<std::string> parts = pRequest->pathSplit();
	std::stringstream stream(parts[5]);
	int gpio;
	stream >> gpio;
	gpio_set_direction((gpio_num_t)gpio, GPIO_MODE_OUTPUT);
	JsonObject obj = GPIO_JSON();
	pResponse->sendData(obj.toString());
	JSON::deleteObject(obj);
} // handle_REST_GPIO_DIRECTION_OUTPUT


static void handle_REST_WiFi(WebServer::HTTPRequest *pRequest, WebServer::HTTPResponse *pResponse) {
	ESP_LOGD(tag, "handle_REST_WIFI");
	pResponse->addHeader("Content-Type", "application/json");
	JsonObject obj = WIFI_JSON();
	pResponse->sendData(obj.toString());
	JSON::deleteObject(obj);
} // handle_REST_WiFi


static void handle_REST_LOG_SET(WebServer::HTTPRequest *pRequest, WebServer::HTTPResponse *pResponse) {
	ESP_LOGD(tag, "handle_REST_LOG_SET");
	std::vector<std::string> parts = pRequest->pathSplit();
	std::stringstream stream(parts[4]);
	int logLevel;
	stream >> logLevel;
	::esp_log_level_set("*", (esp_log_level_t)logLevel);
	pResponse->addHeader("Content-Type", "application/json");
	JsonObject obj = SYSTEM_JSON();
	pResponse->sendData(obj.toString());
	JSON::deleteObject(obj);
} // handle_REST_LOG_SET


static void handle_REST_FILE_GET(WebServer::HTTPRequest *pRequest, WebServer::HTTPResponse *pResponse) {
	ESP_LOGD(tag, "handle_REST_FILE_GET");
	std::vector<std::string> parts = pRequest->pathSplit();
	std::string path = "";
	for (int i=3; i<parts.size(); i++) {
		path += "/" + parts[i];
	}
	JsonObject obj = FILESYSTEM_GET_JSON(path.c_str());
	pResponse->sendData(obj.toString());
	JSON::deleteObject(obj);
} // handle_REST_FILE_GET


class WebServerTask : public Task {
   void run(void *data) {
  	 /*
  	  * Create a WebServer and register handlers for REST requests.
  	  */
  	 WebServer *pWebServer = new WebServer();
  	 pWebServer->setRootPath("/spiflash");
  	 pWebServer->addPathHandler("GET",  "\\/hello\\/.*",                         handleTest);
  	 pWebServer->addPathHandler("GET",  "^\\/ESP32\\/WIFI$",                     handle_REST_WiFi);
  	 pWebServer->addPathHandler("GET",  "^\\/ESP32\\/I2S$",                      handle_REST_I2S);
  	 pWebServer->addPathHandler("GET",  "^\\/ESP32\\/GPIO$",                     handle_REST_GPIO);
  	 pWebServer->addPathHandler("POST", "^\\/ESP32\\/GPIO\\/SET",                handle_REST_GPIO_SET);
  	 pWebServer->addPathHandler("POST", "^\\/ESP32\\/GPIO\\/CLEAR",              handle_REST_GPIO_CLEAR);
  	 pWebServer->addPathHandler("POST", "^\\/ESP32\\/GPIO\\/DIRECTION\\/INPUT",  handle_REST_GPIO_DIRECTION_INPUT);
  	 pWebServer->addPathHandler("POST", "^\\/ESP32\\/GPIO\\/DIRECTION\\/OUTPUT", handle_REST_GPIO_DIRECTION_OUTPUT);
  	 pWebServer->addPathHandler("POST", "^\\/ESP32\\/LOG\\/SET",                 handle_REST_LOG_SET);
  	 pWebServer->addPathHandler("GET",  "^\\/ESP32\\/FILE",                      handle_REST_FILE_GET);
  	 pWebServer->addPathHandler("GET",  "^\\/ESP32\\/SYSTEM$",                   handle_REST_SYSTEM);
  	 pWebServer->start(80); // Start the WebServer listening on port 80.
   }
};


class TFTPTask : public Task {
   void run(void *data) {
  	 TFTP tftp;
  	 tftp.setBaseDir("/spiflash");
  	 tftp.start();
   }
};


ESP32_Explorer::ESP32_Explorer() {
}

ESP32_Explorer::~ESP32_Explorer() {
}

void ESP32_Explorer::start() {
	FATFS_VFS *fs = new FATFS_VFS("/spiflash", "storage");
	fs->mount();
	//FileSystem::mkdir("/spiflash/mydir2");
	//FileSystem::mkdir("/spiflash/mydir2/fred");
	//FileSystem::dumpDirectory("/spiflash/");

	WebServerTask *webServerTask = new WebServerTask();
	webServerTask->setStackSize(20000);
	webServerTask->start();

	TFTPTask *pTFTPTask = new TFTPTask();
	pTFTPTask->setStackSize(8000);
	pTFTPTask->start();
	ESP32CPP::GPIO::setOutput(GPIO_NUM_25);
	ESP32CPP::GPIO::setOutput(GPIO_NUM_26);
	ESP32CPP::GPIO::high(GPIO_NUM_25);
	ESP32CPP::GPIO::low(GPIO_NUM_26);
} // start
