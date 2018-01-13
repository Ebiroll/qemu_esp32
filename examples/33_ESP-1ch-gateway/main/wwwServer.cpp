// 1-channel LoRa Gateway for ESP8266
// Copyright (c) 2016, 2017 Maarten Westenberg version for ESP8266
// Version 5.0.1
// Date: 2017-11-15
//
// 	based on work done by many people and making use of several libraries.
//
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the MIT License
// which accompanies this distribution, and is available at
// https://opensource.org/licenses/mit-license.php
//
// Author: Maarten Westenberg (mw12554@hotmail.com)
//
// This file contains the webserver code for the ESP Single Channel Gateway.

// Note:
// The ESP Webserver works with Strings to display html content. 
// Care must be taken that not all data is output to the webserver in one string
// as this will use a LOT of memory and possibly kill the heap (cause system
// crash or other unreliable behaviour.
// Instead, output of the various tables on the webpage should be displayed in
// chucks so that strings are limited in size.
// Be aware that using no strings but only sendContent() calls has its own
// disadvantage that these calls take a lot of time and cause the page to be
// displayed like an old typewriter.
// So, the trick is to make chucks that are sent to the website by using
// a response String but not make those Strings too big.
#include <Arduino.h>
#include "ESP-sc-gway.h"
#include <Time.h>

int writeConfig(const char *fn, struct espGwayConfig *c);
int writeGwayCfg(const char *fn);


// ----------------------------------------------------------------------------
// PRINT IP
// Output the 4-byte IP address for easy printing.
// As this function is also used by _otaServer.ino do not put in #define
// ----------------------------------------------------------------------------
static void printIP(IPAddress ipa, const char sep, String& response)
{
	response+=(IPAddress)ipa[0]; response+=sep;
	response+=(IPAddress)ipa[1]; response+=sep;
	response+=(IPAddress)ipa[2]; response+=sep;
	response+=(IPAddress)ipa[3];
}

//
// The remainder of the file ONLY works is A_SERVER=1 is set.
//
#if A_SERVER==1

// ================================================================================
// WEBSERVER DECLARATIONS 

// None at the moment

// ================================================================================
// WEBSERVER FUNCTIONS 


// ----------------------------------------------------------------------------
// Print a HEXadecimal string from a 4-byte char string
//
// ----------------------------------------------------------------------------
static void printHEX(char * hexa, const char sep, String& response) 
{
	char m;
	m = hexa[0]; if (m<016) response+='0'; response += String(m, HEX);  response+=sep;
	m = hexa[1]; if (m<016) response+='0'; response += String(m, HEX);  response+=sep;
	m = hexa[2]; if (m<016) response+='0'; response += String(m, HEX);  response+=sep;
	m = hexa[3]; if (m<016) response+='0'; response += String(m, HEX);  response+=sep;
}

// ----------------------------------------------------------------------------
// stringTime
// Only when RTC is present we print real time values
// t contains number of milli seconds since system started that the event happened.
// So a value of 100 would mean that the event took place 1 minute and 40 seconds ago
// ----------------------------------------------------------------------------
static void stringTime(unsigned long t, String& response) {

	if (t==0) { response += "--"; return; }
	
	// now() gives seconds since 1970
	time_t eventTime = now() - ((millis()-t)/1000);
	byte _hour   = hour(eventTime);
	byte _minute = minute(eventTime);
	byte _second = second(eventTime);
	
	switch(weekday(eventTime)) {
		case 1: response += "Sunday "; break;
		case 2: response += "Monday "; break;
		case 3: response += "Tuesday "; break;
		case 4: response += "Wednesday "; break;
		case 5: response += "Thursday "; break;
		case 6: response += "Friday "; break;
		case 7: response += "Saturday "; break;
	}
	response += String() + day(eventTime) + "-";
	response += String() + month(eventTime) + "-";
	response += String() + year(eventTime) + " ";
	
	if (_hour < 10) response += "0";
	response += String() + _hour + ":";
	if (_minute < 10) response += "0";
	response += String() + _minute + ":";
	if (_second < 10) response += "0";
	response += String() + _second;
}




// ----------------------------------------------------------------------------
// SET ESP8266 WEB SERVER VARIABLES
//
// This funtion implements the WiFI Webserver (very simple one). The purpose
// of this server is to receive simple admin commands, and execute these
// results are sent back to the web client.
// Commands: DEBUG, ADDRESS, IP, CONFIG, GETTIME, SETTIME
// The webpage is completely built response and then printed on screen.
// ----------------------------------------------------------------------------
static void setVariables(const char *cmd, const char *arg) {

	// DEBUG settings; These can be used as a single argument
	if (strcmp(cmd, "DEBUG")==0) {									// Set debug level 0-2
		if (atoi(arg) == 1) {
			debug = (debug+1)%4;
		}	
		else if (atoi(arg) == -1) {
			debug = (debug+3)%4;
		}
		writeGwayCfg(CONFIGFILE);									// Save configuration to file
	}
	
	if (strcmp(cmd, "CAD")==0) {									// Set -cad on=1 or off=0
		_cad=(bool)atoi(arg);
		writeGwayCfg(CONFIGFILE);									// Save configuration to file
	}
	
	if (strcmp(cmd, "HOP")==0) {									// Set -hop on=1 or off=0
		_hop=(bool)atoi(arg);
		if (! _hop) { 
			ifreq=0; 
			freq=freqs[0]; 
			rxLoraModem(); 
		}
		writeGwayCfg(CONFIGFILE);									// Save configuration to file
	}
	
	if (strcmp(cmd, "DELAY")==0) {									// Set delay usecs
		txDelay+=atoi(arg)*1000;
	}
	
	// SF; Handle Spreading Factor Settings
	if (strcmp(cmd, "SF")==0) {
		uint8_t sfi = sf;
		if (atoi(arg) == 1) {
			if (sf>=SF12) sf=SF7; else sf= (sf_t)((int)sf+1);
		}	
		else if (atoi(arg) == -1) {
			if (sf<=SF7) sf=SF12; else sf= (sf_t)((int)sf-1);
		}
		rxLoraModem();												// Reset the radion with the new spreading factor
		writeGwayCfg(CONFIGFILE);									// Save configuration to file
	}
	
	// FREQ; Handle Frequency  Settings
	if (strcmp(cmd, "FREQ")==0) {
		uint8_t nf = sizeof(freqs)/sizeof(int);						// Number of elements in array
		
		// Compute frequency index
		if (atoi(arg) == 1) {
			if (ifreq==(nf-1)) ifreq=0; else ifreq++;
		}
		else if (atoi(arg) == -1) {
			Serial.println("down");
			if (ifreq==0) ifreq=(nf-1); else ifreq--;
		}

		freq = freqs[ifreq];
		rxLoraModem();												// Reset the radion with the new frequency
		writeGwayCfg(CONFIGFILE);									// Save configuration to file
	}

	//if (strcmp(cmd, "GETTIME")==0) { Serial.println(F("gettime tbd")); }	// Get the local time
	
	//if (strcmp(cmd, "SETTIME")==0) { Serial.println(F("settime tbd")); }	// Set the local time
	
	if (strcmp(cmd, "HELP")==0)    { Serial.println(F("Display Help Topics")); }
	
#if GATEWAYNODE==1
	if (strcmp(cmd, "NODE")==0) {									// Set node on=1 or off=0
		gwayConfig.isNode =(bool)atoi(arg);
		writeGwayCfg(CONFIGFILE);									// Save configuration to file
	}
	
	if (strcmp(cmd, "FCNT")==0)   { 
		frameCount=0; 
		rxLoraModem();												// Reset the radion with the new frequency
		writeGwayCfg(CONFIGFILE);
	}
#endif
	
#if WIFIMANAGER==1
	if (strcmp(cmd, "NEWSSID")==0) { 
		WiFiManager wifiManager;
		strcpy(wpa[0].login,""); 
		strcpy(wpa[0].passw,"");
		WiFi.disconnect();
		wifiManager.autoConnect(AP_NAME, AP_PASSWD );
	}
#endif

#if A_OTA==1
	if (strcmp(cmd, "UPDATE")==0) {
		if (atoi(arg) == 1) {
			updateOtaa();
		}
	}
#endif

#if A_REFRESH==1
	if (strcmp(cmd, "REFR")==0) {									// Set refresh on=1 or off=0
		gwayConfig.refresh =(bool)atoi(arg);
		writeGwayCfg(CONFIGFILE);									// Save configuration to file
	}
#endif

}


// ----------------------------------------------------------------------------
// OPEN WEB PAGE
//	This is the init function for opening the webpage
//
// ----------------------------------------------------------------------------
static void openWebPage()
{
	++gwayConfig.views;									// increment number of views
#if A_REFRESH==1
	//server.client().stop();							// Experimental, stop webserver in case something is still running!
#endif
	String response="";	

	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");
	
	// init webserver, fill the webpage
	// NOTE: The page is renewed every _WWW_INTERVAL seconds, please adjust in ESP-sc-gway.h
	//
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send(200, "text/html", "");
#if A_REFRESH==1
	if (gwayConfig.refresh) {
		response += String() + "<!DOCTYPE HTML><HTML><HEAD><meta http-equiv='refresh' content='"+_WWW_INTERVAL+";http://";
		printIP((IPAddress)WiFi.localIP(),'.',response);
#ifdef ESP32BUILD
    response += "'><TITLE>ESP32 1ch Gateway</TITLE>";
#else    
		response += "'><TITLE>ESP8266 1ch Gateway</TITLE>";
#endif
	}
	else {
		response += String() + "<!DOCTYPE HTML><HTML><HEAD><TITLE>ESP8266 1ch Gateway</TITLE>";
	}
#else
	response += String() + "<!DOCTYPE HTML><HTML><HEAD><TITLE>ESP8266 1ch Gateway</TITLE>";
#endif
	response += "<META HTTP-EQUIV='CONTENT-TYPE' CONTENT='text/html; charset=UTF-8'>";
	response += "<META NAME='AUTHOR' CONTENT='M. Westenberg (mw1554@hotmail.com)'>";

	response += "<style>.thead {background-color:green; color:white;} ";
	response += ".cell {border: 1px solid black;}";
	response += ".config_table {max_width:100%; min-width:400px; width:95%; border:1px solid black; border-collapse:collapse;}";
	response += "</style></HEAD><BODY>";
	
	response +="<h1>ESP Gateway Config</h1>";

	response +="Version: "; response+=VERSION;
	response +="<br>ESP alive since "; 					// STARTED ON
	stringTime(1, response);

	response +=", Uptime: ";							// UPTIME
	uint32_t secs = millis()/1000;
	uint16_t days = secs / 86400;						// Determine number of days
	uint8_t _hour   = hour(secs);
	uint8_t _minute = minute(secs);
	uint8_t _second = second(secs);
	response += String() + days + "-";
	if (_hour < 10) response += "0";
	response += String() + _hour + ":";
	if (_minute < 10) response += "0";
	response += String() + _minute + ":";
	if (_second < 10) response += "0";
	response += String() + _second;
	
	response +="<br>Current time    "; 					// CURRENT TIME
	stringTime(millis(), response);
	response +="<br>";
	
	server.sendContent(response);
}


// ----------------------------------------------------------------------------
// CONFIG DATA
//
//
// ----------------------------------------------------------------------------
static void configData() 
{
	String response="";
	String bg="";
	
	response +="<h2>Gateway Settings</h2>";
	
	response +="<table class=\"config_table\">";
	response +="<tr>";
	response +="<th class=\"thead\">Setting</th>";
	response +="<th style=\"background-color: green; color: white; width:120px;\">Value</th>";
	response +="<th colspan=\"2\" style=\"background-color: green; color: white; width:100px;\">Set</th>";
	response +="</tr>";
	
	bg = " background-color: ";
	bg += ( _cad ? "LightGreen" : "orange" );
	response +="<tr><td class=\"cell\">CAD</td>";
	response +="<td style=\"border: 1px solid black;"; response += bg; response += "\">";
	response += ( _cad ? "ON" : "OFF" );
	response +="<td style=\"border: 1px solid black; width:40px;\"><a href=\"CAD=1\"><button>ON</button></a></td>";
	response +="<td style=\"border: 1px solid black; width:40px;\"><a href=\"CAD=0\"><button>OFF</button></a></td>";
	response +="</tr>";
	
	bg = " background-color: ";
	bg += ( _hop ? "LightGreen" : "orange" );
	response +="<tr><td class=\"cell\">HOP</td>";
	response +="<td style=\"border: 1px solid black;"; response += bg; response += "\">";
	response += ( _hop ? "ON" : "OFF" );
	response +="<td style=\"border: 1px solid black; width:40px;\"><a href=\"HOP=1\"><button>ON</button></a></td>";
	response +="<td style=\"border: 1px solid black; width:40px;\"><a href=\"HOP=0\"><button>OFF</button></a></td>";
	response +="</tr>";
	
	response +="<tr><td class=\"cell\">SF Setting</td><td class=\"cell\">";
	if (_cad) {
		response += "AUTO</td>";
	}
	else {
		response += sf;
		response +="<td class=\"cell\"><a href=\"SF=-1\"><button>-</button></a></td>";
		response +="<td class=\"cell\"><a href=\"SF=1\"><button>+</button></a></td>";
	}
	response +="</tr>";
	
	// Channel
	response +="<tr><td class=\"cell\">Channel</td>";
	response +="<td class=\"cell\">"; 
	if (_hop) {
		response += "AUTO</td>";
	}
	else {
		response += String() + ifreq; 
		response +="</td>";
		response +="<td class=\"cell\"><a href=\"FREQ=-1\"><button>-</button></a></td>";
		response +="<td class=\"cell\"><a href=\"FREQ=1\"><button>+</button></a></td>";
	}
	response +="</tr>";

	// Debugging options	
	response +="<tr><td class=\"cell\">";
	response +="Debug level</td><td class=\"cell\">"; 
	response +=debug; 
	response +="</td>";
	response +="<td class=\"cell\"><a href=\"DEBUG=-1\"><button>-</button></a></td>";
	response +="<td class=\"cell\"><a href=\"DEBUG=1\"><button>+</button></a></td>";
	response +="</tr>";

	// Serial Debugging
	response +="<tr><td class=\"cell\">";
	response +="Usb Debug</td><td class=\"cell\">"; 
	response +=DUSB; 
	response +="</td>";
	//response +="<td class=\"cell\"> </td>";
	//response +="<td class=\"cell\"> </td>";
	response +="</tr>";
	
#if GATEWAYNODE==1
	response +="<tr><td class=\"cell\">Framecounter Internal Sensor</td>";
	response +="<td class=\"cell\">";
	response +=frameCount;
	response +="</td><td colspan=\"2\" style=\"border: 1px solid black;\">";
	response +="<button><a href=\"/FCNT\">RESET</a></button></td>";
	response +="</tr>";
	
	bg = " background-color: ";
	bg += ( (gwayConfig.isNode == 1) ? "LightGreen" : "orange" );
	response +="<tr><td class=\"cell\">Gateway Node</td>";
	response +="<td class=\"cell\" style=\"border: 1px solid black;" + bg + "\">";
	response += ( (gwayConfig.isNode == true) ? "ON" : "OFF" );
	response +="<td style=\"border: 1px solid black; width:40px;\"><a href=\"NODE=1\"><button>ON</button></a></td>";
	response +="<td style=\"border: 1px solid black; width:40px;\"><a href=\"NODE=0\"><button>OFF</button></a></td>";
	response +="</tr>";
#endif

#if A_REFRESH==1
	bg = " background-color: ";
	bg += ( (gwayConfig.refresh == 1) ? "LightGreen" : "orange" );
	response +="<tr><td class=\"cell\">WWW Refresh</td>";
	response +="<td class=\"cell\" style=\"border: 1px solid black;" + bg + "\">";
	response += ( (gwayConfig.refresh == 1) ? "ON" : "OFF" );
	response +="<td style=\"border: 1px solid black; width:40px;\"><a href=\"REFR=1\"><button>ON</button></a></td>";
	response +="<td style=\"border: 1px solid black; width:40px;\"><a href=\"REFR=0\"><button>OFF</button></a></td>";
	response +="</tr>";
#endif

#if WIFIMANAGER==1
	response +="<tr><td>";
	response +="Click <a href=\"/NEWSSID\">here</a> to reset accesspoint<br>";
	response +="</td><td></td></tr>";
#endif

	// Reset all statistics
#if STATISTICS >= 1
	response +="<tr><td class=\"cell\">Statistics</td>";
	response +=String() + "<td class=\"cell\">"+statc.resets+"</td>";
	response +="<td colspan=\"2\" class=\"cell\"><a href=\"/RESET\"><button>RESET</button></a></td></tr>";
	
	response +="<tr><td class=\"cell\">Boots and Resets</td>";
	response +=String() + "<td class=\"cell\">"+gwayConfig.boots+"</td>";
	response +="<td colspan=\"2\" class=\"cell\"><a href=\"/BOOT\"><button>RESET</button></a></td></tr>";
#endif
	response +="</table>";
	
	// Update Firmware all statistics
	response +="<tr><td class=\"cell\">Update Firmware</td>";
	response +="<td class=\"cell\"></td><td colspan=\"2\" class=\"cell\"><a href=\"/UPDATE=1\"><button>RESET</button></a></td></tr>";
	response +="</table>";
	
	server.sendContent(response);
}


// ----------------------------------------------------------------------------
// INTERRUPT DATA
// Display interrupt data, but only for debug >= 2
//
// ----------------------------------------------------------------------------
static void interruptData()
{
	uint8_t flags = readRegister(REG_IRQ_FLAGS);
	uint8_t mask = readRegister(REG_IRQ_FLAGS_MASK);
	
	if (debug >= 2) {
		String response="";
		
		response +="<h2>System State and Interrupt</h2>";
		
		response +="<table class=\"config_table\">";
		response +="<tr>";
		response +="<th class=\"thead\">Parameter</th>";
		response +="<th class=\"thead\">Value</th>";
		response +="<th colspan=\"2\"  class=\"thead\">Set</th>";
		response +="</tr>";
		
		response +="<tr><td class=\"cell\">_state</td>";
		response +="<td class=\"cell\">";
		switch (_state) {							// See loraModem.h
			case S_INIT: response +="INIT"; break;
			case S_SCAN: response +="SCAN"; break;
			case S_CAD: response +="CAD"; break;
			case S_RX: response +="RX"; break;
			case S_TX: response +="TX"; break;
			default: response +="unknown"; break;
		}
		response +="</td></tr>";

		response +="<tr><td class=\"cell\">flags (8 bits)</td>";
		response +="<td class=\"cell\">0x";
		if (flags <16) response += "0";
		response +=String(flags,HEX); response+="</td></tr>";

		
		response +="<tr><td class=\"cell\">mask (8 bits)</td>";
		response +="<td class=\"cell\">0x"; 
		if (mask <16) response += "0";
		response +=String(mask,HEX); response+="</td></tr>";
		
		response +="<tr><td class=\"cell\">Re-entrant cntr</td>";
		response +="<td class=\"cell\">"; 
		response += String() + gwayConfig.reents;
		response +="</td></tr>";

		response +="<tr><td class=\"cell\">ntp call cntr</td>";
		response +="<td class=\"cell\">"; 
		response += String() + gwayConfig.ntps;
		response+="</td></tr>";
		
		response +="<tr><td class=\"cell\">ntpErr cntr</td>";
		response +="<td class=\"cell\">"; 
		response += String() + gwayConfig.ntpErr;
		response +="</td>";
		response +="<td colspan=\"2\" style=\"border: 1px solid black;\">";
		stringTime(gwayConfig.ntpErrTime, response);
		response +="</td>";
		response +="</tr>";
		
		response +="<tr><td class=\"cell\">Time Correction (uSec)</td><td class=\"cell\">"; 
		response += txDelay; 
		response +="</td>";
		response +="<td class=\"cell\"><a href=\"DELAY=-1\"><button>-</button></a></td>";
		response +="<td class=\"cell\"><a href=\"DELAY=1\"><button>+</button></a></td>";
		response +="</tr>";
		
		response +="</table>";
		
		server.sendContent(response);
	}// if debug>=2
}


// ----------------------------------------------------------------------------
// STATISTICS DATA
//
// ----------------------------------------------------------------------------
static void statisticsData()
{
	String response="";

	response +="<h2>Package Statistics</h2>";
	
	response +="<table class=\"config_table\">";
	response +="<tr>";
	response +="<th class=\"thead\">Counter</th>";
	response +="<th class=\"thead\">Pkgs</th>";
	response +="<th class=\"thead\">Pkgs/hr</th>";
	response +="</tr>";
	
	response +="<tr><td class=\"cell\">Packages Uplink Total</td>";
		response +="<td class=\"cell\">" + String(cp_nb_rx_rcv) + "</td>";
		response +="<td class=\"cell\">" + String((cp_nb_rx_rcv*3600)/(millis()/1000)) + "</td></tr>";
	response +="<tr><td class=\"cell\">Packages Uplink OK </td><td class=\"cell\">";
		response +=cp_nb_rx_ok; response+="</tr>";
	response +="<tr><td class=\"cell\">Packages Downlink</td><td class=\"cell\">"; 
		response +=cp_up_pkt_fwd; response+="</tr>";

	// Privide a tbale with all the SF data including percentage of messsages
#if STATISTICS >= 2
	response +="<tr><td class=\"cell\">SF7 rcvd</td>"; response +="<td class=\"cell\">"; response +=statc.sf7; 
		response +="<td class=\"cell\">"; response += String(cp_nb_rx_rcv>0 ? 100*statc.sf7/cp_nb_rx_rcv : 0)+" %"; response +="</td></tr>";
	response +="<tr><td class=\"cell\">SF8 rcvd</td>"; response +="<td class=\"cell\">"; response +=statc.sf8;
		response +="<td class=\"cell\">"; response += String(cp_nb_rx_rcv>0 ? 100*statc.sf8/cp_nb_rx_rcv : 0)+" %"; response +="</td></tr>";
	response +="<tr><td class=\"cell\">SF9 rcvd</td>"; response +="<td class=\"cell\">"; response +=statc.sf9;
		response +="<td class=\"cell\">"; response += String(cp_nb_rx_rcv>0 ? 100*statc.sf9/cp_nb_rx_rcv : 0)+" %"; response +="</td></tr>";
	response +="<tr><td class=\"cell\">SF10 rcvd</td>"; response +="<td class=\"cell\">"; response +=statc.sf10; 
		response +="<td class=\"cell\">"; response += String(cp_nb_rx_rcv>0 ? 100*statc.sf10/cp_nb_rx_rcv : 0)+" %"; response +="</td></tr>";
	response +="<tr><td class=\"cell\">SF11 rcvd</td>"; response +="<td class=\"cell\">"; response +=statc.sf11; 
		response +="<td class=\"cell\">"; response += String(cp_nb_rx_rcv>0 ? 100*statc.sf11/cp_nb_rx_rcv : 0)+" %"; response +="</td></tr>";
	response +="<tr><td class=\"cell\">SF12 rcvd</td>"; response +="<td class=\"cell\">"; response +=statc.sf12; 
		response +="<td class=\"cell\">"; response += String(cp_nb_rx_rcv>0 ? 100*statc.sf12/cp_nb_rx_rcv : 0)+" %"; response +="</td></tr>";
#endif

	response +="</table>";

	server.sendContent(response);
}




// ----------------------------------------------------------------------------
// SENSORDATA
// If enabled, display the sensorHistory on the current webserver Page.
// Parameters:
//	- <none>
// Returns:
//	- <none>
// ----------------------------------------------------------------------------
static void sensorData() 
{
#if STATISTICS >= 1
	String response="";
	
	response += "<h2>Message History</h2>";
	response += "<table class=\"config_table\">";
	response += "<tr>";
	response += "<th class=\"thead\">Time</th>";
	response += "<th class=\"thead\">Node</th>";
	response += "<th class=\"thead\" colspan=\"2\">Channel</th>";
	response += "<th class=\"thead\" style=\"width: 50px;\">SF</th>";
	response += "<th class=\"thead\" style=\"width: 50px;\">pRSSI</th>";
#if RSSI==1
	if (debug > 1) {
		response += "<th class=\"thead\" style=\"width: 50px;\">RSSI</th>";
	}
#endif
	response += "</tr>";
	server.sendContent(response);

	for (int i=0; i<MAX_STAT; i++) {
		if (statr[i].sf == 0) break;
		
		response = "";
		
		response += String() + "<tr><td class=\"cell\">";
		stringTime(statr[i].tmst, response);
		response += "</td>";
		response += String() + "<td class=\"cell\">";
		printHEX((char *)(& (statr[i].node)),' ',response);
		response += "</td>";
		response += String() + "<td class=\"cell\">" + statr[i].ch + "</td>";
		response += String() + "<td class=\"cell\">" + freqs[statr[i].ch] + "</td>";
		response += String() + "<td class=\"cell\">" + statr[i].sf + "</td>";

		response += String() + "<td class=\"cell\">" + statr[i].prssi + "</td>";
#if RSSI==1
		if (debug > 1) {
			response += String() + "<td class=\"cell\">" + statr[i].rssi + "</td>";
		}
#endif
		response += "</tr>";
		server.sendContent(response);
	}
	
	server.sendContent("</table>");
	
#endif
}

// ----------------------------------------------------------------------------
// SYSTEMDATA
//
// ----------------------------------------------------------------------------
static void systemData()
{
	String response="";
	response +="<h2>System Status</h2>";
	
	response +="<table class=\"config_table\">";
	response +="<tr>";
	response +="<th class=\"thead\">Parameter</th>";
	response +="<th class=\"thead\">Value</th>";
	response +="<th colspan=\"2\" class=\"thead\">Set</th>";
	response +="</tr>";
	
	response +="<tr><td style=\"border: 1px solid black; width:120px;\">Gateway ID</td>";
	response +="<td class=\"cell\">";	
	  if (MAC_array[0]< 0x10) response +='0'; response +=String(MAC_array[0],HEX);	// The MAC array is always returned in lowercase
	  if (MAC_array[1]< 0x10) response +='0'; response +=String(MAC_array[1],HEX);
	  if (MAC_array[2]< 0x10) response +='0'; response +=String(MAC_array[2],HEX);
	  response +="FFFF"; 
	  if (MAC_array[3]< 0x10) response +='0'; response +=String(MAC_array[3],HEX);
	  if (MAC_array[4]< 0x10) response +='0'; response +=String(MAC_array[4],HEX);
	  if (MAC_array[5]< 0x10) response +='0'; response +=String(MAC_array[5],HEX);
	response+="</tr>";
	

	response +="<tr><td class=\"cell\">Free heap</td><td class=\"cell\">"; response+=ESP.getFreeHeap(); response+="</tr>";
	response +="<tr><td class=\"cell\">ESP speed</td><td class=\"cell\">"; response+=ESP.getCpuFreqMHz(); 
		response +="<td style=\"border: 1px solid black; width:40px;\"><a href=\"SPEED=80\"><button>80</button></a></td>";
		response +="<td style=\"border: 1px solid black; width:40px;\"><a href=\"SPEED=160\"><button>160</button></a></td>";
		response+="</tr>";
#ifdef ESP32BUILD
  {
    char serial[13];
    uint64_t chipid = ESP.getEfuseMac();
    sprintf(serial,"%04X%08X", (uint16_t) (chipid>>32), (uint32_t) chipid);
    response +="<tr><td class=\"cell\">ESP Chip ID</td><td class=\"cell\">"; response+=serial; response+="</tr>";
  }
#else
  response +="<tr><td class=\"cell\">ESP Chip ID</td><td class=\"cell\">"; response+=ESP.getChipId(); response+="</tr>";
#endif  
	response +="<tr><td class=\"cell\">OLED</td><td class=\"cell\">"; response+=OLED; response+="</tr>";
		
#if STATISTICS>=1
	response +="<tr><td class=\"cell\">WiFi Setups</td><td class=\"cell\">"; response+=gwayConfig.wifis; response+="</tr>";
	response +="<tr><td class=\"cell\">WWW Views</td><td class=\"cell\">"; response+=gwayConfig.views; response+="</tr>";
#endif

	response +="</table>";
	server.sendContent(response);
}


// ----------------------------------------------------------------------------
// WIFIDATA
// Display the most important Wifi parameters gathered
//
// ----------------------------------------------------------------------------
static void wifiData()
{
	String response="";
	response +="<h2>WiFi Config</h2>";

	response +="<table class=\"config_table\">";

	response +="<tr><th class=\"thead\">Parameter</th><th class=\"thead\">Value</th></tr>";
	
	response +="<tr><td class=\"cell\">WiFi host</td><td class=\"cell\">"; 
#ifdef ESP32BUILD
  response +=WiFi.getHostname(); response+="</tr>";
#else
	response +=wifi_station_get_hostname(); response+="</tr>";
#endif

	response +="<tr><td class=\"cell\">WiFi SSID</td><td class=\"cell\">"; 
	response +=WiFi.SSID(); response+="</tr>";
	
	response +="<tr><td class=\"cell\">IP Address</td><td class=\"cell\">"; 
	printIP((IPAddress)WiFi.localIP(),'.',response); 
	response +="</tr>";
	response +="<tr><td class=\"cell\">IP Gateway</td><td class=\"cell\">"; 
	printIP((IPAddress)WiFi.gatewayIP(),'.',response); 
	response +="</tr>";
	response +="<tr><td class=\"cell\">NTP Server</td><td class=\"cell\">"; response+=NTP_TIMESERVER; response+="</tr>";
	response +="<tr><td class=\"cell\">LoRa Router</td><td class=\"cell\">"; response+=_TTNSERVER; response+="</tr>";
	response +="<tr><td class=\"cell\">LoRa Router IP</td><td class=\"cell\">"; 
	printIP((IPAddress)ttnServer,'.',response); 
	response +="</tr>";
#ifdef _THINGSERVER
	response +="<tr><td class=\"cell\">LoRa Router 2</td><td class=\"cell\">"; response+=_THINGSERVER; 
	response += String() + ":" + _THINGPORT + "</tr>";
	response +="<tr><td class=\"cell\">LoRa Router 2 IP</td><td class=\"cell\">"; 
	printIP((IPAddress)thingServer,'.',response);
	response +="</tr>";
#endif
	response +="</table>";

	server.sendContent(response);
}


// ----------------------------------------------------------------------------
// SEND WEB PAGE() 
// Call the webserver and send the standard content and the content that is 
// passed by the parameter.
//
// NOTE: This is the only place where yield() or delay() calls are used.
//
// ----------------------------------------------------------------------------
void sendWebPage(const char *cmd, const char *arg)
{
	openWebPage(); yield();
	
	setVariables(cmd,arg); yield();

	statisticsData(); yield();		 			// Node statistics
	sensorData(); yield();						// Display the sensor history, message statistics
	systemData(); yield();						// System statistics such as heap etc.
	wifiData(); yield();						// WiFI specific parameters
	
	configData(); yield();						// Display web configuration
	
	interruptData(); yield();					// Display interrupts only when debug >= 2
		
	// Close the client connection to server
	server.sendContent(String() + "<br><br />Click <a href=\"/HELP\">here</a> to explain Help and REST options<br>");
	server.sendContent(String() + "</BODY></HTML>");
	server.sendContent(""); yield();
	
	server.client().stop();
}

// ----------------------------------------------------------------------------
// Used by timeout functions
// This function only displays the standard homepage
//	Note: This function is not actively used, as the page is renewed 
//	by using a HTML meta setting of 60 seconds
// ----------------------------------------------------------------------------
static void renewWebPage()
{
	//Serial.println(F("Renew Web Page"));
	//sendWebPage("","");
	//return;
}

// ----------------------------------------------------------------------------
// SetupWWW function called by main setup() program to setup webserver
// It does actually not much more than installing the callback handlers
// for messages sent to the webserver
//
// Implemented is an interface like:
// http://<server>/<Variable>=<value>
//
// ----------------------------------------------------------------------------
extern "C" void setup_WWW(void) 
{
	server.begin();									// Start the webserver
	
	// -----------------
	// BUTTONS, define what should happen with the buttons we press on the homepage
	
	server.on("/", []() {
		sendWebPage("","");							// Send the webPage string
		server.sendHeader("Location", String("/"), true);
		server.send ( 302, "text/plain", "");
	});

	
	server.on("/HELP", []() {
		sendWebPage("HELP","");					// Send the webPage string
		server.sendHeader("Location", String("/"), true);
		server.send ( 302, "text/plain", "");
	});

	
	// Reset the statistics
	server.on("/RESET", []() {
		Serial.println(F("RESET"));
		cp_nb_rx_rcv = 0;
		cp_nb_rx_ok = 0;
		cp_up_pkt_fwd = 0;
#if STATISTICS >= 1
		for (int i=0; i<MAX_STAT; i++) { statr[i].sf = 0; }
#if STATISTICS >= 2
		statc.sf7 = 0;
		statc.sf8 = 0;
		statc.sf9 = 0;
		statc.sf10= 0;
		statc.sf11= 0;
		statc.sf12= 0;
		
		statc.resets= 0;
		writeGwayCfg(CONFIGFILE);
#endif
#endif
		server.sendHeader("Location", String("/"), true);
		server.send ( 302, "text/plain", "");
	});

	// Reset the boot counter
	server.on("/BOOT", []() {
		Serial.println(F("BOOT"));
#if STATISTICS >= 2
		gwayConfig.boots = 0;
		gwayConfig.wifis = 0;
		gwayConfig.views = 0;
		gwayConfig.ntpErr = 0;					// NTP errors
		gwayConfig.ntpErrTime = 0;				// NTP last error time
		gwayConfig.ntps = 0;					// Number of NTP calls
#endif
		gwayConfig.reents = 0;					// Re-entrance

		writeGwayCfg(CONFIGFILE);
		server.sendHeader("Location", String("/"), true);
		server.send ( 302, "text/plain", "");
	});

	server.on("/NEWSSID", []() {
		sendWebPage("NEWSSID","");				// Send the webPage string
		server.sendHeader("Location", String("/"), true);
		server.send ( 302, "text/plain", "");
	});

	// Set debug parameter
	server.on("/DEBUG=-1", []() {				// Set debug level 0-2						
		debug = (debug+3)%4;
		writeGwayCfg(CONFIGFILE);				// Save configuration to file
		server.sendHeader("Location", String("/"), true);
		server.send ( 302, "text/plain", "");
	});
	server.on("/DEBUG=1", []() {
		debug = (debug+1)%4;
		writeGwayCfg(CONFIGFILE);				// Save configuration to file
		server.sendHeader("Location", String("/"), true);
		server.send ( 302, "text/plain", "");
	});

	
	// Set delay in microseconds
	server.on("/DELAY=1", []() {
		txDelay+=1000;
		server.sendHeader("Location", String("/"), true);
		server.send ( 302, "text/plain", "");
	});
	server.on("/DELAY=-1", []() {
		txDelay-=1000;
		server.sendHeader("Location", String("/"), true);
		server.send ( 302, "text/plain", "");
	});


	// Spreading Factor setting
	server.on("/SF=1", []() {
		if (sf>=SF12) sf=SF7; else sf= (sf_t)((int)sf+1);
		server.sendHeader("Location", String("/"), true);
		server.send ( 302, "text/plain", "");
	});
	server.on("/SF=-1", []() {
		if (sf<=SF7) sf=SF12; else sf= (sf_t)((int)sf-1);
		server.sendHeader("Location", String("/"), true);
		server.send ( 302, "text/plain", "");
	});

	// Frequency of the GateWay node
	server.on("/FREQ=1", []() {
		uint8_t nf = sizeof(freqs)/sizeof(int);	// Number of elements in array
		if (ifreq==(nf-1)) ifreq=0; else ifreq++;
		server.sendHeader("Location", String("/"), true);
		server.send ( 302, "text/plain", "");
	});
	server.on("/FREQ=-1", []() {
		uint8_t nf = sizeof(freqs)/sizeof(int);	// Number of elements in array
		if (ifreq==0) ifreq=(nf-1); else ifreq--;
		server.sendHeader("Location", String("/"), true);
		server.send ( 302, "text/plain", "");
	});

	// Set CAD function off/on
	server.on("/CAD=1", []() {
		_cad=(bool)1;
		writeGwayCfg(CONFIGFILE);				// Save configuration to file
		server.sendHeader("Location", String("/"), true);
		server.send ( 302, "text/plain", "");
	});
	server.on("/CAD=0", []() {
		_cad=(bool)0;
		writeGwayCfg(CONFIGFILE);				// Save configuration to file
		server.sendHeader("Location", String("/"), true);
		server.send ( 302, "text/plain", "");
	});

	// GatewayNode
	server.on("/NODE=1", []() {
#if GATEWAYNODE==1
		gwayConfig.isNode =(bool)1;
		writeGwayCfg(CONFIGFILE);				// Save configuration to file
#endif
		server.sendHeader("Location", String("/"), true);
		server.send ( 302, "text/plain", "");
	});
	server.on("/NODE=0", []() {
#if GATEWAYNODE==1
		gwayConfig.isNode =(bool)0;
		writeGwayCfg(CONFIGFILE);				// Save configuration to file
#endif
		server.sendHeader("Location", String("/"), true);
		server.send ( 302, "text/plain", "");
	});

#if GATEWAYNODE==1	
	// Framecounter of the Gateway node
	server.on("/FCNT", []() {

		frameCount=0; 
		rxLoraModem();							// Reset the radion with the new frequency
		writeGwayCfg(CONFIGFILE);

		//sendWebPage("","");						// Send the webPage string
		server.sendHeader("Location", String("/"), true);
		server.send ( 302, "text/plain", "");
	});
#endif
	// WWW Page refresh function
	server.on("/REFR=1", []() {					// WWW page auto refresh ON
#if A_REFRESH==1
		gwayConfig.refresh =1;
		writeGwayCfg(CONFIGFILE);				// Save configuration to file
#endif		
		server.sendHeader("Location", String("/"), true);
		server.send ( 302, "text/plain", "");
	});
	server.on("/REFR=0", []() {					// WWW page auto refresh OFF
#if A_REFRESH==1
		gwayConfig.refresh =0;
		writeGwayCfg(CONFIGFILE);				// Save configuration to file
#endif
		server.sendHeader("Location", String("/"), true);
		server.send ( 302, "text/plain", "");
	});

	
	// Switch off/on the HOP functions
	server.on("/HOP=1", []() {
		_hop=true;
		server.sendHeader("Location", String("/"), true);
		server.send ( 302, "text/plain", "");
	});
	server.on("/HOP=0", []() {
		_hop=false;
		ifreq=0; 
		freq=freqs[0]; 
		rxLoraModem();
		server.sendHeader("Location", String("/"), true);
		server.send ( 302, "text/plain", "");
	});

#ifndef ESP32BUILD
	server.on("/SPEED=80", []() {
		system_update_cpu_freq(80);
		server.sendHeader("Location", String("/"), true);
		server.send ( 302, "text/plain", "");
	});
	server.on("/SPEED=160", []() {
		system_update_cpu_freq(160);
		server.sendHeader("Location", String("/"), true);
		server.send ( 302, "text/plain", "");
	});
#endif

	// Update the sketch. Not yet implemented
	server.on("/UPDATE=1", []() {
#if A_OTA==1
		updateOtaa();
#endif
		server.sendHeader("Location", String("/"), true);
		server.send ( 302, "text/plain", "");
	});

	// -----------
	// This section from version 4.0.7 defines what PART of the
	// webpage is shown based on the buttons pressed by the user
	// Maybe not all information should be put on the screen since it
	// may take too much time to serve all information before a next
	// package interrupt arrives at the gateway
	
	Serial.print(F("WWW Server started on port "));
	Serial.println(A_SERVERPORT);
	return;
}

#endif

