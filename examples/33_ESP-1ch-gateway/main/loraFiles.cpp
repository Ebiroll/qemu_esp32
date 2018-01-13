// 1-channel LoRa Gateway for ESP8266
// Copyright (c) 2016, 2017 Maarten Westenberg version for ESP8266
// Version 5.0.1
// Date: 2017-11-15
//
// 	based on work done by Thomas Telkamp for Raspberry PI 1ch gateway
//	and many others.
//
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the MIT License
// which accompanies this distribution, and is available at
// https://opensource.org/licenses/mit-license.php
//
// Author: Maarten Westenberg (mw12554@hotmail.com)
//
// This file contains the LoRa filesystem specific code

#include <Arduino.h>
#include <SPIFFS.h>
#include "loraFiles.h"
#include "ESP-sc-gway.h"
#include <WiFi.h>

espGwayConfig gwayConfig;


int writeConfig(const char *fn, struct espGwayConfig *c);

// ============================================================================
// LORA SPIFFS FILESYSTEM FUNCTIONS
//
// The LoRa supporting functions are in the section below

// ----------------------------------------------------------------------------
// Directory listing. s is a string containing HTML/text code so far.
// The resulting directory listing is appended to s and returned.
// ----------------------------------------------------------------------------
//String espDir(String s) {
//
//	return s;
//}

// ----------------------------------------------------------------------------
// Read the gateway configuration file
// ----------------------------------------------------------------------------
int readConfig(const char *fn, struct espGwayConfig *c) {

	Serial.println(F("readConfig:: Starting"));

	if (!SPIFFS.exists(fn)) {
		Serial.print(F("ERROR:: readConfig, file does not exist "));
		Serial.println(fn);
		return(-1);
	}
	File f = SPIFFS.open(fn, "r");
	if (!f) {
		Serial.println(F("ERROR:: SPIFFS open failed"));
		return(-1);
	}

	while (f.available()) {
		
		String id =f.readStringUntil('=');
		String val=f.readStringUntil('\n');
		
		if (id == "SSID") {									// WiFi SSID
			Serial.print(F("SSID=")); Serial.println(val);
			(*c).ssid = val;								// val contains ssid, we do NO check
		}
		//if (id == "PASS") { 								// WiFi Password
		//	Serial.print(F("PASS=")); Serial.println(val); 
		//	(*c).pass = val;
		//}
		if (id == "CH") { 									// Frequency Channel
			Serial.print(F("CH=")); Serial.println(val); 
			(*c).ch = (uint32_t) val.toInt();
		}
		if (id == "SF") { 									// Spreading Factor
			Serial.print(F("SF  =")); Serial.println(val);
			(*c).sf = (uint32_t) val.toInt();
		}
		if (id == "FCNT") {									// Frame Counter
			Serial.print(F("FCNT=")); Serial.println(val);
			(*c).fcnt = (uint32_t) val.toInt();
		}
		if (id == "DEBUG") {								// Frame Counter
			Serial.print(F("DEBUG=")); Serial.println(val);
			(*c).debug = (uint8_t) val.toInt();
		}
		if (id == "CAD") {									// CAD setting
			Serial.print(F("CAD=")); Serial.println(val);
			(*c).cad = (uint8_t) val.toInt();
		}
		if (id == "HOP") {									// HOP setting
			Serial.print(F("HOP=")); Serial.println(val);
			(*c).hop = (uint8_t) val.toInt();
		}
		if (id == "BOOTS") {								// BOOTS setting
			Serial.print(F("BOOTS=")); Serial.println(val);
			(*c).boots = (uint8_t) val.toInt();
		}
		if (id == "RESETS") {								// RESET setting
			Serial.print(F("RESETS=")); Serial.println(val);
			(*c).resets = (uint8_t) val.toInt();
		}
		if (id == "WIFIS") {								// WIFIS setting
			Serial.print(F("WIFIS=")); Serial.println(val);
			(*c).wifis = (uint8_t) val.toInt();
		}
		if (id == "VIEWS") {								// VIEWS setting
			Serial.print(F("VIEWS=")); Serial.println(val);
			(*c).views = (uint8_t) val.toInt();
		}
		if (id == "NODE") {									// NODE setting
			Serial.print(F("NODE=")); Serial.println(val);
			(*c).isNode = (uint8_t) val.toInt();
		}
		if (id == "REFR") {									// REFR setting
			Serial.print(F("REFR=")); Serial.println(val);
			(*c).refresh = (uint8_t) val.toInt();
		}
		if (id == "REENTS") {								// REENTS setting
			Serial.print(F("REENTS=")); Serial.println(val);
			(*c).reents = (uint8_t) val.toInt();
		}
		if (id == "NTPERR") {								// NTPERR setting
			Serial.print(F("NTPERR=")); Serial.println(val);
			(*c).ntpErr = (uint8_t) val.toInt();
		}
		if (id == "NTPETIM") {								// NTPERR setting
			Serial.print(F("NTPETIM=")); Serial.println(val);
			(*c).ntpErrTime = (uint32_t) val.toInt();
		}
		if (id == "NTPS") {									// NTPS setting
			Serial.print(F("NTPS=")); Serial.println(val);
			(*c).ntps = (uint8_t) val.toInt();
		}
	}
	f.close();
	return(1);
}

// ----------------------------------------------------------------------------
// Write the current gateway configuration to SPIFFS. First copy all the
// separate data items to the gwayConfig structure
//
// ----------------------------------------------------------------------------
int writeGwayCfg(const char *fn) {

	gwayConfig.sf = (uint8_t) sf;
	gwayConfig.ssid = WiFi.SSID();
	//gwayConfig.pass = WiFi.PASS();					// XXX We should find a way to store the password too
	gwayConfig.ch = ifreq;
	gwayConfig.debug = debug;
	gwayConfig.cad = _cad;
	gwayConfig.hop = _hop;
#if GATEWAYNODE==1
	gwayConfig.fcnt = frameCount;
#endif

	return(writeConfig(fn, &gwayConfig));
}

// ----------------------------------------------------------------------------
// Write the configuration ad found in the espGwayConfig structure
// to SPIFFS
// ----------------------------------------------------------------------------
int writeConfig(const char *fn, struct espGwayConfig *c) {

	if (!SPIFFS.exists(fn)) {
		Serial.print("WARNING:: writeConfig, file not exists, formatting ");
		SPIFFS.format();
		// XXX make all initial declarations here if config vars need to have a value
		Serial.println(fn);
	}
	File f = SPIFFS.open(fn, "w");
	if (!f) {
		Serial.print("ERROR:: writeConfig, open file=");
		Serial.print(fn);
		Serial.println();
		return(-1);
	}

	f.print("SSID"); f.print('='); f.print((*c).ssid); f.print('\n'); 
	f.print("PASS"); f.print('='); f.print((*c).pass); f.print('\n');
	f.print("CH"); f.print('='); f.print((*c).ch); f.print('\n');
	f.print("SF");   f.print('='); f.print((*c).sf);   f.print('\n');
	f.print("FCNT"); f.print('='); f.print((*c).fcnt); f.print('\n');
	f.print("DEBUG"); f.print('='); f.print((*c).debug); f.print('\n');
	f.print("CAD");  f.print('='); f.print((*c).cad); f.print('\n');
	f.print("HOP");  f.print('='); f.print((*c).hop); f.print('\n');
	f.print("NODE");  f.print('='); f.print((*c).isNode); f.print('\n');
	f.print("BOOTS");  f.print('='); f.print((*c).boots); f.print('\n');
	f.print("RESETS");  f.print('='); f.print((*c).resets); f.print('\n');
	f.print("WIFIS");  f.print('='); f.print((*c).wifis); f.print('\n');
	f.print("VIEWS");  f.print('='); f.print((*c).views); f.print('\n');
	f.print("REFR");  f.print('='); f.print((*c).refresh); f.print('\n');
	f.print("REENTS");  f.print('='); f.print((*c).reents); f.print('\n');
	f.print("NTPETIM");  f.print('='); f.print((*c).ntpErrTime); f.print('\n');
	f.print("NTPERR");  f.print('='); f.print((*c).ntpErr); f.print('\n');
	f.print("NTPS");  f.print('='); f.print((*c).ntps); f.print('\n');
	
	f.close();
	return(1);
}
