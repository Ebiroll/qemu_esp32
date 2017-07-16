/*
 * ESP32Explorer.cpp
 *
 *  Created on: May 22, 2017
 *      Author: kolban
 */
#include <string>
#include <iostream>
#include "RequestManager.h"
#include "sdkconfig.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <WebServer.h>
#include <stdio.h>
#include "JSON.h"


#include <esp_wifi.h>

extern JsonObject FILESYSTEM_GET_JSON(std::string path);

static char tag[] = "RequestManager";


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
	//JsonObject obj = SYSTEM_JSON();
	//pResponse->sendData(obj.toString());
	//JSON::deleteObject(obj);
} // handle_REST_SYSTEM



static void handle_REST_LOG_SET(WebServer::HTTPRequest *pRequest, WebServer::HTTPResponse *pResponse) {
	ESP_LOGD(tag, "handle_REST_LOG_SET");
	std::vector<std::string> parts = pRequest->pathSplit();
	std::stringstream stream(parts[4]);
	int logLevel;
	stream >> logLevel;
	::esp_log_level_set("*", (esp_log_level_t)logLevel);
	pResponse->addHeader("Content-Type", "application/json");
	//JsonObject obj = SYSTEM_JSON();
	//pResponse->sendData(obj.toString());
	//JSON::deleteObject(obj);
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


void webserverTask(void *data) {
	/*
	* Create a WebServer and register handlers for REST requests.
	*/
	WebServer *pWebServer = new WebServer();
	pWebServer->setRootPath("/spiffs");
	pWebServer->addPathHandler("GET",  "\\/hello\\/.*",                         handleTest);
	pWebServer->addPathHandler("POST", "^\\/ESP32\\/LOG\\/SET",                 handle_REST_LOG_SET);
	pWebServer->addPathHandler("GET",  "^\\/ESP32\\/FILE",                      handle_REST_FILE_GET);
	pWebServer->addPathHandler("GET",  "^\\/ESP32\\/SYSTEM$",                   handle_REST_SYSTEM);
	pWebServer->start(80); // Start the WebServer listening on port 80.
}



RequestManager::RequestManager() {
}

RequestManager::~RequestManager() {
}

void RequestManager::start() {
	

	xTaskCreatePinnedToCore(webserverTask, "webserver", 20000, NULL, 14, NULL, 0);

} // start
