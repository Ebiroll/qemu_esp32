// 1-channel LoRa Gateway for ESP8266
// Copyright (c) 2016, 2017 Maarten Westenberg
// Verison 5.0.1
// Date: 2017-11-15
//
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the MIT License
// which accompanies this distribution, and is available at
// https://opensource.org/licenses/mit-license.php
//
// NO WARRANTY OF ANY KIND IS PROVIDED
//
// Author: Maarten Westenberg (mw12554@hotmail.com)
//
// This file contains code for using the single channel gateway also as a sensor node. 
// Please specify the DevAddr and the AppSKey below (and on your LoRa backend).
// Also you will have to choose what sensors to forward to your application.
//
// ============================================================================
		
#if GATEWAYNODE==1

unsigned char DevAddr[4]  = _DEVADDR ;				// see ESP-sc-gway.h


// ----------------------------------------------------------------------------
// XXX Experimental Read Internal Sensors
//
// You can monitor some settings of the RFM95/sx1276 chip. For example the temperature
// which is set in REGTEMP in FSK mode (not in LORA). Or the battery value.
// Find some sensible sensor values for LoRa radio and read them below in separate function
//
// ----------------------------------------------------------------------------
//uint8_t readInternal(uint8_t reg) {
//
//	return 0;
//}


// ----------------------------------------------------------------------------
// LoRaSensors() is a function that puts sensor values in the MACPayload and 
// sends these values up to the server. For the server it is impossible to know 
// whther or not the message comes from a LoRa node or from the gateway.
//
// The example code below adds a battery value in lCode (encoding protocol) but
// of-course you can add any byte string you wish
//
// Parameters: 
//	- buf: contains the buffer to put the sensor values in
// Returns:
//	- The amount of sensor characters put in the buffer 
// ----------------------------------------------------------------------------
static int LoRaSensors(uint8_t *buf) {

	uint8_t internalSersors;
	//internalSersors = readInternal(0x1A);
	//if (internalSersors > 0) {		
	//	return (internalSersors);
	//}
	
	
	buf[0] = 0x86;									// 134; User code <lCode + len==3 + Parity
	buf[1] = 0x80;									// 128; lCode code <battery>
	buf[2] = 0x3F;									//  63; lCode code <value>
	// Parity = buf[0]==1 buf[1]=1 buf[2]=0 ==> even, so last bit of first byte must be 0
	
	return(3);	// return the number of bytes added to payload
}


// ----------------------------------------------------------------------------
// XOR()
// perform x-or function for buffer and key
// Since we do this ONLY for keys and X, Y we know that we need to XOR 16 bytes.
//
// ----------------------------------------------------------------------------
static void mXor(uint8_t *buf, uint8_t *key) {
	for (uint8_t i = 0; i < 16; ++i) buf[i] ^= key[i];
}


// ----------------------------------------------------------------------------
// SHIFT-LEFT
// Shift the buffer buf left one bit
// Parameters:
//	- buf: An array of uint8_t bytes
//	- len: Length of the array in bytes
// ----------------------------------------------------------------------------
static void shift_left(uint8_t * buf, uint8_t len) {
    while (len--) {
        uint8_t next = len ? buf[1] : 0;			// len 0 to 15

        uint8_t val = (*buf << 1);
        if (next & 0x80) val |= 0x01;
        *buf++ = val;
    }
}


// ----------------------------------------------------------------------------
// generate_subkey
// RFC 4493, para 2.3
// ----------------------------------------------------------------------------
static void generate_subkey(uint8_t *key, uint8_t *k1, uint8_t *k2) {

	memset(k1, 0, 16);								// Fill subkey1 with 0x00
	
	// Step 1: Assume k1 is an all zero block
	AES_Encrypt(k1,key);
	
	// Step 2: Analyse outcome of Encrypt operation (in k1), generate k1
	if (k1[0] & 0x80) {
		shift_left(k1,16);
		k1[15] ^= 0x87;
	}
	else {
		shift_left(k1,16);
	}
	
	// Step 3: Generate k2
	for (uint8_t i=0; i<16; i++) k2[i]=k1[i];
	if (k1[0] & 0x80) {								// use k1(==k2) according rfc 
		shift_left(k2,16);
		k2[15] ^= 0x87;
	}
	else {
		shift_left(k2,16);
	}
	
	// step 4: Done, return k1 and k2
	return;
}


// ----------------------------------------------------------------------------
// ENCODEPACKET
// In Sensor mode, we have to encode the user payload before sending.
// The library files for AES are added to the library directory in AES.
// For the moment we use the AES library made by ideetron as this library
// is also used in the LMIC stack and is small in size.
//
// The function below follows the LoRa spec exactly.
//
// The resulting mumber of Bytes is returned by the functions. This means
// 16 bytes per block, and as we add to the last block we also return 16
// bytes for the last block.
//
// The LMIC code does not do this, so maybe we shorten the last block to only
// the meaningful bytes in the last block. This means that encoded buffer
// is exactly as big as the original message.
//
// NOTE:: Be aware that the LICENSE of the used AES library files 
//	that we call with AES_Encrypt() is GPL3. It is used as-is,
//  but not part of this code.
//
// cmac = aes128_encrypt(K, Block_A[i])
// ----------------------------------------------------------------------------
uint8_t encodePacket(uint8_t *Data, uint8_t DataLength, uint16_t FrameCount, uint8_t Direction) {

	unsigned char AppSKey[16] = _APPSKEY ;	// see ESP-sc-gway.h
	uint8_t i, j;
	uint8_t Block_A[16];
	uint8_t bLen=16;						// Block length is 16 except for last block in message
		
	uint8_t restLength = DataLength % 16;	// We work in blocks of 16 bytes, this is the rest
	uint8_t numBlocks  = DataLength / 16;	// Number of whole blocks to encrypt
	if (restLength>0) numBlocks++;			// And add block for the rest if any

	for(i = 1; i <= numBlocks; i++) {
		Block_A[0] = 0x01;
		
		Block_A[1] = 0x00; 
		Block_A[2] = 0x00; 
		Block_A[3] = 0x00; 
		Block_A[4] = 0x00;

		Block_A[5] = Direction;				// 0 is uplink

		Block_A[6] = DevAddr[3];			// Only works for and with ABP
		Block_A[7] = DevAddr[2];
		Block_A[8] = DevAddr[1];
		Block_A[9] = DevAddr[0];

		Block_A[10] = (FrameCount & 0x00FF);
		Block_A[11] = ((FrameCount >> 8) & 0x00FF);
		Block_A[12] = 0x00; 				// Frame counter upper Bytes
		Block_A[13] = 0x00;					// These are not used so are 0

		Block_A[14] = 0x00;

		Block_A[15] = i;

		// Encrypt and calculate the S
		AES_Encrypt(Block_A, AppSKey);
		
		// Last block? set bLen to rest
		if ((i == numBlocks) && (restLength>0)) bLen = restLength;
		
		for(j = 0; j < bLen; j++) {
			*Data = *Data ^ Block_A[j];
			Data++;
		}
	}
	//return(numBlocks*16);			// Do we really want to return all 16 bytes in lastblock
	return(DataLength);				// or only 16*(numBlocks-1)+bLen;
}


// ----------------------------------------------------------------------------
// MICPACKET()
// Provide a valid MIC 4-byte code (par 2.4 of spec, RFC4493)
// 		see also https://tools.ietf.org/html/rfc4493
//
// Although our own handler may choose not to interpret the last 4 (MIC) bytes
// of a PHYSPAYLOAD physical payload message of in internal sensor,
// The official TTN (and other) backends will intrpret the complete message and
// conclude that the generated message is bogus.
// So we sill really simulate internal messages coming from the -1ch gateway
// to come from a real sensor and append 4 MIC bytes to every message that are 
// perfectly legimate
// Parameters:
//	- data:			uint8_t array of bytes = ( MHDR | FHDR | FPort | FRMPayload )
//	- len:			8=bit length of data, normally less than 64 bytes
//	- FrameCount:	16-bit framecounter
//	- dir:			0=up, 1=down
//
// B0 = ( 0x49 | 4 x 0x00 | Dir | 4 x DevAddr | 4 x FCnt |  0x00 | len )
// MIC is cmac [0:3] of ( aes128_cmac(NwkSKey, B0 | Data )
//
// ----------------------------------------------------------------------------
uint8_t micPacket(uint8_t *data, uint8_t len, uint16_t FrameCount, uint8_t dir) {


	unsigned char NwkSKey[16] = _NWKSKEY ;
	uint8_t Block_B[16];
	uint8_t X[16];
	uint8_t Y[16];
	
	// ------------------------------------
	// build the B block used by the MIC process
	Block_B[0]= 0x49;						// 1 byte MIC code
			
	Block_B[1]= 0x00;						// 4 byte 0x00
	Block_B[2]= 0x00;
	Block_B[3]= 0x00;
	Block_B[4]= 0x00;
	
	Block_B[5]= dir;						// 1 byte Direction
	
	Block_B[6]= DevAddr[3];					// 4 byte DevAddr
	Block_B[7]= DevAddr[2];
	Block_B[8]= DevAddr[1];
	Block_B[9]= DevAddr[0];
	
	Block_B[10]= (FrameCount & 0x00FF);		// 4 byte FCNT
	Block_B[11]= ((FrameCount >> 8) & 0x00FF);
	Block_B[12]= 0x00; 						// Frame counter upper Bytes
	Block_B[13]= 0x00;						// These are not used so are 0
	
	Block_B[14]= 0x00;						// 1 byte 0x00
	
	Block_B[15]= len;						// 1 byte len
	
	// ------------------------------------
	// Step 1: Generate the subkeys
	//
	uint8_t k1[16];
	uint8_t k2[16];
	generate_subkey(NwkSKey, k1, k2);
	
	// ------------------------------------
	// Copy the data to a new buffer which is prepended with Block B0
	//
	uint8_t micBuf[len+16];					// B0 | data
	for (uint8_t i=0; i<16; i++) micBuf[i]=Block_B[i];
	for (uint8_t i=0; i<len; i++) micBuf[i+16]=data[i];
	
	// ------------------------------------
	// Step 2: Calculate the number of blocks for CMAC
	//
	uint8_t numBlocks = len/16 + 1;			// Compensate for B0 block
	if ((len % 16)!=0) numBlocks++;			// If we have only a part block, take it all
	
	// ------------------------------------
	// Step 3: Calculate padding is necessary
	//
	uint8_t restBits = len%16;				// if numBlocks is not a multiple of 16 bytes
	
	
	// ------------------------------------
	// Step 5: Make a buffer of zeros
	//
	memset(X, 0, 16);
	
	// ------------------------------------
	// Step 6: Do the actual encoding according to RFC
	//
	for(uint8_t i= 0x0; i < (numBlocks - 1); i++) {
		for (uint8_t j=0; j<16; j++) Y[j] = micBuf[(i*16)+j];
		mXor(Y, X);
		AES_Encrypt(Y, NwkSKey);
		for (uint8_t j=0; j<16; j++) X[j] = Y[j];
	}
	

	// ------------------------------------
	// Step 4: If there is a rest Block, padd it
	// Last block. We move step4 to the end as we need Y
	// to compute the last block
	// 
	if (restBits) {
		for (uint8_t i=0; i<16; i++) {
			if (i< restBits) Y[i] = micBuf[((numBlocks-1)*16)+i];
			if (i==restBits) Y[i] = 0x80;
			if (i> restBits) Y[i] = 0x00;
		}
		mXor(Y, k2);
	}
	else {
		for (uint8_t i=0; i<16; i++) {
			Y[i] = micBuf[((numBlocks-1)*16)+i];
		}
		mXor(Y, k1);
	}
	mXor(Y, X);
	AES_Encrypt(Y,NwkSKey);
	
	// ------------------------------------
	// Step 7: done, return the MIC size. 
	// Only 4 bytes are returned (32 bits), which is less than the RFC recommends.
	// We return by appending 4 bytes to data, so there must be space in data array.
	//
	data[len+0]=Y[0];
	data[len+1]=Y[1];
	data[len+2]=Y[2];
	data[len+3]=Y[3];
	return 4;
}


#if _CHECK_MIC==1
// ----------------------------------------------------------------------------
// CHECKMIC
// Function to check the MIC computed for existing messages and for new messages
// Parameters:
//	- buf: LoRa buffer to check in bytes, last 4 bytes contain the MIC
//	- len: Length of buffer in bytes
//	- key: Key to use for MIC. Normally this is the NwkSKey
//
// ----------------------------------------------------------------------------
static void checkMic(uint8_t *buf, uint8_t len, uint8_t *key) {
	uint8_t cBuf[len+1];
	
	if (debug>=2) {
		Serial.print(F("old="));
		for (uint8_t i=0; i<len; i++) { 
			printHexDigit(buf[i]); 
			Serial.print(' '); 
		}
		Serial.println();
	}	
	for (uint8_t i=0; i<len-4; i++) cBuf[i] = buf[i];
	len -=4;
	
	uint16_t FrameCount = ( cBuf[7] * 256 ) + cBuf[6];
	len += micPacket(cBuf, len, FrameCount, 0);
	
	if (debug>=2) {
		Serial.print(F("new="));
		for (uint8_t i=0; i<len; i++) { 
			printHexDigit(cBuf[i]); 
			Serial.print(' '); 
		}
		Serial.println();
	}
}
#endif

// ----------------------------------------------------------------------------
// SENSORPACKET
// The gateway may also have local sensors that need reporting.
// We will generate a message in gateway-UDP format for upStream messaging
// so that for the backend server it seems like a LoRa node has reported a
// sensor value.
//
// NOTE: We do not need ANY LoRa functions here since we are on the gateway.
// We only need to send a gateway message upstream that looks like a node message.
//
// NOTE:: This function does encrypt the sensorpayload, and the backend
//		picks it up fine as decoder thinks it is a MAC message.
//
// Par 4.0 LoraWan spec:
//	PHYPayload =	( MHDR | MACPAYLOAD | MIC ) 
// which is equal to
//					( MHDR | ( FHDR | FPORT | FRMPAYLOAD ) | MIC )
//
//	This function makes the totalpackage and calculates MIC
// Te maximum size of the message is: 12 + ( 9 + 2 + 64 ) + 4	
// So message size should be lass than 128 bytes if Payload is limited to 64 bytes.
//
// return value:
//	- On success returns the number of bytes to send
//	- On error returns -1
// ----------------------------------------------------------------------------
int sensorPacket() {

	uint8_t buff_up[512];								// Declare buffer here to avoid exceptions
	uint8_t message[64]={ 0 };							// Payload, init to 0
	uint8_t mlength = 0;
	uint32_t tmst = micros();
	
	// In the next few bytes the fake LoRa message must be put
	// PHYPayload = MHDR | MACPAYLOAD | MIC
	// MHDR, 1 byte
	// MIC, 4 bytes
	
	// ------------------------------
	// MHDR (Para 4.2), bit 5-7 MType, bit 2-4 RFU, bit 0-1 Major
	message[0] = 0x40;									// MHDR 0x40 == unconfirmed up message,
														// FRU and major are 0
	
	// -------------------------------
	// FHDR consists of 4 bytes addr, 1 byte Fctrl, 2 byte FCnt, 0-15 byte FOpts
	// We support ABP addresses only for Gateways
	message[1] = DevAddr[3];							// Last byte[3] of address
	message[2] = DevAddr[2];
	message[3] = DevAddr[1];
	message[4] = DevAddr[0];							// First byte[0] of Dev_Addr
	
	message[5] = 0x00;									// FCtrl is normally 0
	message[6] = frameCount % 0x100;					// LSB
	message[7] = frameCount / 0x100;					// MSB

	// -------------------------------
	// FPort, either 0 or 1 bytes. Must be != 0 for non MAC messages such as user payload
	//
	message[8] = 0x01;									// FPort must not be 0
	mlength = 9;
	
	// FRMPayload; Payload will be AES128 encoded using AppSKey
	// See LoRa spec para 4.3.2
	// You can add any byte string below based on you personal choice of sensors etc.
	//
	// Payload bytes in this example are encoded in the LoRaCode(c) format
	uint8_t PayLength = LoRaSensors((uint8_t *)(message+mlength));
	
	// we have to include the AES functions at this stage in order to generate LoRa Payload.
	uint8_t CodeLength = encodePacket((uint8_t *)(message+mlength), PayLength, (uint16_t)frameCount, 0);

	mlength += CodeLength;								// length inclusive sensor data
	
	// MIC, Message Integrity Code
	// As MIC is used by TTN (and others) we have to make sure that
	// framecount is valid and the message is correctly encrypted.
	// Note: Until MIC is done correctly, TTN does not receive these messages
	//		 The last 4 bytes are MIC bytes.
	//
	mlength += micPacket((uint8_t *)(message), mlength, (uint16_t)frameCount, 0);

	// So now our package is ready, and we can send it up through the gateway interface
	// Note Be aware that the sensor message (which is bytes) in message will be
	// be expanded if the server expacts JSON messages.
	//
	int buff_index = buildPacket(tmst, buff_up, message, mlength, true);
	
	frameCount++;
	
	// In order to save the memory, we only write the framecounter
	// to EEPROM every 10 values. It also means that we will invalidate
	// 10 value when restarting the gateway.
	//
	if (( frameCount % 10)==0) writeGwayCfg(CONFIGFILE);
	
	//yield();								// XXX Can we remove this here?
	
	if (buff_index > 512) {
		if (debug>0) Serial.println(F("sensorPacket:: ERROR buffer size too large"));
		return(-1);
	}
	
	if (!sendUdp(ttnServer, _TTNPORT, buff_up, buff_index)) {
		return(-1);
	}
#ifdef _THINGSERVER
	if (!sendUdp(thingServer, _THINGPORT, buff_up, buff_index)) {
		return(-1);
	}
#endif

	if (_cad) {
		// Set the state to CAD scanning after receiving
		_state = S_SCAN;						// Inititialise scanner
		cadScanner();
	}
	else {
		// Reset all RX lora stuff
		_state = S_RX;
		rxLoraModem();	
	}
		
	return(buff_index);
}

#endif //GATEWAYNODE==1