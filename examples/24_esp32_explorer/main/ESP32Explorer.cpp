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
#include <HttpServer.h>
#include <WebServer.h>
#include <TFTP.h>
#include <JSON.h>
#include <WiFi.h>
#include <stdio.h>
#include <esp_wifi.h>

#include <GeneralUtils.h>


static char tag[] = "ESP32Explorer";
static const char* LOG_TAG =tag;

extern JsonObject I2S_JSON();
extern JsonObject GPIO_JSON();
extern JsonObject WIFI_JSON();
extern JsonObject SYSTEM_JSON();
extern JsonObject FILESYSTEM_GET_JSON_DIRECTORY(std::string path, bool isRecursive);
extern JsonObject FILESYSTEM_GET_JSON_CONTENT(std::string path);
extern JsonObject I2C_READ(std::map<std::string, std::string> parts);
extern JsonObject I2C_WRITE(std::map<std::string, std::string> parts);
extern JsonObject I2C_SCAN_JSON();

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

	pResponse->addHeader("access-control-allow-origin", "*");
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
	//JsonObject obj = FILESYSTEM_GET_JSON(path.c_str());
	//pResponse->sendData(obj.toString());
	//JSON::deleteObject(obj);
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

/*
class MyWebSocketHandler : public WebServer::WebSocketHandler {
	void onMessage(std::string message) {
		ESP_LOGD(LOG_TAG, "MyWebSocketHandler: Data length: %s", message.c_str());
		JsonObject obj = JSON::parseObject(message);
		std::string command = obj.getString("command");
		ESP_LOGD(LOG_TAG, "Command: %s", command.c_str());
		if (command == "getfile") {
			std::string path = obj.getString("path");
			ESP_LOGD(LOG_TAG, "   path: %s", path.c_str());
			File f(path);
			std::string fileContent = f.getContent(0, 1000);
			ESP_LOGD(LOG_TAG, "Length of file content is %d", fileContent.length());
			// send response
			sendData(fileContent);
		}
	} // onMessage
}; // MyWebSocketHandler

class MyWebSocketHandlerFactory : public WebServer::WebSocketHandlerFactory {
	WebServer::WebSocketHandler* newInstance() {
		return new MyWebSocketHandler();
	}
};
*/

#if 0

static void handle_REST_I2C_INIT(HttpRequest *pRequest, HttpResponse *pResponse) { //TODO implement slave mode
	ESP_LOGD(LOG_TAG, ">> init_I2C");
	pResponse->addHeader("access-control-allow-origin", "*");
	pResponse->addHeader("Content-Type", "application/json");
	std::map<std::string, std::string> parts = pRequest->parseForm();
	if(atoi(pRequest->getHeader(pRequest->HTTP_HEADER_CONTENT_LENGTH).c_str())>0){
		uint8_t SDA, SCL, I2C_NUM, SPEED, I2C_MODE;
		I2C_NUM = atoi(parts.at("port").c_str());
		SDA = atoi(parts.at("sda").c_str());
		SCL = atoi(parts.at("scl").c_str());
		SPEED = atoi(parts.at("speed").c_str());
		I2C_MODE = atoi(parts.at("mode").c_str());

		i2c_config_t conf;
		conf.mode = I2C_MODE_MASTER;
		conf.sda_io_num = (gpio_num_t)SDA;
		conf.scl_io_num = (gpio_num_t)SCL;
		conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
		conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
		conf.master.clk_speed = SPEED;
		::i2c_param_config((i2c_port_t)I2C_NUM, &conf);
		::i2c_driver_install((i2c_port_t)I2C_NUM, (i2c_mode_t)I2C_MODE, 0, 0, 0);
	}
	ESP_LOGI(LOG_TAG, "<< init_i2c");

	JsonObject obj = I2C_SCAN_JSON();
	pResponse->sendData(obj.toString());
	JSON::deleteObject(obj);
} // handle_REST_I2C_INIT


static void handle_REST_I2C_CLOSE(HttpRequest *pRequest, HttpResponse *pResponse) {
	ESP_LOGD(LOG_TAG, ">> delete_I2C");
	std::map<std::string, std::string> parts = pRequest->parseForm();
	if(atoi(pRequest->getHeader(pRequest->HTTP_HEADER_CONTENT_LENGTH).c_str())>0){
		uint8_t I2C_NUM;
		I2C_NUM = atoi(parts.at("port").c_str());
		::i2c_driver_delete((i2c_port_t)I2C_NUM);
		ESP_LOGI(LOG_TAG, "<< delete_i2c");
	}

	pResponse->addHeader("access-control-allow-origin", "*");
	pResponse->addHeader("Content-Type", "text/plain");
	pResponse->sendData("i2c port closed");
	//JSON::deleteObject(obj);
} // handle_REST_I2C_CLOSE


static void handle_REST_I2C_SCAN(HttpRequest *pRequest, HttpResponse *pResponse) {
	ESP_LOGD(LOG_TAG, ">> init_I2C");
	pResponse->addHeader("access-control-allow-origin", "*");
	pResponse->addHeader("Content-Type", "application/json");
	JsonObject obj = I2C_SCAN_JSON();
	pResponse->sendData(obj.toString());
	JSON::deleteObject(obj);
} // handle_REST_I2C_SCAN


static void handle_REST_I2C_COMMAND_READ(HttpRequest *pRequest, HttpResponse *pResponse) {
	ESP_LOGD(LOG_TAG, ">> init_I2C");
	pResponse->addHeader("access-control-allow-origin", "*");
	pResponse->addHeader("Content-Type", "application/json");
	std::map<std::string, std::string> parts = pRequest->parseForm();
	JsonObject obj = I2C_READ(parts);
	pResponse->sendData(obj.toString());
	JSON::deleteObject(obj);
} //

static void handle_REST_I2C_COMMAND_WRITE(HttpRequest *pRequest, HttpResponse *pResponse) {
	ESP_LOGD(LOG_TAG, ">> init_I2C");
	pResponse->addHeader("access-control-allow-origin", "*");
	pResponse->addHeader("Content-Type", "application/json");
	JsonArray obj = JSON::createArray();
	std::map<std::string, std::string> parts = pRequest->parseForm();
	JsonObject obj2 = I2C_WRITE(parts);
	pResponse->sendData(obj2.toString());
	JSON::deleteArray(obj);
	JSON::deleteObject(obj2);
} //

static void handle_REST_BLE_CLIENT_SCAN(HttpRequest* pRequest, HttpResponse* pResponse) {
	ESP_LOGD(LOG_TAG, "handle_REST_BLE_CLIENT_SCAN");
	JsonArray obj = g_pBLEExplorer->scan();
	pResponse->addHeader("access-control-allow-origin", "*");
	pResponse->addHeader("Content-Type", "application/json");
	pResponse->sendData(obj.toString());
	JSON::deleteArray(obj);
} // handle_REST_BLE_CLIENT_SCAN


static void handle_REST_BLE_CLIENT_CONNECT(HttpRequest* pRequest, HttpResponse* pResponse) {
	ESP_LOGD(LOG_TAG, "handle_REST_BLE_CLIENT_CONNECT");
	JsonArray obj = JSON::createArray();
	std::map<std::string, std::string> parts = pRequest->parseForm();
	if(atoi(pRequest->getHeader(pRequest->HTTP_HEADER_CONTENT_LENGTH).c_str())>0){
		obj = g_pBLEExplorer->connect(parts.at("connect").c_str());
	}
	pResponse->addHeader("access-control-allow-origin", "*");
	pResponse->addHeader("Content-Type", "application/json");
	pResponse->sendData(obj.toString());
	JSON::deleteArray(obj);
} // handle_REST_BLE_CLIENT_SCAN

static void handle_REST_BLE_SERVER_CREATE(HttpRequest* pRequest, HttpResponse* pResponse) {
	g_pBLEExplorer->createServer("Esp32");
	pResponse->addHeader("access-control-allow-origin", "*");
	pResponse->addHeader("Content-Type", "text/plain");
	pResponse->sendData("create server");
}
static void handle_REST_BLE_SERVER_DELETE(HttpRequest* pRequest, HttpResponse* pResponse) {
	g_pBLEExplorer->deleteServer();
}
static void handle_REST_BLE_SERVER_START_ADV(HttpRequest* pRequest, HttpResponse* pResponse) {
	g_pBLEExplorer->startAdvertising();
	pResponse->addHeader("access-control-allow-origin", "*");
	pResponse->addHeader("Content-Type", "text/plain");
	pResponse->sendData("start adv");
}
static void handle_REST_BLE_SERVER_STOP_ADV(HttpRequest* pRequest, HttpResponse* pResponse) {
	/*JsonObject obj = JSON::createObject();
	std::map<std::string, std::string> parts = pRequest->parseForm();
	if(atoi(pRequest->getHeader(pRequest->HTTP_HEADER_CONTENT_LENGTH).c_str())>0){
		uint16_t service = strtoul(parts.at("handle").c_str(), NULL, 16);
		g_pBLEExplorer->startService(service);
	}
*/
	g_pBLEExplorer->stopAdvertising();
	pResponse->addHeader("access-control-allow-origin", "*");
	pResponse->addHeader("Content-Type", "text/plain");
	pResponse->sendData("stop adv");
}
static void handle_REST_BLE_SERVER_ADD_SERVICE(HttpRequest* pRequest, HttpResponse* pResponse) {
	JsonObject obj = JSON::createObject();
	std::map<std::string, std::string> parts = pRequest->parseForm();
	if(atoi(pRequest->getHeader(pRequest->HTTP_HEADER_CONTENT_LENGTH).c_str())>0){
		BLEUUID uuid = BLEUUID::fromString(parts.at("UUID").c_str());
		obj = g_pBLEExplorer->addService(uuid);
	}

	pResponse->addHeader("access-control-allow-origin", "*");
	pResponse->addHeader("Content-Type", "application/json");
	pResponse->sendData(obj.toString());
}
static void handle_REST_BLE_SERVER_GET_SERVICES(HttpRequest* pRequest, HttpResponse* pResponse) {
}
static void handle_REST_BLE_SERVER_ADD_CHARACTERISTIC(HttpRequest* pRequest, HttpResponse* pResponse) {
	JsonObject obj = JSON::createObject();
	std::map<std::string, std::string> parts = pRequest->parseForm();
	if(atoi(pRequest->getHeader(pRequest->HTTP_HEADER_CONTENT_LENGTH).c_str())>0){
		BLEUUID uuid = BLEUUID::fromString(parts.at("UUID").c_str());
		uint16_t service = strtoul(parts.at("serviceUUID").c_str(), NULL, 16);
		obj = g_pBLEExplorer->addCharacteristic(uuid, service);
	}

	pResponse->addHeader("access-control-allow-origin", "*");
	pResponse->addHeader("Content-Type", "application/json");
	pResponse->sendData(obj.toString());
}

static void handle_REST_BLE_SERVER_GET_(HttpRequest* pRequest, HttpResponse* pResponse) {
}
static void handle_REST_BLE_SERVER_ADD_DESCRIPTOR(HttpRequest* pRequest, HttpResponse* pResponse) {
	JsonObject obj = JSON::createObject();
	std::map<std::string, std::string> parts = pRequest->parseForm();
	if(atoi(pRequest->getHeader(pRequest->HTTP_HEADER_CONTENT_LENGTH).c_str())>0){
		BLEUUID uuid = BLEUUID::fromString(parts.at("UUID").c_str());
		uint16_t characteristic = strtoul(parts.at("characteristicUUID").c_str(), NULL, 16);
		obj = g_pBLEExplorer->addDescriptor(uuid, characteristic);
	}

	pResponse->addHeader("access-control-allow-origin", "*");
	pResponse->addHeader("Content-Type", "application/json");
	pResponse->sendData(obj.toString());
}
#endif

static void handle_REST_BLE_SERVER_GET_DESCRIPTOR(HttpRequest* pRequest, HttpResponse* pResponse) {
}

class TFTPTask : public Task {
   void run(void *data) {
  	 TFTP tftp;
  	 tftp.setBaseDir("/spiflash");
  	 tftp.start();
   }
};


ESP32_Explorer::ESP32_Explorer() {
	//m_pBLEExplorer = new BLEExplorer();
	//g_pBLEExplorer = new BLEExplorer();
} // ESP32_Explorer


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

	//TFTPTask *pTFTPTask = new TFTPTask();
	//pTFTPTask->setStackSize(8000);
	//pTFTPTask->start();
	ESP32CPP::GPIO::setOutput(GPIO_NUM_25);
	ESP32CPP::GPIO::setOutput(GPIO_NUM_26);
	ESP32CPP::GPIO::high(GPIO_NUM_25);
	ESP32CPP::GPIO::low(GPIO_NUM_26);

	 /*
	  * Create a WebServer and register handlers for REST requests.
	  */
	 HttpServer* pHttpServer = new HttpServer();
	 pHttpServer->setRootPath("/spiflash");
	 pHttpServer->addPathHandler("GET",    "/hello",      		           handleTest);
	 pHttpServer->addPathHandler("GET",    "/ESP32/WIFI",                  handle_REST_WiFi);
	 pHttpServer->addPathHandler("GET",    "/ESP32/I2S",                   handle_REST_I2S);
	 pHttpServer->addPathHandler("GET",    "/ESP32/GPIO",                  handle_REST_GPIO);
	 pHttpServer->addPathHandler("POST",   "/ESP32/GPIO/SET",              handle_REST_GPIO_SET);
	 pHttpServer->addPathHandler("POST",   "/ESP32/GPIO/CLEAR",            handle_REST_GPIO_CLEAR);
	 pHttpServer->addPathHandler("POST",   "/ESP32/GPIO/DIRECTION/INPUT",  handle_REST_GPIO_DIRECTION_INPUT);
	 pHttpServer->addPathHandler("POST",   "/ESP32/GPIO/DIRECTION/OUTPUT", handle_REST_GPIO_DIRECTION_OUTPUT);
	 pHttpServer->addPathHandler("POST",   "/ESP32/LOG/SET",               handle_REST_LOG_SET);
	 pHttpServer->addPathHandler("GET",    "/ESP32/FILE",                  handle_REST_FILE_GET);
	 pHttpServer->addPathHandler("POST",   "/ESP32/FILE",                  handle_REST_FILE_POST);
	 pHttpServer->addPathHandler("DELETE", "/ESP32/FILE",                  handle_REST_FILE_DELETE);
	 pHttpServer->addPathHandler("GET",    "/ESP32/SYSTEM",                handle_REST_SYSTEM);
	 
#if 0
// I2C
	 pHttpServer->addPathHandler("POST",   "/ESP32/I2C/COMMAND/READ",      handle_REST_I2C_COMMAND_READ);
	 pHttpServer->addPathHandler("POST",   "/ESP32/I2C/COMMAND/WRITE",     handle_REST_I2C_COMMAND_WRITE);
	 pHttpServer->addPathHandler("POST",   "/ESP32/I2C/INIT",              handle_REST_I2C_INIT);
	 pHttpServer->addPathHandler("GET",    "/ESP32/I2C/SCAN",              handle_REST_I2C_SCAN);
	 pHttpServer->addPathHandler("POST",   "/ESP32/I2C/DEINIT",            handle_REST_I2C_CLOSE);
	 // BLE CLIENT
	 pHttpServer->addPathHandler("GET",    "/ESP32/BLE/CLIENT/SCAN",       handle_REST_BLE_CLIENT_SCAN);
	 pHttpServer->addPathHandler("POST",   "/ESP32/BLE/CLIENT/CONNECT",    handle_REST_BLE_CLIENT_CONNECT);
	 // BLE SERVER
	 pHttpServer->addPathHandler("POST",   "/ESP32/BLE/SERVER",   		   handle_REST_BLE_SERVER_CREATE);
	 //pHttpServer->addPathHandler("DELETE", "/ESP32/BLE/SERVER",   		   handle_REST_BLE_SERVER_DELETE);
	 pHttpServer->addPathHandler("POST",   "/ESP32/BLE/SERVER/SERVICE",    handle_REST_BLE_SERVER_ADD_SERVICE);
	 pHttpServer->addPathHandler("DELETE", "/ESP32/BLE/SERVER/SERVICE",    handle_REST_BLE_SERVER_ADD_SERVICE);
	 pHttpServer->addPathHandler("GET",    "/ESP32/BLE/SERVER/SERVICES",   handle_REST_BLE_SERVER_GET_SERVICES);
	 pHttpServer->addPathHandler("POST",   "/ESP32/BLE/SERVER/CHARACTERISTIC",    handle_REST_BLE_SERVER_ADD_CHARACTERISTIC);
	 pHttpServer->addPathHandler("DELETE", "/ESP32/BLE/SERVER/CHARACTERISTIC",    handle_REST_BLE_SERVER_ADD_SERVICE);
	 pHttpServer->addPathHandler("GET",    "/ESP32/BLE/SERVER/CHARACTERISTICS",   handle_REST_BLE_SERVER_GET_SERVICES);
	 pHttpServer->addPathHandler("POST",   "/ESP32/BLE/SERVER/DESCRIPTOR",    handle_REST_BLE_SERVER_ADD_DESCRIPTOR);
	 pHttpServer->addPathHandler("DELETE", "/ESP32/BLE/SERVER/DESCRIPTOR",    handle_REST_BLE_SERVER_ADD_SERVICE);
	 pHttpServer->addPathHandler("GET",    "/ESP32/BLE/SERVER/DESCRIPTORS",   handle_REST_BLE_SERVER_GET_SERVICES);
	 pHttpServer->addPathHandler("GET",    "/ESP32/BLE/SERVER/START",    	handle_REST_BLE_SERVER_START_ADV);
	 pHttpServer->addPathHandler("POST",    "/ESP32/BLE/SERVER/STOP",   		handle_REST_BLE_SERVER_STOP_ADV);
#endif
	 //pHttpServer->setWebSocketHandlerFactory(new MyWebSocketHandlerFactory());
	 pHttpServer->start(80); // Start the WebServer listening on port 80.
	  	ESP_LOGE(LOG_TAG, "%d, minimum ever: %d", xPortGetFreeHeapSize(), xPortGetMinimumEverFreeHeapSize());
} // start
