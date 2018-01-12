// 1-channel LoRa Gateway for ESP8266
// Copyright (c) 2016, 2017 Maarten Westenberg version for ESP8266
// Version 5.0.1
// Date: 2017-11-15
// Author: Maarten Westenberg (mw12554@hotmail.com)
//
// 	based on work done by Thomas Telkamp for Raspberry PI 1-ch gateway
//	and many others.
//
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the MIT License
// which accompanies this distribution, and is available at
// https://opensource.org/licenses/mit-license.php
//
// The protocols and specifications used for this 1ch gateway: 
// 1. LoRA Specification version V1.0 and V1.1 for Gateway-Node communication
//	
// 2. Semtech Basic communication protocol between Lora gateway and server version 3.0.0
//	https://github.com/Lora-net/packet_forwarder/blob/master/PROTOCOL.TXT
//
// Notes: 
// - Once call gethostbyname() to get IP for services, after that only use IP
//	 addresses (too many gethost name makes the ESP unstable)
// - Only call yield() in main stream (not for background NTP sync). 
//
// ----------------------------------------------------------------------------------------

//

#include "ESP-sc-gway.h"					
// This file contains configuration of GWay

#include <Esp.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>
#include <sys/time.h>
#include <cstring>
#include <SPI.h>
#include <TimeLib.h>							// http://playground.arduino.cc/code/time
#ifdef ESP32BUILD
#include "esp_wifi.h"
#include "WiFi.h"
#include "SPIFFS.h"
#else
#include <ESP8266WiFi.h>
#include <DNSServer.h> 							// Local DNSserver
#endif
#include "FS.h"
#include <WiFiUdp.h>
#include <pins_arduino.h>
#include <ArduinoJson.h>
#include <SimpleTimer.h>
#include <gBase64.h>							// https://github.com/adamvr/arduino-base64 (changed the name)
#ifndef ESP32BUILD
#include <ESP8266mDNS.h>

extern "C" {
#include "user_interface.h"
#include "lwip/err.h"
#include "lwip/dns.h"
#include "c_types.h"
}
#endif

#if MUTEX_LIB==1
#include <mutex.h>								// See lib directory
#endif

#include "loraModem.h"
#include "loraFiles.h"


#if WIFIMANAGER>0
#include <WiFiManager.h>						// Library for ESP WiFi config through an AP
#endif

#if A_OTA==1
#include <ESP8266httpUpdate.h>
#include <ArduinoOTA.h>
#endif

#if A_SERVER==1
#include <ESP8266WebServer.h>
#endif 

#if GATEWAYNODE==1
#include "AES-128_V10.h"
#endif

#if OLED==1
#include "SSD1306.h"
SSD1306  display(OLED_ADDR, OLED_SDA, OLED_SCL);// i2c ADDR & SDA, SCL on wemos
#endif

int debug=1;									// Debug level! 0 is no msgs, 1 normal, 2 extensive

// You can switch webserver off if not necessary but probably better to leave it in.
#if A_SERVER==1
#ifdef ESP32BUILD
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
WebServer server(A_SERVERPORT);
#else
#include <Streaming.h>          				// http://arduiniana.org/libraries/streaming/
  ESP8266WebServer server(A_SERVERPORT);
#endif
#endif
using namespace std;

byte currentMode = 0x81;

//char b64[256];
bool sx1272 = true;								// Actually we use sx1276/RFM95

uint32_t cp_nb_rx_rcv;
uint32_t cp_nb_rx_ok;
uint32_t cp_nb_rx_bad;
uint32_t cp_nb_rx_nocrc;
uint32_t cp_up_pkt_fwd;

uint8_t MAC_array[6];
//char MAC_char[19];

// ----------------------------------------------------------------------------
//
// Configure these values only if necessary!
//
// ----------------------------------------------------------------------------

// Set spreading factor (SF7 - SF12)
sf_t sf 			= _SPREADING;
sf_t sfi 			= _SPREADING;				// Initial value of SF

// Set location, description and other configuration parameters
// Defined in ESP-sc_gway.h
//
float lat			= _LAT;						// Configuration specific info...
float lon			= _LON;
int   alt			= _ALT;
char platform[24]	= _PLATFORM; 				// platform definition
char email[40]		= _EMAIL;    				// used for contact email
char description[64]= _DESCRIPTION;				// used for free form description 

// define servers

IPAddress ntpServer;							// IP address of NTP_TIMESERVER
IPAddress ttnServer;							// IP Address of thethingsnetwork server
IPAddress thingServer;

WiFiUDP Udp;
uint32_t stattime = 0;							// last time we sent a stat message to server
uint32_t pulltime = 0;							// last time we sent a pull_data request to server
uint32_t lastTmst = 0;
#if A_SERVER==1
uint32_t wwwtime = 0;
#endif
#if NTP_INTR==0
uint32_t ntptimer = 0;
#endif

SimpleTimer timer; 								// Timer is needed for delayed sending

#define TX_BUFF_SIZE  1024						// Upstream buffer to send to MQTT
#define RX_BUFF_SIZE  1024						// Downstream received from MQTT
#define STATUS_SIZE	  512						// Should(!) be enough based on the static text .. was 1024

#if GATEWAYNODE==1
uint16_t frameCount=0;							// We write this to SPIFF file
#endif

// volatile bool inSPI This initial value of mutex is to be free,
// which means that its value is 1 (!)
// 
int inSPI = 1;
int inSPO = 1;
int mutexSPI = 1;

//volatile bool inIntr = false;
int inIntr;

// ----------------------------------------------------------------------------
// FORWARD DECARATIONS
// These forward declarations are done since _loraModem.ino is linked by the
// compiler/linker AFTER the main ESP-sc-gway.ino file. 
// And espcesially when calling functions with ICACHE_RAM_ATTR the complier 
// does not want this.
// ----------------------------------------------------------------------------

void ICACHE_RAM_ATTR Interrupt_0();
void ICACHE_RAM_ATTR Interrupt_1();

#if MUTEX==1
// Forward declarations
void ICACHE_FLASH_ATTR CreateMutux(int *mutex);
bool ICACHE_FLASH_ATTR GetMutex(int *mutex);
void ICACHE_FLASH_ATTR ReleaseMutex(int *mutex);
#endif

// ----------------------------------------------------------------------------
// DIE is not use actively in the source code anymore.
// It is replaced by a Serial.print command so we know that we have a problem
// somewhere.
// There are at least 3 other ways to restart the ESP. Pick one if you want.
// ----------------------------------------------------------------------------
void die(const char *s)
{
	Serial.println(s);
	if (debug>=2) Serial.flush();

	delay(50);
	// system_restart();						// SDK function
	// ESP.reset();				
	abort();									// Within a second
}

// ----------------------------------------------------------------------------
// gway_failed is a function called by ASSERT in ESP-sc-gway.h
//
// ----------------------------------------------------------------------------
void gway_failed(const char *file, uint16_t line) {
	Serial.print(F("Program failed in file: "));
	Serial.print(file);
	Serial.print(F(", line: "));
	Serial.println(line);
	if (debug>=2) Serial.flush();
}

// ----------------------------------------------------------------------------
// Print leading '0' digits for hours(0) and second(0) when
// printing values less than 10
// ----------------------------------------------------------------------------
void printDigits(unsigned long digits)
{
    // utility function for digital clock display: prints leading 0
    if(digits < 10)
        Serial.print(F("0"));
    Serial.print(digits);
}

// ----------------------------------------------------------------------------
// Print utin8_t values in HEX with leading 0 when necessary
// ----------------------------------------------------------------------------
void printHexDigit(uint8_t digit)
{
    // utility function for printing Hex Values with leading 0
    if(digit < 0x10)
        Serial.print('0');
    Serial.print(digit,HEX);
}


// ----------------------------------------------------------------------------
// Print the current time
// ----------------------------------------------------------------------------
static void printTime() {
	switch (weekday())
	{
		case 1: Serial.print(F("Sunday")); break;
		case 2: Serial.print(F("Monday")); break;
		case 3: Serial.print(F("Tuesday")); break;
		case 4: Serial.print(F("Wednesday")); break;
		case 5: Serial.print(F("Thursday")); break;
		case 6: Serial.print(F("Friday")); break;
		case 7: Serial.print(F("Saturday")); break;
		default: Serial.print(F("ERROR")); break;
	}
	Serial.print(F(" "));
	printDigits(hour());
	Serial.print(F(":"));
	printDigits(minute());
	Serial.print(F(":"));
	printDigits(second());
	return;
}


// ----------------------------------------------------------------------------
// Convert a float to string for printing
// Parameters:
//	f is float value to convert
//	p is precision in decimal digits
//	val is character array for results
// ----------------------------------------------------------------------------
void ftoa(float f, char *val, int p) {
	int j=1;
	int ival, fval;
	char b[7] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	
	for (int i=0; i< p; i++) { j= j*10; }

	ival = (int) f;								// Make integer part
	fval = (int) ((f- ival)*j);					// Make fraction. Has same sign as integer part
	if (fval<0) fval = -fval;					// So if it is negative make fraction positive again.
												// sprintf does NOT fit in memory
	strcat(val,itoa(ival,b,10));				// Copy integer part first, base 10, null terminated
	strcat(val,".");							// Copy decimal point
	
	itoa(fval,b,10);							// Copy fraction part base 10
	for (int i=0; i<(p-strlen(b)); i++) {
		strcat(val,"0"); 						// first number of 0 of faction?
	}
	
	// Fraction can be anything from 0 to 10^p , so can have less digits
	strcat(val,b);
}

// ============================================================================
// NTP TIME functions



// ----------------------------------------------------------------------------
// Send the request packet to the NTP server.
//
// ----------------------------------------------------------------------------
int sendNtpRequest(IPAddress timeServerIP) {
	const int NTP_PACKET_SIZE = 48;				// Fixed size of NTP record
	byte packetBuffer[NTP_PACKET_SIZE];

	memset(packetBuffer, 0, NTP_PACKET_SIZE);	// Zeroise the buffer.
	
	packetBuffer[0]  = 0b11100011;   			// LI, Version, Mode
	packetBuffer[1]  = 0;						// Stratum, or type of clock
	packetBuffer[2]  = 6;						// Polling Interval
	packetBuffer[3]  = 0xEC;					// Peer Clock Precision
	// 8 bytes of zero for Root Delay & Root Dispersion
	packetBuffer[12] = 49;
	packetBuffer[13] = 0x4E;
	packetBuffer[14] = 49;
	packetBuffer[15] = 52;	

	
	if (!sendUdp( (IPAddress) timeServerIP, (int) 123, packetBuffer, NTP_PACKET_SIZE)) {
		gwayConfig.ntpErr++;
		gwayConfig.ntpErrTime = millis();
		return(0);	
	}
	return(1);
}


// ----------------------------------------------------------------------------
// Get the NTP time from one of the time servers
// Note: As this function is called from SyncINterval in the background
//	make sure we have no blocking calls in this function
// ----------------------------------------------------------------------------
time_t getNtpTime()
{
	gwayConfig.ntps++;
	
    if (!sendNtpRequest(ntpServer))					// Send the request for new time
	{
		if (debug>0) Serial.println(F("sendNtpRequest failed"));
		return(0);
	}
	
	const int NTP_PACKET_SIZE = 48;					// Fixed size of NTP record
	byte packetBuffer[NTP_PACKET_SIZE];
	memset(packetBuffer, 0, NTP_PACKET_SIZE);		// Set buffer cntents to zero

    uint32_t beginWait = millis();
	delay(10);
    while (millis() - beginWait < 1500) 
	{
		int size = Udp.parsePacket();
		if ( size >= NTP_PACKET_SIZE ) {
		
			if (Udp.read(packetBuffer, NTP_PACKET_SIZE) < NTP_PACKET_SIZE) {
				break;
			}
			else {
				// Extract seconds portion.
				unsigned long secs;
				secs  = packetBuffer[40] << 24;
				secs |= packetBuffer[41] << 16;
				secs |= packetBuffer[42] <<  8;
				secs |= packetBuffer[43];\
				// UTC is 1 TimeZone correction when no daylight saving time
				return(secs - 2208988800UL + NTP_TIMEZONES * SECS_PER_HOUR);
			}
			Udp.flush();	
		}
		delay(100);									// Wait 100 millisecs, allow kernel to act when necessary
    }

	Udp.flush();
	
	// If we are here, we could not read the time from internet
	// So increase the counter
	gwayConfig.ntpErr++;
	gwayConfig.ntpErrTime = millis();
	if (debug>0) Serial.println(F("getNtpTime:: read failed"));
	return(0); 										// return 0 if unable to get the time
}

// ----------------------------------------------------------------------------
// Set up regular synchronization of NTP server and the local time.
// ----------------------------------------------------------------------------
#if NTP_INTR==1
void setupTime() {
  setSyncProvider(getNtpTime);
  setSyncInterval(_NTP_INTERVAL);
}
#endif


// ============================================================================
// UDP AND WLAN FUNCTIONS


// ----------------------------------------------------------------------------
// config.txt is a text file that contains lines(!) with WPA configuration items
// Each line contains an KEY vaue pair describing the gateway configuration
//
// ----------------------------------------------------------------------------
int WlanReadWpa() {
	
	readConfig( CONFIGFILE, &gwayConfig);

	if (gwayConfig.sf != (uint8_t) 0) sf = (sf_t) gwayConfig.sf;
	ifreq = gwayConfig.ch;
	debug = gwayConfig.debug;
	_cad = gwayConfig.cad;
	_hop = gwayConfig.hop;
	gwayConfig.boots++;							// Every boot of the system we increase the reset
	
#if GATEWAYNODE==1
	if (gwayConfig.fcnt != (uint8_t) 0) frameCount = gwayConfig.fcnt+10;
#endif
	
#if WIFIMANAGER > 0
	String ssid=gwayConfig.ssid;
	String pass=gwayConfig.pass;

	char ssidBuf[ssid.length()+1];
	ssid.toCharArray(ssidBuf,ssid.length()+1);
	char passBuf[pass.length()+1];
	pass.toCharArray(passBuf,pass.length()+1);
	Serial.print(F("WlanReadWpa: ")); Serial.print(ssidBuf); Serial.print(F(", ")); Serial.println(passBuf);
	
	strcpy(wpa[0].login, ssidBuf);				// XXX changed from wpa[0][0] = ssidBuf
	strcpy(wpa[0].passw, passBuf);
	
	Serial.print(F("WlanReadWpa: <")); 
	Serial.print(wpa[0].login); 				// XXX
	Serial.print(F(">, <")); 
	Serial.print(wpa[0].passw);
	Serial.println(F(">"));
#endif

}

// ----------------------------------------------------------------------------
// Print the WPA data of last WiFiManager to file
// ----------------------------------------------------------------------------
#if WIFIMANAGER==1
int WlanWriteWpa( char* ssid, char *pass) {

	Serial.print(F("WlanWriteWpa:: ssid=")); 
	Serial.print(ssid);
	Serial.print(F(", pass=")); 
	Serial.print(pass); 
	Serial.println();
	
	// Version 3.3 use of config file
	String s((char *) ssid);
	gwayConfig.ssid = s;
	
	String p((char *) pass);
	gwayConfig.pass = p;

#if GATEWAYNODE==1	
	gwayConfig.fcnt = frameCount;
#endif
	gwayConfig.ch = ifreq;
	gwayConfig.sf = sf;
	gwayConfig.cad = _cad;
	gwayConfig.hop = _hop;
	
	writeConfig( CONFIGFILE, &gwayConfig);
	return 1;
}
#endif

// ----------------------------------------------------------------------------
// Function to join the Wifi Network
//	It is a matter of returning to the main loop() asap and make sure in next loop
//	the reconnect is done first thing.
// Parameters:
//		int maxTtry: Number of reties we do:
//		0: Try forever. Which is normally what we want except for Setup maybe
//		1: Try once and if unsuccessful return(0);
//		x: Try x times
//
//  Returns:
//		On failure: Return -1
//		int number of retries necessary
// ----------------------------------------------------------------------------
int WlanConnect(int maxTry) {
  
#if WIFIMANAGER==1
	WiFiManager wifiManager;
#endif

	unsigned char agains = 0;
	unsigned char wpa_index = (WIFIMANAGER >0 ? 0 : 1);		// Skip over first record for WiFiManager
	
	// So try to connect to WLAN as long as we are not connected.
	// The try parameters tells us how many times we try before giving up
	int i=0;

	// We try 5 times before giving up on connect
	while ( (WiFi.status() != WL_CONNECTED) && ( i< maxTry ) )
	{

	  // We try every SSID in wap array until success
	  for (int j=wpa_index; j< (sizeof(wpa)/sizeof(wpa[0])) ; j++) {
	 
		// Start with well-known access points in the list
		char *ssid		= wpa[j].login;
		char *password	= wpa[j].passw;
	
		Serial.print(i);
		Serial.print(':');
		Serial.print(j); 
		Serial.print(F(". WiFi connect to: ")); 
		Serial.println(ssid);
		
		// Count the number of times we call WiFi.begin
		gwayConfig.wifis++;
		writeGwayCfg(CONFIGFILE);
		
		WiFi.begin(ssid, password);
		
		// We increase the time for connect but try the same SSID
		// We try for 9 times
		agains=0;
		while ((WiFi.status() != WL_CONNECTED) && (agains < 9)) {
			delay(agains*400);
			agains++;
			if (debug>=2) Serial.print(".");
		}
		if (WiFi.status() == WL_CONNECTED) 
			break;
		else 
			WiFi.disconnect();
	  } //for
	  i++;													// Number of times we try to connect
	}
	
#if DUSB>=1
	if (i>0) {
		Serial.print(F("WLAN reconnected"));
		Serial.println();
	}
#endif
  
	// Still not connected?
	if (WiFi.status() != WL_CONNECTED) {
#if WIFIMANAGER==1
		Serial.println(F("Starting Access Point Mode"));
		Serial.print(F("Connect Wifi to accesspoint: "));
		Serial.print(AP_NAME);
		Serial.print(F(" and connect to IP: 192.168.4.1"));
		Serial.println();
		wifiManager.autoConnect(AP_NAME, AP_PASSWD );
		//wifiManager.startConfigPortal(AP_NAME, AP_PASSWD );
		// At this point, there IS a Wifi Access Point found and connected
		// We must connect to the local SPIFFS storage to store the access point
		//String s = WiFi.SSID();
		//char ssidBuf[s.length()+1];
		//s.toCharArray(ssidBuf,s.length()+1);
		// Now look for the password
		struct station_config sta_conf;
		wifi_station_get_config(&sta_conf);

		//WlanWriteWpa(ssidBuf, (char *)sta_conf.password);
		WlanWriteWpa((char *)sta_conf.ssid, (char *)sta_conf.password);
#else
		return(-1);
#endif
	}

	yield();
  
	return(1);
}


// ----------------------------------------------------------------------------
// Read DOWN a package from UDP socket, can come from any server
// Messages are received when server responds to gateway requests from LoRa nodes 
// (e.g. JOIN requests etc.) or when server has downstream data.
// We repond only to the server that sent us a message!
// Note: So normally we can forget here about codes that do upstream
// Parameters:
//	Packetsize: size of the buffer to read, as read by loop() calling function
//
// Returns:
//	-1 or false if not read
//	Or number of characters read is success
//
// ----------------------------------------------------------------------------
int readUdp(int packetSize)
{
	uint8_t protocol;
	uint16_t token;
	uint8_t ident; 
	uint8_t buff[32]; 						// General buffer to use for UDP, set to 64
	uint8_t buff_down[RX_BUFF_SIZE];		// Buffer for downstream

	if (WlanConnect(10) < 0) {
#if DUSB>=1
			Serial.print(F("readdUdp: ERROR connecting to WLAN"));
			if (debug>=2) Serial.flush();
#endif
			Udp.flush();
			yield();
			return(-1);
	}

	yield();
	
	if (packetSize > RX_BUFF_SIZE) {
#if DUSB>=1
		Serial.print(F("readUDP:: ERROR package of size: "));
		Serial.println(packetSize);
#endif
		Udp.flush();
		return(-1);
	}
  
	// We assume here that we know the originator of the message
	// In practice however this can be any sender!
	if (Udp.read(buff_down, packetSize) < packetSize) {
#if DUSB>=1
		Serial.println(F("readUsb:: Reading less chars"));
		return(-1);
#endif
	}

	// Remote Address should be known
	IPAddress remoteIpNo = Udp.remoteIP();

	// Remote port is either of the remote TTN server or from NTP server (=123)
	unsigned int remotePortNo = Udp.remotePort();

	if (remotePortNo == 123) {
		// This is an NTP message arriving
#if DUSB>=1
		if (debug>0) {
			Serial.println(F("readUdp:: NTP msg rcvd"));
		}
#endif
		gwayConfig.ntpErr++;
		gwayConfig.ntpErrTime = millis();
		return(0);
	}
	
	// If it is not NTP it must be a LoRa message for gateway or node
	else {
		uint8_t *data = (uint8_t *) ((uint8_t *)buff_down + 4);
		protocol = buff_down[0];
		token = buff_down[2]*256 + buff_down[1];
		ident = buff_down[3];

		// now parse the message type from the server (if any)
		switch (ident) {

		// This message is used by the gateway to send sensor data to the
		// server. As this function is used for downstream only, this option
		// will never be selected but is included as a reference only
		case PKT_PUSH_DATA: // 0x00 UP
#if DUSB>=1
			if (debug >=1) {
				Serial.print(F("PKT_PUSH_DATA:: size ")); Serial.print(packetSize);
				Serial.print(F(" From ")); Serial.print(remoteIpNo);
				Serial.print(F(", port ")); Serial.print(remotePortNo);
				Serial.print(F(", data: "));
				for (int i=0; i<packetSize; i++) {
					Serial.print(buff_down[i],HEX);
					Serial.print(':');
				}
				Serial.println();
				if (debug>=2) Serial.flush();
			}
#endif
		break;
	
		// This message is sent by the server to acknoledge receipt of a
		// (sensor) message sent with the code above.
		case PKT_PUSH_ACK:	// 0x01 DOWN
#if DUSB>=1
			if (debug >= 2) {
				Serial.print(F("PKT_PUSH_ACK:: size ")); 
				Serial.print(packetSize);
				Serial.print(F(" From ")); 
				Serial.print(remoteIpNo);
				Serial.print(F(", port ")); 
				Serial.print(remotePortNo);
				Serial.print(F(", token: "));
				Serial.println(token, HEX);
				Serial.println();
			}
#endif
		break;
	
		case PKT_PULL_DATA:	// 0x02 UP
#if DUSB>=1
			Serial.print(F(" Pull Data"));
			Serial.println();
#endif
		break;
	
		// This message type is used to confirm OTAA message to the node
		// XXX This message format may also be used for other downstream communucation
		case PKT_PULL_RESP:	// 0x03 DOWN
#if DUSB>=1
			if (debug>=0) {
				Serial.println(F("PKT_PULL_RESP:: received"));
			}
#endif
			lastTmst = micros();					// Store the tmst this package was received
			
			// Send to the LoRa Node first (timing) and then do messaging
			_state=S_TX;
			if (sendPacket(data, packetSize-4) < 0) {
				return(-1);
			}
		
			// Now respond with an PKT_TX_ACK; 0x04 UP
			buff[0]=buff_down[0];
			buff[1]=buff_down[1];
			buff[2]=buff_down[2];
			//buff[3]=PKT_PULL_ACK;				// Pull request/Change of Mogyi
			buff[3]=PKT_TX_ACK;
			buff[4]=MAC_array[0];
			buff[5]=MAC_array[1];
			buff[6]=MAC_array[2];
			buff[7]=0xFF;
			buff[8]=0xFF;
			buff[9]=MAC_array[3];
			buff[10]=MAC_array[4];
			buff[11]=MAC_array[5];
			buff[12]=0;
#if DUSB>=1
			Serial.println(F("readUdp:: TX buff filled"));
#endif
			// Only send the PKT_PULL_ACK to the UDP socket that just sent the data!!!
			Udp.beginPacket(remoteIpNo, remotePortNo);
#ifdef ESP32BUILD
			if (Udp.write(buff, 12) != 12) {
#else
      if (Udp.write((char *)buff, 12) != 12) {
#endif
#if DUSB>=1
				if (debug>=0)
					Serial.println("PKT_PULL_ACK:: Error UDP write");
#endif
			}
			else {
#if DUSB>=1
				if (debug>=0) {
					Serial.print(F("PKT_TX_ACK:: tmst="));
					Serial.println(micros());
				}
#endif
			}

			if (!Udp.endPacket()) {
#if DUSB>=1
				if (debug>=0)
					Serial.println(F("PKT_PULL_DATALL Error Udp.endpaket"));
#endif
			}
			
			yield();
#if DUSB>=1
			if (debug >=1) {
				Serial.print(F("PKT_PULL_RESP:: size ")); 
				Serial.print(packetSize);
				Serial.print(F(" From ")); 
				Serial.print(remoteIpNo);
				Serial.print(F(", port ")); 
				Serial.print(remotePortNo);	
				Serial.print(F(", data: "));
				data = buff_down + 4;
				data[packetSize] = 0;
				Serial.print((char *)data);
				Serial.println(F("..."));
			}
#endif		
		break;
	
		case PKT_PULL_ACK:	// 0x04 DOWN; the server sends a PULL_ACK to confirm PULL_DATA receipt
#if DUSB>=1
			if (debug >= 2) {
				Serial.print(F("PKT_PULL_ACK:: size ")); Serial.print(packetSize);
				Serial.print(F(" From ")); Serial.print(remoteIpNo);
				Serial.print(F(", port ")); Serial.print(remotePortNo);	
				Serial.print(F(", data: "));
				for (int i=0; i<packetSize; i++) {
					Serial.print(buff_down[i],HEX);
					Serial.print(':');
				}
				Serial.println();
			}
#endif
		break;
	
		default:
#if GATEWAYMGT==1
			// For simplicity, we send the first 4 bytes too
			gateway_mgt(packetSize, buff_down);
#else

#endif
#if DUSB>=1
			Serial.print(F(", ERROR ident not recognized="));
			Serial.println(ident);
#endif
		break;
		}
#if DUSB>=2
		if (debug>=1) {
			Serial.print(F("readUdp:: returning=")); 
			Serial.println(packetSize);
		}
#endif
		// For downstream messages
		return packetSize;
	}
}//readUdp


// ----------------------------------------------------------------------------
// Send UP an UDP/DGRAM message to the MQTT server
// If we send to more than one host (not sure why) then we need to set sockaddr 
// before sending.
// Parameters:
//	IPAddress
//	port
//	msg *
//	length (of msg)
// return values:
//	0: Error
//	1: Success
// ----------------------------------------------------------------------------
int sendUdp(IPAddress server, int port, uint8_t *msg, int length) {

	// Check whether we are conected to Wifi and the internet
	if (WlanConnect(3) < 0) {
#if DUSB>=1
		Serial.print(F("sendUdp: ERROR connecting to WiFi"));
		Serial.flush();
#endif
		Udp.flush();
		yield();
		return(0);
	}

	yield();

	//send the update
#if DUSB>=1
  if (debug>=2) Serial.println(F("WiFi connected"));
#endif	
	if (!Udp.beginPacket(server, (int) port)) {
#if DUSB>=1
		if (debug>=1) Serial.println(F("sendUdp:: Error Udp.beginPacket"));
#endif
		return(0);
	}
	
	yield();
	
#ifdef ESP32BUILD
	if (Udp.write(msg, length) != length) {
#else
  if (Udp.write((char *)msg, length) != length) {
#endif
#if DUSB>=1
		Serial.println(F("sendUdp:: Error write"));
#endif
		Udp.endPacket();						// Close UDP
		return(0);								// Return error
	}
	
	yield();
	
	if (!Udp.endPacket()) {
#if DUSB>=1
		if (debug>=1) {
			Serial.println(F("sendUdp:: Error Udp.endPacket"));
			Serial.flush();
		}
#endif
		return(0);
	}
	return(1);
}//sendUDP

// ----------------------------------------------------------------------------
// UDPconnect(): connect to UDP (which is a local thing, after all UDP 
// connections do not exist.
// Parameters:
//	<None>
// Returns
//	Boollean indicating success or not
// ----------------------------------------------------------------------------
bool UDPconnect() {

	bool ret = false;
	unsigned int localPort = _LOCUDPPORT;			// To listen to return messages from WiFi
#if DUSB>=1
	if (debug>=1) {
		Serial.print(F("Local UDP port="));
		Serial.println(localPort);
	}
#endif	
	if (Udp.begin(localPort) == 1) {
#if DUSB>=1
		if (debug>=1) Serial.println(F("Connection successful"));
#endif
		ret = true;
	}
	else{
#if DUSB>=1
		if (debug>=1) Serial.println("Connection failed");
#endif
	}
	return(ret);
}//udpConnect


// ----------------------------------------------------------------------------
// Send UP periodic Pull_DATA message to server to keepalive the connection
// and to invite the server to send downstream messages when these are available
// *2, par. 5.2
//	- Protocol Version (1 byte)
//	- Random Token (2 bytes)
//	- PULL_DATA identifier (1 byte) = 0x02
//	- Gateway unique identifier (8 bytes) = MAC address
// ----------------------------------------------------------------------------
void pullData() {

    uint8_t pullDataReq[12]; 								// status report as a JSON object
    int pullIndex=0;
	int i;
	
	uint8_t token_h = (uint8_t)rand(); 						// random token
    uint8_t token_l = (uint8_t)rand();						// random token
	
    // pre-fill the data buffer with fixed fields
    pullDataReq[0]  = PROTOCOL_VERSION;						// 0x01
    pullDataReq[1]  = token_h;
    pullDataReq[2]  = token_l;
    pullDataReq[3]  = PKT_PULL_DATA;						// 0x02
	// READ MAC ADDRESS OF ESP8266, and return unique Gateway ID consisting of MAC address and 2bytes 0xFF
    pullDataReq[4]  = MAC_array[0];
    pullDataReq[5]  = MAC_array[1];
    pullDataReq[6]  = MAC_array[2];
    pullDataReq[7]  = 0xFF;
    pullDataReq[8]  = 0xFF;
    pullDataReq[9]  = MAC_array[3];
    pullDataReq[10] = MAC_array[4];
    pullDataReq[11] = MAC_array[5];
    //pullDataReq[12] = 0/00; 								// add string terminator, for safety
	
    pullIndex = 12;											// 12-byte header
	
    //send the update
	
	uint8_t *pullPtr;
	pullPtr = pullDataReq,
#ifdef _TTNSERVER
    sendUdp(ttnServer, _TTNPORT, pullDataReq, pullIndex);
	yield();
#endif

#if DUSB>=1
	if (pullPtr != pullDataReq) {
		Serial.println(F("pullPtr != pullDatReq"));
		Serial.flush();
	}

#endif
#ifdef _THINGSERVER
	sendUdp(thingServer, _THINGPORT, pullDataReq, pullIndex);
#endif

#if DUSB>=1
    if (debug>= 2) {
		yield();
		Serial.print(F("PKT_PULL_DATA request, len=<"));
		Serial.print(pullIndex);
		Serial.print(F("> "));
		for (i=0; i<pullIndex; i++) {
			Serial.print(pullDataReq[i],HEX);				// DEBUG: display JSON stat
			Serial.print(':');
		}
		Serial.println();
		if (debug>=2) Serial.flush();
	}
#endif
	return;
}//pullData


// ----------------------------------------------------------------------------
// Send UP periodic status message to server even when we do not receive any
// data. 
// Parameters:
//	- <none>
// ----------------------------------------------------------------------------
void sendstat() {

    uint8_t status_report[STATUS_SIZE]; 					// status report as a JSON object
    char stat_timestamp[32];								// XXX was 24
    time_t t;
	char clat[10]={0};
	char clon[10]={0};

    int stat_index=0;
	uint8_t token_h   = (uint8_t)rand(); 					// random token
    uint8_t token_l   = (uint8_t)rand();					// random token
	
    // pre-fill the data buffer with fixed fields
    status_report[0]  = PROTOCOL_VERSION;					// 0x01
	status_report[1]  = token_h;
    status_report[2]  = token_l;
    status_report[3]  = PKT_PUSH_DATA;						// 0x00
	
	// READ MAC ADDRESS OF ESP8266, and return unique Gateway ID consisting of MAC address and 2bytes 0xFF
    status_report[4]  = MAC_array[0];
    status_report[5]  = MAC_array[1];
    status_report[6]  = MAC_array[2];
    status_report[7]  = 0xFF;
    status_report[8]  = 0xFF;
    status_report[9]  = MAC_array[3];
    status_report[10] = MAC_array[4];
    status_report[11] = MAC_array[5];

    stat_index = 12;										// 12-byte header
	
    t = now();												// get timestamp for statistics
	
	// XXX Using CET as the current timezone. Change to your timezone	
	sprintf(stat_timestamp, "%04d-%02d-%02d %02d:%02d:%02d CET", year(),month(),day(),hour(),minute(),second());
	yield();
	
	ftoa(lat,clat,5);										// Convert lat to char array with 5 decimals
	ftoa(lon,clon,5);										// As IDE CANNOT prints floats
	
	// Build the Status message in JSON format, XXX Split this one up...
	delay(1);
	
    int j = snprintf((char *)(status_report + stat_index), STATUS_SIZE-stat_index, 
		"{\"stat\":{\"time\":\"%s\",\"lati\":%s,\"long\":%s,\"alti\":%i,\"rxnb\":%u,\"rxok\":%u,\"rxfw\":%u,\"ackr\":%u.0,\"dwnb\":%u,\"txnb\":%u,\"pfrm\":\"%s\",\"mail\":\"%s\",\"desc\":\"%s\"}}", 
		stat_timestamp, clat, clon, (int)alt, cp_nb_rx_rcv, cp_nb_rx_ok, cp_up_pkt_fwd, 0, 0, 0, platform, email, description);
		
	yield();												// Give way to the internal housekeeping of the ESP8266

    stat_index += j;
    status_report[stat_index] = 0; 							// add string terminator, for safety

    if (debug>=2) {
		Serial.print(F("stat update: <"));
		Serial.print(stat_index);
		Serial.print(F("> "));
		Serial.println((char *)(status_report+12));			// DEBUG: display JSON stat
	}
	
	if (stat_index > STATUS_SIZE) {
		Serial.println(F("sendstat:: ERROR buffer too big"));
		return;
	}
	
    //send the update
#ifdef _TTNSERVER
    sendUdp(ttnServer, _TTNPORT, status_report, stat_index);
	yield();
#endif

#ifdef _THINGSERVER
	sendUdp(thingServer, _THINGPORT, status_report, stat_index);
#endif
	return;
}//sendstat



// ============================================================================
// MAIN PROGRAM CODE (SETUP AND LOOP)


// ----------------------------------------------------------------------------
// Setup code (one time)
// _state is S_INIT
// ----------------------------------------------------------------------------
void setup() {

	char MAC_char[19];								// XXX Unbelievable
	MAC_char[18] = 0;
	
	
	
	Serial.begin(_BAUDRATE);						// As fast as possible for bus
	delay(100);
	Serial.flush();
	delay(500);
#if MUTEX_SPI==1
	CreateMutux(&inSPI);
#endif
#if MUTEX_SPO==1
	CreateMutux(&inSPO);
#endif
#if MUTEX_INT==1
	CreateMutux(&inIntr);
#endif
	if (SPIFFS.begin()) Serial.println(F("SPIFFS loaded success"));

	Serial.print(F("Assert="));
#if defined CFG_noassert
	Serial.println(F("No Asserts"));
#else
	Serial.println(F("Do Asserts"));
#endif

#if OLED==1
	// Initialising the UI will init the display too.
#ifdef ESP32BUILD
  pinMode(16,OUTPUT);
  digitalWrite(16, LOW);    // set GPIO16 low to reset OLED
  delay(50);
  digitalWrite(16, HIGH); // while OLED is running, must set GPIO16 in high、
#endif
  display.init();
	display.flipScreenVertically();
	display.setFont(ArialMT_Plain_24);
	display.setTextAlignment(TEXT_ALIGN_LEFT);
	display.drawString(0, 24, "STARTING");
	display.display();
#endif

	delay(500);
	yield();
#if DUSB>=1	
	if (debug>=1) {
		Serial.print(F("debug=")); 
		Serial.println(debug);
		yield();
	}
#endif
	WiFi.mode(WIFI_STA);
	WlanReadWpa();								// Read the last Wifi settings from SPIFFS into memory

	WiFi.macAddress(MAC_array);
	
    sprintf(MAC_char,"%02x:%02x:%02x:%02x:%02x:%02x",
		MAC_char[0],MAC_array[1],MAC_char[2],MAC_array[3],MAC_char[4],MAC_array[5]);
	Serial.print("MAC: ");
    Serial.print(MAC_char);
	Serial.print(F(", len="));
	Serial.println(strlen(MAC_char));
	
	// We start by connecting to a WiFi network, set hostname
	char hostname[12];
#ifdef ESP32BUILD
  sprintf(hostname, "%s%02x%02x%02x", "esp32-", MAC_array[3], MAC_array[4], MAC_array[5]);
#else
	sprintf(hostname, "%s%02x%02x%02x", "esp8266-", MAC_array[3], MAC_array[4], MAC_array[5]);
#endif

#ifdef ESP32BUILD
  WiFi.setHostname(hostname);
#else
  wifi_station_set_hostname( hostname );
#endif
	
	// Setup WiFi UDP connection. Give it some time and retry 50 times..
	while (WlanConnect(50) < 0) {
		Serial.print(F("Error Wifi network connect "));
		Serial.println();
		yield();
	}
	
	Serial.print(F("Host "));
#ifdef ESP32BUILD
  Serial.print(WiFi.getHostname());
#else
  Serial.print(wifi_station_get_hostname());
#endif
	Serial.print(F(" WiFi Connected to "));
	Serial.print(WiFi.SSID());
	Serial.println();
	delay(200);
	
	// If we are here we are connected to WLAN
	// So now test the UDP function
	if (!UDPconnect()) {
		Serial.println(F("Error UDPconnect"));
	}
	delay(200);
	
	// Pins are defined and set in loraModem.h
  pinMode(pins.ss, OUTPUT);
	pinMode(pins.rst, OUTPUT);
  pinMode(pins.dio0, INPUT);								// This pin is interrupt
	pinMode(pins.dio1, INPUT);								// This pin is interrupt
	//pinMode(pins.dio2, INPUT);

	// Init the SPI pins
#ifdef ESP32BUILD
  SPI.begin(SCK,MISO,MOSI,SS);
#else
	SPI.begin();
#endif
	
	delay(500);
	
	// We choose the Gateway ID to be the Ethernet Address of our Gateway card
    // display results of getting hardware address
	// 
  Serial.print("Gateway ID: ");
	printHexDigit(MAC_array[0]);
  printHexDigit(MAC_array[1]);
  printHexDigit(MAC_array[2]);
	printHexDigit(0xFF);
	printHexDigit(0xFF);
  printHexDigit(MAC_array[3]);
  printHexDigit(MAC_array[4]);
  printHexDigit(MAC_array[5]);

  Serial.print(", Listening at SF");
	Serial.print(sf);
	Serial.print(" on ");
	Serial.print((double)freq/1000000);
	Serial.println(" Mhz.");

	if (!WiFi.hostByName(NTP_TIMESERVER, ntpServer))		// Get IP address of Timeserver
	{
		die("Setup:: ERROR hostByName NTP");
	};
	delay(100);
#ifdef _TTNSERVER
	if (!WiFi.hostByName(_TTNSERVER, ttnServer))			// Use DNS to get server IP once
	{
		die("Setup:: ERROR hostByName TTN");
	};
	delay(100);
#endif
#ifdef _THINGSERVER
	if (!WiFi.hostByName(_THINGSERVER, thingServer))
	{
		die("Setup:: ERROR hostByName THING");
	}
	delay(100);
#endif

	// The Over the AIr updates are supported when we have a WiFi connection.
	// The NTP time setting does not have to be precise for this function to work.
#if A_OTA==1
	setupOta(hostname);										// Uses wwwServer 
#endif

	// Set the NTP Time
#if NTP_INTR==1
	setupTime();											// Set NTP time host and interval
#else
	//setTime((time_t)getNtpTime());
	while (timeStatus() == timeNotSet) {
		Serial.println(F("setupTime:: Time not set (yet)"));
		delay(500);
		time_t newTime;
		newTime = (time_t)getNtpTime();
		if (newTime != 0) setTime(newTime);
	}
	Serial.print("Time: "); printTime();
	Serial.println();

	writeGwayCfg(CONFIGFILE );
	Serial.println(F("Gateway configuration saved"));
#endif

#if A_SERVER==1	
	// Setup the webserver
	setupWWW();
#endif

	delay(100);												// Wait after setup
	
	// Setup and initialise LoRa state machine of _loramModem.ino
	_state = S_INIT;
	initLoraModem();
	
	if (_cad) {
		_state = S_SCAN;
		cadScanner();										// Always start at SF7
	}
	else { 
		_state = S_RX;
		rxLoraModem();
	}
	LoraUp.payLength = 0;						// Init the length to 0

	// init interrupt handlers, which are shared for GPIO15 / D8, 
	// we switch on HIGH interrupts
	if (pins.dio0 == pins.dio1) {
		//SPI.usingInterrupt(digitalPinToInterrupt(pins.dio0));
		attachInterrupt(pins.dio0, Interrupt_0, RISING);		// Share interrupts
	}
	// Or in the traditional Comresult case
	else {
		//SPI.usingInterrupt(digitalPinToInterrupt(pins.dio0));
		//SPI.usingInterrupt(digitalPinToInterrupt(pins.dio1));
		attachInterrupt(pins.dio0, Interrupt_0, RISING);	// Separate interrupts
		attachInterrupt(pins.dio1, Interrupt_1, RISING);	// Separate interrupts		
	}
	
	writeConfig( CONFIGFILE, &gwayConfig);					// Write config

	// activate OLED display
#if OLED==1
	// Initialising the UI will init the display too.
	display.clear();
	display.setFont(ArialMT_Plain_24);
	display.drawString(0, 24, "READY");
	display.display();
#endif

	Serial.println(F("--------------------------------------"));
}//setup



// ----------------------------------------------------------------------------
// LOOP
// This is the main program that is executed time and time again.
// We need to give way to the backend WiFi processing that 
// takes place somewhere in the ESP8266 firmware and therefore
// we include yield() statements at important points.
//
// Note: If we spend too much time in user processing functions
//	and the backend system cannot do its housekeeping, the watchdog
// function will be executed which means effectively that the 
// program crashes.
// We use yield() a lot to avoid ANY watch dog activity of the program.
//
// NOTE2: For ESP make sure not to do large array declarations in loop();
// ----------------------------------------------------------------------------
void loop ()
{
	uint32_t nowSeconds;
	int packetSize;
	
	nowTime = micros();
	nowSeconds = (uint32_t) millis() /1000;
	
	// check for event value, which means that an interrupt has arrived.
	// In this case we handle the interrupt ( e.g. message received)
	// in userspace in loop().
	//
	if (_event != 0x00) {
		stateMachine();					// start the state machine
		_event = 0;						// reset value
		return;							// Restart loop
	}
	
	// After a quiet period, make sure we reinit the modem.
	// XXX Still have to measure quiet period in stat[0];
	// For the moment we use msgTime
	if ( (((nowTime - statr[0].tmst) / 1000000) > _MSG_INTERVAL ) &&
		(msgTime < statr[0].tmst)) {
#if DUSB>=1
		Serial.print('r');
#endif
		initLoraModem();
		if (_cad) {
			_state = S_SCAN;
			cadScanner();
		}
		else {
			_state = S_RX;
			rxLoraModem();
		}
		writeRegister(REG_IRQ_FLAGS_MASK, (uint8_t) 0x00);
		writeRegister(REG_IRQ_FLAGS, 0xFF);				// Reset all interrupt flags
		msgTime = nowTime;
	}
	
#if A_OTA==1
	// Perform Over the Air (OTA) update if enabled and requested by user.
	// It is important to put this function at the start of loop() as it is
	// not called frequently but it should always run when called.
	//
	yield();
	ArduinoOTA.handle();
#endif

#if A_SERVER==1
	// Handle the WiFi server part of this sketch. Mainly used for administration 
	// and monitoring of the node. This function is important so it is called at the
	// start of the loop() function
	yield();
	server.handleClient();	
#endif


	// If we are not connected, try to connect.
	// We will not read Udp in this loop cycle then
	if (WlanConnect(1) < 0) {
#if DUSB>=1
			Serial.print(F("loop: ERROR reconnect WLAN"));
#endif
			yield();
			return;										// Exit loop if no WLAN connected
	}
	
	// So if we are connected 
	// Receive UDP PUSH_ACK messages from server. (*2, par. 3.3)
	// This is important since the TTN broker will return confirmation
	// messages on UDP for every message sent by the gateway. So we have to consume them..
	// As we do not know when the server will respond, we test in every loop.
	//
	else {
		while( (packetSize = Udp.parsePacket()) > 0) {		// Length of UDP message waiting
#if DUSB>=2
			Serial.println(F("loop:: readUdp calling"));
#endif
			// Packet may be PKT_PUSH_ACK (0x01), PKT_PULL_ACK (0x03) or PKT_PULL_RESP (0x04)
			// This command is found in byte 4 (buffer[3])
			if (readUdp(packetSize) <= 0) {
#if DUSB>=1
				if (debug>0) Serial.println(F("readUDP error"));
#endif
				break;
			}
			// Now we know we succesfull received message from host
			else {
				_event=1;									// Could be done double if more messages received
			}
			//yield();
		}
	}
	
	yield();
	
	
	// The next section is emergency only. If posible we hop() in the state machine.
	// If hopping is enabled, and by lack of timer, we hop()
	// XXX Experimental, 2.5 ms between hops max
	//

	if ((_hop) && (((long)(nowTime - hopTime)) > 7500)) {
	
		if ((_state == S_SCAN) && (sf==SF12)) {
#if DUSB>=1
			if (debug>=1) Serial.println(F("loop:: hop"));
#endif
			hop(); 
		}
		
		// XXX section below does not work without further work. It is the section with the MOST
		// influence on the HOP mode of operation (which is somewhat unexpected)
		// If we keep staying in another state, reset
		else if (((long)(nowTime - hopTime)) > 100000) {
		
			_state= S_RX;		
			rxLoraModem();
			
			hop();

			if (_cad) { 
				_state= S_SCAN; 
				cadScanner(); 
			}
		}
		else if (debug>=3) { Serial.print(F(" state=")); Serial.println(_state); } 
		inHop = false;									// Reset re-entrane protection of HOP
		yield();
	}
	


	// stat PUSH_DATA message (*2, par. 4)
	//	

    if ((nowSeconds - stattime) >= _STAT_INTERVAL) {	// Wake up every xx seconds
#if DUSB>=1
		if (debug>=2) {
			Serial.print(F("STAT <"));
			Serial.flush();
		}
#endif
        sendstat();										// Show the status message and send to server
#if DUSB>=1
		if (debug>=2) {
			Serial.println(F(">"));
			if (debug>=2) Serial.flush();
		}
#endif	

		// If the gateway behaves like a node, we do from time to time
		// send a node message to the backend server.
		// The Gateway nod emessage has nothing to do with the STAT_INTERVAL
		// message but we schedule it in the same frequency.
		//
#if GATEWAYNODE==1
		if (gwayConfig.isNode) {
			// Give way to internal Admin if necessary
			yield();
			
			// If the 1ch gateway is a sensor itself, send the sensor values
			// could be battery but also other status info or sensor info
		
			if (sensorPacket() < 0) {
				Serial.println(F("sensorPacket: Error"));
			}
		}
#endif
		stattime = nowSeconds;
    }
	
	yield();

	
	// send PULL_DATA message (*2, par. 4)
	//
	nowSeconds = (uint32_t) millis() /1000;
    if ((nowSeconds - pulltime) >= _PULL_INTERVAL) {	// Wake up every xx seconds
#if DUSB>=1
		if (debug>=1) {
			Serial.print(F("PULL <"));
			if (debug>=2) Serial.flush();
		}
#endif
        pullData();										// Send PULL_DATA message to server
		initLoraModem();
		if (_cad) {
			_state = S_SCAN;
			cadScanner();
		}
		else {
			_state = S_RX;
			rxLoraModem();
		}
		writeRegister(REG_IRQ_FLAGS_MASK, (uint8_t) 0x00);
		writeRegister(REG_IRQ_FLAGS, 0xFF);				// Reset all interrupt flags
#if DUSB>=1
		if (debug>=1) {
			Serial.println(F(">"));
			if (debug>=2) Serial.flush();
		}
#endif		
		pulltime = nowSeconds;
    }

	
	// If we do our own NTP handling (advisable)
	// We do not use the timer interrupt but use the timing
	// of the loop() itself which is better for SPI
#if NTP_INTR==0
	// Set the time in a manual way. Do not use setSyncProvider
	// as this function may collide with SPI and other interrupts
	yield();
	nowSeconds = (uint32_t) millis() /1000;
	if (nowSeconds - ntptimer >= _NTP_INTERVAL) {
		yield();
		time_t newTime;
		newTime = (time_t)getNtpTime();
		if (newTime != 0) setTime(newTime);
		ntptimer = nowSeconds;
	}
#endif
}
