#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>

#include "Adafruit_GFX.h"
#include "Adafruit_HT1632.h"

#define HT_DATA 4
#define HT_WR   5
#define HT_CS   0
#define HT_CS2  2
#define HT_CS3  14

#define LED_COLS 72
#define LED_ROWS 16
#define SCREEN_BYTES 144

#define SCREEN_MEMORY_START 255

void handleRoot();
void handleClear();
void handleFill();
void handleDraw();
void handleSet();
void handleShow();
void handleBright();
void handleNotFound();

void serve_default_js();
void serve_default_css();
void serve_ajax_js();

const char *ssid = "led-display";
const char *password = "wifipassword";

IPAddress router_ip(192, 168, 1, 1);
ESP8266WebServer server (80);

Adafruit_HT1632LEDMatrix matrix = Adafruit_HT1632LEDMatrix(HT_DATA, HT_WR, HT_CS, HT_CS2, HT_CS3);


void setup() {

  Serial.begin(115200);
  EEPROM.begin(SCREEN_MEMORY_START+(SCREEN_BYTES * 50));
  
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(router_ip, router_ip, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid, password);
  MDNS.begin(ssid);
  
  matrix.begin(ADA_HT1632_COMMON_16NMOS);  
  
  matrix.fillScreen();
  delay(500);
  matrix.clearScreen();

  server.on("/", handleRoot);
  server.on("/clear", handleClear);
  server.on("/fill", handleFill);
  server.on("/draw", handleDraw);
  server.on("/set", handleSet);
  server.on("/show", handleShow);
  server.on("/brghtness", handleBright);

  server.on("/style.css", serve_default_css);
  server.on("/ajax.js", serve_ajax_js);
  server.on("/default.js", serve_default_js);
   
  server.onNotFound (handleNotFound);
  
  server.begin();
  
  Serial.println("HTTP server started");
}

void loop ( void ) {
  server.handleClient();
}
