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
// This file contains the LoRa modem specific code enabling to receive
// and transmit packages/messages.
// ========================================================================================

#include <Arduino.h>
//#include<ArduinoJson/StaticJsonBuffer.hpp>
#include<ArduinoJson.h>
#include <gBase64.h>
#include "ESP-sc-gway.h"
#include "loraModem.h"
#include <Time.h>

#include "SSD1306.h"
extern SSD1306  display;

extern uint8_t payLoad[128];
extern IPAddress ttnServer;
void ftoa(float f, char *val, int p);
int sendUdp(IPAddress server, int port, uint8_t *msg, int length);




// ----------------------------------------------------------------------------
// DOWN DOWN DOWN DOWN DOWN DOWN DOWN DOWN DOWN DOWN DOWN DOWN DOWN DOWN DOWN 
// Send DOWN a LoRa packet over the air to the node. This function does all the 
// decoding of the server message and prepares a Payload buffer.
// The payload is actually transmitted by the sendPkt() function.
// This function is used for regular downstream messages and for JOIN_ACCEPT
// messages.
// NOTE: This is not an interrupt function, but is started by loop().
// ----------------------------------------------------------------------------
int sendPacket(uint8_t *buf, uint8_t length) 
{
	// Received package with Meta Data:
	// codr	: "4/5"
	// data	: "Kuc5CSwJ7/a5JgPHrP29X9K6kf/Vs5kU6g=="	// for example
	// freq	: 868.1 									// 868100000
	// ipol	: true/false
	// modu : "LORA"
	// powe	: 14										// Set by default
	// rfch : 0											// Set by default
	// size	: 21
	// tmst : 1800642 									// for example
	// datr	: "SF7BW125"
	
	// 12-byte header;
	//		HDR (1 byte)
	//		
	//
	// Data Reply for JOIN_ACCEPT as sent by server:
	//		AppNonce (3 byte)
	//		NetID (3 byte)
	//		DevAddr (4 byte) [ 31..25]:NwkID , [24..0]:NwkAddr
 	//		DLSettings (1 byte)
	//		RxDelay (1 byte)
	//		CFList (fill to 16 bytes)
			
	int i=0;
	StaticJsonBuffer<312> jsonBuffer;
	char * bufPtr = (char *) (buf);
	buf[length] = 0;
	
#if DUSB>=1
	if (debug>=2) {
		Serial.println((char *)buf);
		Serial.print(F("<"));
		Serial.flush();
	}
#endif
	// Use JSON to decode the string after the first 4 bytes.
	// The data for the node is in the "data" field. This function destroys original buffer
	JsonObject& root = jsonBuffer.parseObject(bufPtr);
		
	if (!root.success()) {
#if DUSB>=1
		Serial.print (F("sendPacket:: ERROR Json Decode"));
		if (debug>=2) {
			Serial.print(':');
			Serial.println(bufPtr);
		}
		Serial.flush();
#endif
		return(-1);
	}
	delay(1);
	// Meta Data sent by server (example)
	// {"txpk":{"codr":"4/5","data":"YCkEAgIABQABGmIwYX/kSn4Y","freq":868.1,"ipol":true,"modu":"LORA","powe":14,"rfch":0,"size":18,"tmst":1890991792,"datr":"SF7BW125"}}

	// Used in the protocol:
	const char * data	= root["txpk"]["data"];
	uint8_t psize		= root["txpk"]["size"];
	bool ipol			= root["txpk"]["ipol"];
	uint8_t powe		= root["txpk"]["powe"];
	uint32_t tmst		= (uint32_t) root["txpk"]["tmst"].as<unsigned long>();

	// Not used in the protocol:
	const char * datr	= root["txpk"]["datr"];			// eg "SF7BW125"
	const float ff		= root["txpk"]["freq"];			// eg 869.525
	const char * modu	= root["txpk"]["modu"];			// =="LORA"
	const char * codr	= root["txpk"]["codr"];
	//if (root["txpk"].containsKey("imme") ) {
	//	const bool imme = root["txpk"]["imme"];			// Immediate Transmit (tmst don't care)
	//}

	if (data != NULL) {
#if DUSB>=1
		if (debug>=2) { 
			Serial.print(F("data: ")); 
			Serial.println((char *) data);
			if (debug>=2) Serial.flush();
		}
#endif
	}
	else {
#if DUSB>=1
		Serial.println(F("sendPacket:: ERROR: data is NULL"));
		if (debug>=2) Serial.flush();
#endif
		return(-1);
	}
	
	uint8_t iiq = (ipol? 0x40: 0x27);					// if ipol==true 0x40 else 0x27
	uint8_t crc = 0x00;									// switch CRC off for TX
	uint8_t payLength = base64_dec_len((char *) data, strlen(data));
	
	// Fill payload with decoded message
	base64_decode((char *) payLoad, (char *) data, strlen(data));

	// Compute wait time in microseconds
	uint32_t w = (uint32_t) (tmst - micros());
	
#if _STRICT_1CH == 1
	// Use RX1 timeslot as this is our frequency.
	// Do not use RX2 or JOIN2 as they contain other frequencies
	if ((w>1000000) && (w<3000000)) { tmst-=1000000; }
	else if ((w>6000000) && (w<7000000)) { tmst-=1000000; }
	
	const uint8_t sfTx = sfi;							// Take care, TX sf not to be mixed with SCAN
	const uint32_t fff = freq;
#else
	const uint8_t sfTx = atoi(datr+2);					// Convert "SF9BW125" to 9
	// convert double frequency (MHz) into uint32_t frequency in Hz.
	const uint32_t fff = (uint32_t) ((uint32_t)((ff+0.000035)*1000)) * 1000;
#endif

	// All data is in Payload and parameters and need to be transmitted.
	// The function is called in user-space
	_state = S_TX;										// _state set to transmit
	
	LoraDown.payLoad = payLoad;
	LoraDown.payLength = payLength;
	LoraDown.tmst = tmst;
	LoraDown.sfTx = sfTx;
	LoraDown.powe = powe;
	LoraDown.fff = fff;
	LoraDown.crc = crc;
	LoraDown.iiq = iiq;

	Serial.println(F("sendPacket:: LoraDown filled"));

#if DUSB>=1
	if (debug>=2) {
		Serial.print(F("Request:: "));
		Serial.print(F(" tmst=")); Serial.print(tmst); Serial.print(F(" wait=")); Serial.println(w);
		
		Serial.print(F(" strict=")); Serial.print(_STRICT_1CH);
		Serial.print(F(" datr=")); Serial.println(datr);
		Serial.print(F(" freq=")); Serial.print(freq); Serial.print(F(" ->")); Serial.println(fff);
		Serial.print(F(" sf  =")); Serial.print(sf); Serial.print(F(" ->")); Serial.print(sfTx);
		
		Serial.print(F(" modu=")); Serial.print(modu);
		Serial.print(F(" powe=")); Serial.print(powe);
		Serial.print(F(" codr=")); Serial.println(codr);

		Serial.print(F(" ipol=")); Serial.println(ipol);
		Serial.println();								// empty line between messages
	}
#endif

	if (payLength != psize) {
#if DUSB>=1
		Serial.print(F("sendPacket:: WARNING payLength: "));
		Serial.print(payLength);
		Serial.print(F(", psize="));
		Serial.println(psize);
		if (debug>=2) Serial.flush();
#endif
	}
#if DUSB>=1
	else if (debug >= 2) {
		for (i=0; i<payLength; i++) {
			Serial.print(payLoad[i],HEX); 
			Serial.print(':'); 
		}
		Serial.println();
		if (debug>=2) Serial.flush();
	}
#endif
	cp_up_pkt_fwd++;

#if DUSB>=1
	Serial.println(F("sendPacket:: fini OK"));
#endif
	return 1;
}//sendPacket




// ----------------------------------------------------------------------------
// UP UP UP UP UP UP UP UP UP UP UP UP UP UP UP UP UP UP UP UP UP UP UP UP UP UP
// Based on the information read from the LoRa transceiver (or fake message)
// build a gateway message to send upstream.
//
// parameters:
// tmst: Timestamp to include in the upstream message
// buff_up: The buffer that is generated for upstream
// message: The payload message to include in the the buff_up
// messageLength: The number of bytes received by the LoRa transceiver
// internal: Boolean value to indicate whether the local sensor is processed
//
// ----------------------------------------------------------------------------
int buildPacket(uint32_t tmst, uint8_t *buff_up, struct LoraUp_t LoraUp, bool internal) 
{
	long SNR;
    int rssicorr;
	int prssi;											// packet rssi
	
	char cfreq[12] = {0};								// Character array to hold freq in MHz
	lastTmst = tmst;									// Following/according to spec
	int buff_index=0;
	char b64[256];
	
	uint8_t *message = LoraUp.payLoad;
	char messageLength = LoraUp.payLength;
		
#if _CHECK_MIC==1
	unsigned char NwkSKey[16] = _NWKSKEY;
	checkMic(message, messageLength, NwkSKey);
#endif

	// Read SNR and RSSI from the register. Note: Not for internal sensors!
	// For internal sensor we fake these values as we cannot read a register
	if (internal) {
		SNR = 12;
		prssi = 50;
		rssicorr = 157;
	}
	else {
		SNR = LoraUp.snr;
		prssi = LoraUp.prssi;								// read register 0x1A, packet rssi
		rssicorr = LoraUp.rssicorr;
	}

#if STATISTICS >= 1
	// Receive statistics
	for (int m=( MAX_STAT -1); m>0; m--) statr[m]=statr[m-1];
	statr[0].tmst = millis();
	statr[0].ch= ifreq;
	statr[0].prssi = prssi - rssicorr;
#if RSSI==1
	statr[0].rssi = _rssi - rssicorr;
#endif
	statr[0].sf = LoraUp.sf;
	statr[0].node = ( message[1]<<24 | message[2]<<16 | message[3]<<8 | message[4] );

#if STATISTICS >= 2
	switch (statr[0].sf) {
		case SF7: statc.sf7++; break;
		case SF8: statc.sf8++; break;
		case SF9: statc.sf9++; break;
		case SF10: statc.sf10++; break;
		case SF11: statc.sf11++; break;
		case SF12: statc.sf12++; break;
	}
#endif	
#endif

#if DUSB>=1	
	if (debug>=1) {
		Serial.print(F("pRSSI: "));
		Serial.print(prssi-rssicorr);
		Serial.print(F(" RSSI: "));
		Serial.print(_rssi - rssicorr);
		Serial.print(F(" SNR: "));
		Serial.print(SNR);
		Serial.print(F(" Length: "));
		Serial.print((int)messageLength);
		Serial.print(F(" -> "));
		int i;
		for (i=0; i< messageLength; i++) {
					Serial.print(message[i],HEX);
					Serial.print(' ');
		}
		Serial.println();
		yield();
	}
#endif

// Show received message status on OLED display
#if OLED==1
      display.clear();
      display.setFont(ArialMT_Plain_16);
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      char timBuff[20];
      sprintf(timBuff, "%02i:%02i:%02i", hour(), minute(), second());
      display.drawString(0, 0, "Time: " );
      display.drawString(40, 0, timBuff);
      display.drawString(0, 16, "RSSI: " );
      display.drawString(40, 16, String(prssi-rssicorr));
      display.drawString(70, 16, ",SNR: " );
      display.drawString(110, 16, String(SNR) );
	  
	  display.drawString(0, 32, "Addr: " );
	  
      if (message[4] < 0x10) display.drawString( 40, 32, "0"+String(message[4], HEX)); else display.drawString( 40, 32, String(message[4], HEX));
	  if (message[3] < 0x10) display.drawString( 61, 32, "0"+String(message[3], HEX)); else display.drawString( 61, 32, String(message[3], HEX));
	  if (message[2] < 0x10) display.drawString( 82, 32, "0"+String(message[2], HEX)); else display.drawString( 82, 32, String(message[2], HEX));
	  if (message[1] < 0x10) display.drawString(103, 32, "0"+String(message[1], HEX)); else display.drawString(103, 32, String(message[1], HEX));
	  
      display.drawString(0, 48, "LEN: " );
      display.drawString(40, 48, String((int)messageLength) );
      display.display();
	  yield();
#endif
			
	int j;
	
	// XXX Base64 library is nopad. So we may have to add padding characters until
	// 	message Length is multiple of 4!
	// Encode message with messageLength into b64
	int encodedLen = base64_enc_len(messageLength);		// max 341
#if DUSB>=1
	if ((debug>=1) && (encodedLen>255)) {
		Serial.println(F("buildPacket:: b64 error"));
		if (debug>=2) Serial.flush();
	}
#endif
	base64_encode(b64, (char *) message, messageLength);// max 341

	// pre-fill the data buffer with fixed fields
	buff_up[0] = PROTOCOL_VERSION;						// 0x01 still
	buff_up[3] = PKT_PUSH_DATA;							// 0x00

	// READ MAC ADDRESS OF ESP8266, and insert 0xFF 0xFF in the middle
	buff_up[4]  = MAC_array[0];
	buff_up[5]  = MAC_array[1];
	buff_up[6]  = MAC_array[2];
	buff_up[7]  = 0xFF;
	buff_up[8]  = 0xFF;
	buff_up[9]  = MAC_array[3];
	buff_up[10] = MAC_array[4];
	buff_up[11] = MAC_array[5];

	// start composing datagram with the header 
	uint8_t token_h = (uint8_t)rand(); 					// random token
	uint8_t token_l = (uint8_t)rand(); 					// random token
	buff_up[1] = token_h;
	buff_up[2] = token_l;
	buff_index = 12; 									// 12-byte binary (!) header

	// start of JSON structure that will make payload
	memcpy((void *)(buff_up + buff_index), (void *)"{\"rxpk\":[", 9);
	buff_index += 9;
	buff_up[buff_index] = '{';
	++buff_index;
	j = snprintf((char *)(buff_up + buff_index), TX_BUFF_SIZE-buff_index, "\"tmst\":%u", tmst);
#if DUSB>=1
		if ((j<0) && (debug>=1)) {
			Serial.println(F("buildPacket:: Error "));
		}
#endif
	buff_index += j;
	ftoa((double)freq/1000000,cfreq,6);					// XXX This can be done better
	j = snprintf((char *)(buff_up + buff_index), TX_BUFF_SIZE-buff_index, ",\"chan\":%1u,\"rfch\":%1u,\"freq\":%s", 0, 0, cfreq);
	buff_index += j;
	memcpy((void *)(buff_up + buff_index), (void *)",\"stat\":1", 9);
	buff_index += 9;
	memcpy((void *)(buff_up + buff_index), (void *)",\"modu\":\"LORA\"", 14);
	buff_index += 14;
	
	/* Lora datarate & bandwidth, 16-19 useful chars */
	switch (LoraUp.sf) {
		case SF6:
			memcpy((void *)(buff_up + buff_index), (void *)",\"datr\":\"SF6", 12);
			buff_index += 12;
			break;
		case SF7:
			memcpy((void *)(buff_up + buff_index), (void *)",\"datr\":\"SF7", 12);
			buff_index += 12;
			break;
		case SF8:
            memcpy((void *)(buff_up + buff_index), (void *)",\"datr\":\"SF8", 12);
            buff_index += 12;
            break;
		case SF9:
            memcpy((void *)(buff_up + buff_index), (void *)",\"datr\":\"SF9", 12);
            buff_index += 12;
            break;
		case SF10:
            memcpy((void *)(buff_up + buff_index), (void *)",\"datr\":\"SF10", 13);
            buff_index += 13;
            break;
		case SF11:
            memcpy((void *)(buff_up + buff_index), (void *)",\"datr\":\"SF11", 13);
            buff_index += 13;
            break;
		case SF12:
            memcpy((void *)(buff_up + buff_index), (void *)",\"datr\":\"SF12", 13);
            buff_index += 13;
            break;
		default:
            memcpy((void *)(buff_up + buff_index), (void *)",\"datr\":\"SF?", 12);
            buff_index += 12;
	}
	memcpy((void *)(buff_up + buff_index), (void *)"BW125\"", 6);
	buff_index += 6;
	memcpy((void *)(buff_up + buff_index), (void *)",\"codr\":\"4/5\"", 13);
	buff_index += 13;
	j = snprintf((char *)(buff_up + buff_index), TX_BUFF_SIZE-buff_index, ",\"lsnr\":%li", SNR);
	buff_index += j;
	j = snprintf((char *)(buff_up + buff_index), TX_BUFF_SIZE-buff_index, ",\"rssi\":%d,\"size\":%u", prssi-rssicorr, messageLength);
	buff_index += j;
	memcpy((void *)(buff_up + buff_index), (void *)",\"data\":\"", 9);
	buff_index += 9;

	// Use gBase64 library to fill in the data string
	encodedLen = base64_enc_len(messageLength);			// max 341
	j = base64_encode((char *)(buff_up + buff_index), (char *) message, messageLength);

	buff_index += j;
	buff_up[buff_index] = '"';
	++buff_index;

	// End of packet serialization
	buff_up[buff_index] = '}';
	++buff_index;
	buff_up[buff_index] = ']';
	++buff_index;
	
	// end of JSON datagram payload */
	buff_up[buff_index] = '}';
	++buff_index;
	buff_up[buff_index] = 0; 							// add string terminator, for safety
#if DUSB>=1
	if (debug>=2) {
		Serial.print(F("RXPK:: "));
		Serial.println((char *)(buff_up + 12));			// DEBUG: display JSON payload
	}
	if (debug>= 2) {
		Serial.print(F("RXPK:: package length="));
		Serial.println(buff_index);
	}
#endif
	return(buff_index);
}// buildPacket





// ----------------------------------------------------------------------------
// UP UP UP UP UP UP UP UP UP UP UP UP UP UP UP UP UP UP UP UP UP UP UP UP UP 
// Receive a LoRa package over the air, LoRa
//
// Receive a LoRa message and fill the buff_up char buffer.
// returns values:
// - returns the length of string returned in buff_up
// - returns -1 when no message arrived.
//
// This is the "highlevel" function called by loop()
// _state is S_RX when starting and
// _state is S_STANDBY when leaving function
// ----------------------------------------------------------------------------
int receivePacket()
{
	uint8_t buff_up[TX_BUFF_SIZE]; 					// buffer to compose the upstream packet to backend server
	long SNR;
	uint8_t message[128] = { 0x00 };					// MSG size is 128 bytes for rx
	uint8_t messageLength = 0;
	
	// Regular message received, see SX1276 spec table 18
	// Next statement could also be a "while" to combine several messages received
	// in one UDP message as the Semtech Gateway spec does allow this.
	// XXX Not yet supported

		// Take the timestamp as soon as possible, to have accurate reception timestamp
		// TODO: tmst can jump if micros() overflow.
		uint32_t tmst = (uint32_t) micros();			// Only microseconds, rollover in 
		lastTmst = tmst;								// Following/according to spec
		
		// Handle the physical data read from LoraUp
		if (LoraUp.payLength > 0) {

			// externally received packet, so last parameter is false (==LoRa external)
            int build_index = buildPacket(tmst, buff_up, LoraUp, false);

			// REPEATER is a special function where we retransmit received 
			// message on _ICHANN to _OCHANN.
			// Note:: For the moment _OCHANN is not allowed to be same as _ICHANN
#if REPEATER==1
			if (!sendLora(LoraUp.payLoad, LoraUp.payLength)) {
				return(-3);
			}
#endif

			LoraUp.payLength = 0;
			LoraUp.payLoad[0] = 0x00;
			
			// This is one of the potential problem areas.
			// If possible, USB traffic should be left out of interrupt routines
			// rxpk PUSH_DATA received from node is rxpk (*2, par. 3.2)
#ifdef _TTNSERVER
			if (!sendUdp(ttnServer, _TTNPORT, buff_up, build_index)) {
				return(-1); 							// received a message
			}
			yield();
#endif

#ifdef _THINGSERVER
			if (!sendUdp(thingServer, _THINGPORT, buff_up, build_index)) {
				return(-2); 							// received a message
			}
#endif
			return(build_index);
        }
		
	return(0);											// failure no message read
	
}//receivePacket

