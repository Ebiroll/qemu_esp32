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
#include <esp_spi_flash.h>

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
	printf("handle_REST_LOG_SET\n");

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


static void handle_REST_MMU(WebServer::HTTPRequest *pRequest, WebServer::HTTPResponse *pResponse) {
	ESP_LOGD(tag, "handle_REST_MMU");
	std::vector<std::string> parts = pRequest->pathSplit();
	std::string path = "";
	for (int i=3; i<parts.size(); i++) {
		path += "/" + parts[i];
	}
	if (parts.size()>1) {
		printf("QUERY_FOR %s\n",parts[parts.size()-1].c_str());
	}

    unsigned char*mem_location=(unsigned char*)0x3FF10000;

	unsigned int*app_location=(unsigned int*)0x3FF12000;
	unsigned int*simple_mem_location=(unsigned int*)mem_location;
    //unsigned int* end=(unsigned int*)0x3FF12000;
    unsigned int* end=(unsigned int*)mem_location+(63*4);
	int total=0;

    JsonObject obj = JSON::createObject();
    JsonArray arr = JSON::createArray();
    obj.setString("status","success");
    JsonObject item(NULL);

    while(simple_mem_location<end) {
		char buff[128];
        item = JSON::createObject();

		item.setInt("recid",total+1);

		sprintf( buff , "%p", simple_mem_location);
        //printf( "%p\n", simple_mem_location);
        item.setString("MEM",std::string(buff));

        sprintf( buff , "0x%X", *simple_mem_location);
        item.setString("VAL",std::string(buff));

        simple_mem_location++;

        sprintf( buff , "0x%X", *app_location);
        item.setString("AVAL",std::string(buff));

        app_location++;
		total++;		

        arr.addObject(item);
    }

    obj.setInt("total",total);
    obj.setArray("records",arr);
	pResponse->sendData(obj.toString());
	JSON::deleteObject(obj);
} // handle_REST_MMU


// The last part of the path is the mmu_ index selected

// MMU register, 0 holds info about VADDR 0x3f400000
// MMU register, x holds info about VADDR 0x3f400000 + (x * 0x10000) 
// The MMU is more flexible but currently this is how esp-idf maps VADDR 
/*
MMU_BLOCK0_VADDR 0x3f400000    
MMU_BLOCK1_VADDR 0x3f410000

MMU_BLOCK50_VADDR 0x3f720000
..
MMU_BLOCK63_VADDR 0x3fA30000
*/
static void handle_REST_MEM(WebServer::HTTPRequest *pRequest, WebServer::HTTPResponse *pResponse) {
	ESP_LOGD(tag, "handle_REST_MEM");
	std::vector<std::string> parts = pRequest->pathSplit();
	std::string path = "";
	for (int i=3; i<parts.size(); i++) {
		path += "/" + parts[i];
	}
	if (parts.size()>1) {
		printf("MEM_QUERY_FOR %s\n",parts[parts.size()-1].c_str());
	}
	int ix=atoi(parts[parts.size()-1].c_str());

    unsigned char*mem_location=(unsigned char*)0x3f400000 + (ix * 0x10000);
	unsigned int*simple_mem_location=(unsigned int*)mem_location;
    unsigned int* end=(unsigned int*) mem_location +64;
	int total=0;
/*
	if (ix>=51) {
		mem_location=(unsigned char*)0x3f720000 + ((ix-51) * 0x10000);
		simple_mem_location=(unsigned int*)mem_location;
		printf( "instr ix %d %p\n", ix, simple_mem_location);
		end=(unsigned int*) mem_location +64;
	}
*/

	// Dont think this is correct, but seems to work for ix>77 Should be MMU_BLOCK50_VADDR 0x3f720000 ??? 
	if (ix>=51) {
		mem_location=(unsigned char*)0x400d0000+(ix-77)*0x10000;
		unsigned int *silly_location=(unsigned int*)0x3f720000 + ((ix-51) * 0x10000);
		simple_mem_location=(unsigned int*)mem_location;
		printf( "ix %d %p\n", ix, simple_mem_location);
		printf( "silly ix %d %p\n", ix, silly_location);
		end=(unsigned int*) mem_location +64;
	}

    JsonObject obj = JSON::createObject();
    JsonArray arr = JSON::createArray();
    obj.setString("status","success");
    JsonObject item(NULL);

    while(simple_mem_location<end) {
		char buff[128];
        item = JSON::createObject();
		sprintf( buff , "%p", simple_mem_location);
        //printf( "%p\n", simple_mem_location);
        item.setString("MEM",std::string(buff));

        simple_mem_location++;
		total++;		
        sprintf( buff , "%08X", *simple_mem_location);
        item.setString("VAL",std::string(buff));
        arr.addObject(item);
    }

    obj.setInt("total",total);
    obj.setArray("records",arr);
	pResponse->sendData(obj.toString());
	JSON::deleteObject(obj);
} // handle_REST_MMU


// The last part of the path is the mmu_ index selected
static void handle_REST_FLASH(WebServer::HTTPRequest *pRequest, WebServer::HTTPResponse *pResponse) {
	ESP_LOGD(tag, "handle_REST_FLASH");
	int buffer[64];

	std::vector<std::string> parts = pRequest->pathSplit();
	std::string path = "";
	for (int i=3; i<parts.size(); i++) {
		path += "/" + parts[i];
	}
	if (parts.size()>1) {
		printf("FLASH_QUERY_FOR %s\n",parts[parts.size()-1].c_str());
	}

    int val=(int)strtol(parts[parts.size()-1].c_str(), NULL, 0);
	val = val*0x10000;

	spi_flash_read(val,buffer,64*4);

    unsigned char*mem_location=(unsigned char*) &buffer[0];
	unsigned int*simple_mem_location=(unsigned int*)mem_location;
    //unsigned int* end=(unsigned int*)0x3FF12000;
    unsigned int* end=(unsigned int*)&buffer[64];
	int total=0;

    JsonObject obj = JSON::createObject();
    JsonArray arr = JSON::createArray();
    obj.setString("status","success");
    JsonObject item(NULL);

    while(simple_mem_location<end) {
		char buff[128];
        item = JSON::createObject();
		sprintf( buff , "%08X", val+total*4);
        //printf( "%p\n", simple_mem_location);
        item.setString("MEM",std::string(buff));

        simple_mem_location++;
		total++;		
        sprintf( buff , "%08X", *simple_mem_location);
        item.setString("VAL",std::string(buff));
        arr.addObject(item);
    }

    obj.setInt("total",total);
    obj.setArray("records",arr);
	pResponse->sendData(obj.toString());
	JSON::deleteObject(obj);
} // handle_REST_FLASH


void webserverTask(void *data) {
	/*
	* Create a WebServer and register handlers for REST requests.
	*/
	WebServer *pWebServer = new WebServer();
	pWebServer->setRootPath("/spiffs");
	pWebServer->addPathHandler("POST", "^\\/api\\/mmu\\/.*",                    handle_REST_MMU);
	pWebServer->addPathHandler("POST", "^\\/api\\/mem\\/.*",                    handle_REST_MEM);
	pWebServer->addPathHandler("POST", "^\\/api\\/flash\\/.*",                  handle_REST_FLASH);
	// 
	pWebServer->addPathHandler("GET", "^\\/api\\/mmu\\/.*",                     handle_REST_MMU);
	pWebServer->addPathHandler("GET", "^\\/api\\/mem\\/.*",                     handle_REST_MEM);
	pWebServer->addPathHandler("GET", "^\\/api\\/flash\\/.*",                   handle_REST_FLASH);


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
